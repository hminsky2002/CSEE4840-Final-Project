#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "fpga_bridge.h"
#include "oscillator.h"
#include "wavetable.h"

static int16_t  *g_sample_buf = NULL;
static size_t    g_sample_n   = 0;
static uint32_t  g_sample_sr  = 0;

#define NUM_OSCILLATORS 32
static struct oscillator oscillators[NUM_OSCILLATORS] = {0};

static int find_note_slot(uint8_t note) {
    for (int i = 0; i < NUM_OSCILLATORS; i++)
        if (oscillators[i].in_use && oscillators[i].note == note) return i;
    return -1;
}

static int find_free_slot(void) {
    for (int i = 0; i < NUM_OSCILLATORS; i++)
        if (!oscillators[i].in_use) return i;
    return -1;
}

static int any_active(void) {
    for (int i = 0; i < NUM_OSCILLATORS; i++)
        if (oscillators[i].in_use) return 1;
    return 0;
}

static fpga_handle_t *g_handle = NULL;

static void handle_sigint(int sig) {
    (void)sig;
    if (g_handle) {
        fpga_set_note_on(g_handle, 0);
        fpga_cleanup(g_handle);
    }
    exit(0);
}

uint16_t note_to_step_size(uint8_t note) {
	double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
	uint32_t step = (uint32_t)((frequency * TABLE_SIZE) / SAMPLE_RATE);
	return (uint16_t)(step > 0xFFFF ? 0xFFFF : step);
}

void *run_midi_reciever(void *arg){
    fpga_handle_t *handle = (fpga_handle_t *)arg;

    /* open MIDI device */
    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);

    /* main event loop */
    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) continue;

        if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_ON) {
            if (find_note_slot(midi_packet.note) == -1) {
                int i = find_free_slot();

                if (i >= 0) {
                    oscillators[i].note      = midi_packet.note;
                    oscillators[i].step_size = note_to_step_size(midi_packet.note);
                    oscillators[i].in_use    = true;
                }
            }

            fpga_set_step_size(handle, note_to_step_size(midi_packet.note));

            fpga_set_note_on(handle, 1);

        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF) {
            int i = find_note_slot(midi_packet.note);

            if (i >= 0) oscillators[i].in_use = false;

            fpga_set_note_on(handle, any_active() ? 1 : 0);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    /* mmap at startup */
    fpga_handle_t handle;
    fpga_init(&handle);
    g_handle = &handle;
    signal(SIGINT, handle_sigint);

    /* Optional WAV sample load (no hardware upload yet — address bus too narrow) */
    if (argc >= 2) {
        if (load_wav(argv[1], &g_sample_buf, &g_sample_n, &g_sample_sr) == 0) {
            printf("Loaded %zu samples @ %u Hz from %s\n",
                   g_sample_n, g_sample_sr, argv[1]);
        }
    }

    pthread_t midi_thread;
    pthread_create(&midi_thread, NULL, run_midi_reciever, &handle);
    pthread_join(midi_thread, NULL);
    return 0;
}
