#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "fpga_bridge.h"
#include "oscillator.h"
#include "wavetable.h"

#define NUM_OSCILLATORS NUM_VOICES
static struct oscillator oscillators[NUM_OSCILLATORS] = {0};

/* Which wavetable slot new note-ons use (updated by Program Change). */
static uint16_t g_current_wavetable = 0;

static int find_note_slot(uint8_t note) {
    for (int i = 0; i < NUM_OSCILLATORS; i++) {
        if (oscillators[i].in_use && oscillators[i].note == note) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void) {
    for (int i = 0; i < NUM_OSCILLATORS; i++) {
        if (!oscillators[i].in_use) {
            return i;
        }
    }
    return -1;
}

static fpga_handle_t *g_handle = NULL;

/* Hitting this key on the controller force-mutes every voice and clears
 * allocator state — useful when a stuck note happens because a NOTE_OFF
 * got dropped or came in on a different note than the NOTE_ON. */
#define PANIC_NOTE 0x33

/* CC number of the pitch dial on the controller. Value 64 is the "center"
 * (no pitch change); 0 bends down one octave, 127 bends up one octave.
 * Only voices that were already active when the dial moves get bent —
 * new note-ons always start at base pitch. */
#define CC_PITCH_DIAL 8

static uint16_t scaled_step(uint16_t base_step, double ratio) {
    double s = (double)base_step * ratio;
    if (s < 1.0) return 1;
    if (s > 0xFFFFu) return 0xFFFFu;
    return (uint16_t)(s + 0.5);
}

static void panic_all_voices(fpga_handle_t *handle) {
    fpga_all_voices_off(handle);
    for (int i = 0; i < NUM_OSCILLATORS; i++) {
        oscillators[i].in_use = false;
        oscillators[i].note   = 0;
    }
    printf("PANIC: all voices cleared\n");
}

static void handle_sigint(int sig) {
    (void)sig;
    if (g_handle) {
        fpga_cleanup(g_handle);   /* also mutes every voice */
    }
    exit(0);
}

/* Phase is 24-bit; the hardware left-shifts step_size by 8 before adding.
 * So we hand it step = round(freq * 65536 / 48000). A4 (MIDI 69) = 601. */
uint16_t note_to_step_size(uint8_t note) {
    double frequency = 440.0 * pow(2.0, ((double)note - 69.0) / 12.0);
    double step = frequency * 65536.0 / (double)SAMPLE_RATE;
    uint32_t rounded = (uint32_t)(step + 0.5);
    return (uint16_t)(rounded > 0xFFFFu ? 0xFFFFu : rounded);
}

void *run_midi_reciever(void *arg){
    fpga_handle_t *handle = (fpga_handle_t *)arg;

    /* open MIDI device */
    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);

    /* main event loop */
    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) {
            continue;
        }

        if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_ON) {
            if (midi_packet.note == PANIC_NOTE) {
                panic_all_voices(handle);
                continue;
            }
            if (find_note_slot(midi_packet.note) == -1) {
                int i = find_free_slot();

                if (i >= 0) {
                    uint16_t step = note_to_step_size(midi_packet.note);
                    oscillators[i].note        = midi_packet.note;
                    oscillators[i].step_size   = step;
                    oscillators[i].wavetable   = g_current_wavetable;
                    oscillators[i].pitch_ratio = 1.0;   /* fresh note: base pitch */
                    oscillators[i].in_use      = true;

                    fpga_voice_start(handle, i, step, g_current_wavetable);
                }
            }

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF) {
            int i = find_note_slot(midi_packet.note);

            if (i >= 0) {
                oscillators[i].in_use = false;
                fpga_voice_stop(handle, i);
            }

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_CONTROL_CHANGE) {
            if (midi_packet.note == CC_PITCH_DIAL) {
                /* Map dial [0..127] to pitch multiplier [0.5x .. 2x]
                 * around center 64 (one octave either direction).
                 * Apply only to voices currently held — future note-ons
                 * are unaffected because they reset pitch_ratio to 1.0. */
                int dial = midi_packet.attack;
                double ratio = pow(2.0, ((double)dial - 64.0) / 64.0);
                for (int i = 0; i < NUM_OSCILLATORS; i++) {
                    if (oscillators[i].in_use) {
                        oscillators[i].pitch_ratio = ratio;
                        fpga_set_step(handle, i,
                                      scaled_step(oscillators[i].step_size, ratio));
                    }
                }
            }

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_PROGRAM_CHANGE) {
            /* Program Change: program number is in midi_packet.note (byte 2).
             * Switch every oscillator (held included) to the new slot. */
            uint8_t program = midi_packet.note;
            if (wavetable_slot_valid(program)) {
                g_current_wavetable = program;
                for (int i = 0; i < NUM_OSCILLATORS; i++) {
                    oscillators[i].wavetable = program;
                    if (oscillators[i].in_use) {
                        fpga_set_table(handle, i, program);
                    }
                }
                printf("Program change -> slot %u\n", program);
            }
        }
    }
    return NULL;
}

static void run_debug_print(void) {
    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);
    if (!midi_device) {
        fprintf(stderr, "run_debug_print: no MIDI device\n");
        return;
    }
    printf("MIDI debug mode: printing every packet. Ctrl+C to quit.\n");
    midi_event_t midi_packet;
    while (1) {
        midi_read(midi_device, endpoint, &midi_packet);
        /* midi_read already prints the payload */
    }
}

int main(int argc, char **argv) {
    /* mmap at startup */
    fpga_handle_t handle;
    if (fpga_init(&handle) != 0) {
        return 1;
    }
    g_handle = &handle;
    signal(SIGINT, handle_sigint);

    if (argc < 2) {
        /* no wavetable bin -> debug-print mode */
        run_debug_print();
        return 0;
    }

    int loaded = load_wavetable_bin(argv[1], &handle);
    if (loaded <= 0) {
        fprintf(stderr, "No wavetables loaded from %s\n", argv[1]);
        return 1;
    }
    printf("Loaded %d wavetable slot(s). Listening for MIDI.\n", loaded);

    pthread_t midi_thread;
    pthread_create(&midi_thread, NULL, run_midi_reciever, &handle);
    pthread_join(midi_thread, NULL);   /* runs forever; exit via SIGINT */
    return 0;
}
