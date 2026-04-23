#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "fpga_bridge.h"
#include "oscillator.h"
#include "wavetable.h"

static int16_t  *g_sample_buf  = NULL;
static size_t    g_sample_n    = 0;
static uint32_t  g_sample_sr   = 0;
static uint16_t  g_sample_res  = 0;  /* log2(g_sample_n) after pow-2 trim */

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

#define NATIVE_NOTE 69   /* note at which the loaded WAV plays at original speed (A4) */

static double note_to_audio_step(uint8_t note) {
    return pow(2.0, ((double)note - NATIVE_NOTE) / 12.0);
}

#define SYNTH_SECONDS    10
#define SYNTH_OUT_PATH   "out.wav"
#define SYNTH_CHUNK_MS   10
#define SYNTH_CHUNK_SAMP (SAMPLE_RATE / (1000 / SYNTH_CHUNK_MS))

void *run_synth(void *arg) {
    (void)arg;
    size_t n_samples = (size_t)SYNTH_SECONDS * SAMPLE_RATE;
    size_t wt_len    = wavetable_len();
    int16_t *out = malloc(n_samples * sizeof(int16_t));
    if (!out) { perror("run_synth: malloc"); return NULL; }

    struct timespec chunk_sleep = {0, SYNTH_CHUNK_MS * 1000L * 1000L};

    for (size_t chunk_start = 0; chunk_start < n_samples; chunk_start += SYNTH_CHUNK_SAMP) {
        size_t chunk_end = chunk_start + SYNTH_CHUNK_SAMP;
        if (chunk_end > n_samples) chunk_end = n_samples;

        for (size_t t = chunk_start; t < chunk_end; t++) {
            int32_t mix = 0;
            for (int j = 0; j < NUM_OSCILLATORS; j++) {
                if (!oscillators[j].in_use || wt_len == 0) continue;
                oscillators[j].phase += oscillators[j].audio_step;
                while (oscillators[j].phase >= (double)wt_len) {
                    oscillators[j].phase -= (double)wt_len;
                }
                size_t idx = (size_t)oscillators[j].phase;
                mix += wavetable_read(idx);
            }
            if (mix > INT16_MAX) mix = INT16_MAX;
            if (mix < INT16_MIN) mix = INT16_MIN;
            out[t] = (int16_t)mix;
        }

        nanosleep(&chunk_sleep, NULL);
    }

    write_wav(SYNTH_OUT_PATH, out, n_samples, SAMPLE_RATE);
    free(out);
    printf("Wrote %zu samples to %s\n", n_samples, SYNTH_OUT_PATH);
    return NULL;
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
                    oscillators[i].note       = midi_packet.note;
                    oscillators[i].step_size  = note_to_step_size(midi_packet.note);
                    oscillators[i].resolution = g_sample_res;
                    oscillators[i].audio_step = note_to_audio_step(midi_packet.note);
                    oscillators[i].phase      = 0;
                    oscillators[i].in_use     = true;
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

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <wavfile>\n", argv[0]);
        return 1;
    }
    size_t raw_n;
    if (load_wav(argv[1], &g_sample_buf, &raw_n, &g_sample_sr) != 0) {
        return 1;
    }
    trim_to_pow2(raw_n, &g_sample_n, &g_sample_res);
    printf("Loaded %zu samples @ %u Hz from %s (trimmed to %zu = 2^%u for looping)\n",
           raw_n, g_sample_sr, argv[1], g_sample_n, g_sample_res);
    wavetable_init(g_sample_buf, g_sample_n);

    pthread_t midi_thread, synth_thread;
    pthread_create(&midi_thread,  NULL, run_midi_reciever, &handle);
    pthread_create(&synth_thread, NULL, run_synth,         NULL);

    pthread_join(synth_thread, NULL);
    return 0;
}
