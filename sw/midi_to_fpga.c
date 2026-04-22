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

/* Pitch-bend wheel center value (14-bit, MSB:LSB = 0x40:0x00).
 * Wheel at center = no pitch change; full down = -1 octave, full up = +1 octave.
 * Only voices already active when the wheel moves get bent — new note-ons
 * always start at base pitch. */
#define PITCH_BEND_CENTER 8192

/* CC number that flips arpeggiator mode. CC 123 ("All Notes Off") is what the
 * controller's panic button sends (e.g. "BF 7B 00"). While arp is on a worker
 * thread keeps one held voice audible at a time, advancing every ARP_STEP_NS. */
#define ARP_TOGGLE_CC 123
#define ARP_STEP_NS   (100L * 1000L * 1000L)   /* 100 ms per step */

static pthread_mutex_t g_synth_lock = PTHREAD_MUTEX_INITIALIZER;
static bool g_arp_enabled = false;

static uint16_t scaled_step(uint16_t base_step, double ratio) {
    double s = (double)base_step * ratio;
    if (s < 1.0) return 1;
    if (s > 0xFFFFu) return 0xFFFFu;
    return (uint16_t)(s + 0.5);
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

/* Push voice i's step register back to the value implied by its current
 * step_size and pitch_ratio — i.e., undo any arp-induced muting. */
static void restore_voice_step(fpga_handle_t *handle, int i) {
    fpga_set_step(handle, i,
                  scaled_step(oscillators[i].step_size,
                              oscillators[i].pitch_ratio));
}

/* Arpeggiator worker. Idles unless g_arp_enabled. Each tick: gather currently-
 * held voices, pick the next one in round-robin, restore its step and zero the
 * rest. Voice releases / new keypresses are seen on the next tick because we
 * re-scan oscillators[] every iteration. */
static void *run_arp(void *arg) {
    fpga_handle_t *handle = (fpga_handle_t *)arg;
    struct timespec dt = { 0, ARP_STEP_NS };
    int idx = 0;
    while (1) {
        pthread_mutex_lock(&g_synth_lock);
        if (g_arp_enabled) {
            int held[NUM_OSCILLATORS], n = 0;
            for (int i = 0; i < NUM_OSCILLATORS; i++) {
                if (oscillators[i].in_use) held[n++] = i;
            }
            if (n > 0) {
                int active = held[idx % n];
                idx = (idx + 1) % n;
                for (int j = 0; j < n; j++) {
                    if (held[j] == active) restore_voice_step(handle, held[j]);
                    else                   fpga_set_step(handle, held[j], 0);
                }
            }
        }
        pthread_mutex_unlock(&g_synth_lock);
        nanosleep(&dt, NULL);
    }
    return NULL;
}

void *run_midi_reciever(void *arg){
    fpga_handle_t *handle = (fpga_handle_t *)arg;

    /* open MIDI device */
    int midi_fd = midi_open();
    if (midi_fd < 0) return NULL;

    /* main event loop */
    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_fd, &midi_packet) < 0) {
            continue;
        }

        pthread_mutex_lock(&g_synth_lock);

        if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_ON) {
            if (find_note_slot(midi_packet.note) == -1) {
                int i = find_free_slot();

                if (i >= 0) {
                    uint16_t step = note_to_step_size(midi_packet.note);
                    oscillators[i].note        = midi_packet.note;
                    oscillators[i].step_size   = step;
                    oscillators[i].wavetable   = g_current_wavetable;
                    oscillators[i].pitch_ratio = 1.0;   /* fresh note: base pitch */
                    oscillators[i].in_use      = true;

                    /* In arp mode, start muted; the arp thread will unmute it
                     * on its turn. Otherwise start at full step like before. */
                    fpga_voice_start(handle, i,
                                     g_arp_enabled ? 0 : step,
                                     g_current_wavetable);
                }
            }

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_CONTROL_CHANGE) {
            /* Controller's panic button (CC 123 "All Notes Off") toggles arp. */
            if (midi_packet.note == ARP_TOGGLE_CC) {
                g_arp_enabled = !g_arp_enabled;
                if (!g_arp_enabled) {
                    /* leaving arp mode: un-mute every held voice */
                    for (int i = 0; i < NUM_OSCILLATORS; i++) {
                        if (oscillators[i].in_use) restore_voice_step(handle, i);
                    }
                }
                printf("Arp %s\n", g_arp_enabled ? "ON" : "OFF");
            }

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF) {
            int i = find_note_slot(midi_packet.note);

            if (i >= 0) {
                oscillators[i].in_use = false;
                fpga_voice_stop(handle, i);
            }

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_PITCH_BEND) {
            /* 14-bit value: MSB is byte 2 (attack), LSB is byte 1 (note).
             * Center 8192 = no bend; 0 = -1 octave; 16383 = +1 octave.
             * Apply only to voices currently held — future note-ons are
             * unaffected because they reset pitch_ratio to 1.0. */
            int value = ((int)midi_packet.attack << 7) | midi_packet.note;
            double ratio = pow(2.0, ((double)(value - PITCH_BEND_CENTER)) / (double)PITCH_BEND_CENTER);
            for (int i = 0; i < NUM_OSCILLATORS; i++) {
                if (oscillators[i].in_use) {
                    oscillators[i].pitch_ratio = ratio;
                    /* In arp mode, only the arp thread writes the step
                     * register — otherwise we'd un-mute silenced voices. */
                    if (!g_arp_enabled) {
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

        pthread_mutex_unlock(&g_synth_lock);
    }
    return NULL;
}

static void run_debug_print(void) {
    int midi_fd = midi_open();
    if (midi_fd < 0) {
        fprintf(stderr, "run_debug_print: no MIDI device\n");
        return;
    }
    printf("MIDI debug mode: printing every event. Ctrl+C to quit.\n");
    midi_event_t midi_packet;
    while (midi_read(midi_fd, &midi_packet) == 0) {
        printf("%02x %02x %02x\n",
               midi_packet.status, midi_packet.note, midi_packet.attack);
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

    pthread_t midi_thread, arp_thread;
    pthread_create(&midi_thread, NULL, run_midi_reciever, &handle);
    pthread_create(&arp_thread,  NULL, run_arp,           &handle);
    pthread_join(midi_thread, NULL);   /* runs forever; exit via SIGINT */
    return 0;
}
