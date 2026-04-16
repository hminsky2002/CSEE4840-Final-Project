#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "wavetable.h"
#include "modload.h"

static fpga_handle_t *g_handle = NULL;

typedef struct {
    int8_t note; // -1 = free
} voice_state_t;

static voice_state_t voice_states[NUM_VOICES];

static void init_voices(void) {
    for (int i = 0; i < NUM_VOICES; i++)
        voice_states[i].note = -1;
}

static int allocate_voice(void) {
    for (int i = 0; i < NUM_VOICES; i++)
        if (voice_states[i].note < 0) return i;
    return -1;
}

static int find_voice(uint8_t note) {
    for (int i = 0; i < NUM_VOICES; i++)
        if (voice_states[i].note == (int8_t)note) return i;
    return -1;
}

static void handle_sigint(int sig) {
    (void)sig;
    if (g_handle) {
        for (int v = 0; v < NUM_VOICES; v++)
            fpga_set_note_on(g_handle, v, 0);
        fpga_cleanup(g_handle);
    }
    exit(0);
}

int fpga_init(fpga_handle_t *handle) {
    handle->mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (handle->mem_fd < 0) {
        perror("Failed to open /dev/mem");
        return -1;
    }

    handle->lw_bridge = mmap(
        NULL,
        LW_BRIDGE_SPAN,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        handle->mem_fd,
        LW_BRIDGE_BASE
    );

    if (handle->lw_bridge == MAP_FAILED) {
        perror("Failed to mmap");
        close(handle->mem_fd);
        return -1;
    }
    return 0;
}

void fpga_cleanup(fpga_handle_t *handle) {
    if (handle->lw_bridge)
        munmap((void *)handle->lw_bridge, LW_BRIDGE_SPAN);
    if (handle->mem_fd >= 0)
        close(handle->mem_fd);
}

void fpga_set_note_on(fpga_handle_t *handle, uint8_t voice, uint8_t on) {
    handle->lw_bridge[VOICE_NOTE_ON(voice)] = (uint16_t)on;
}

void fpga_set_step_size(fpga_handle_t *handle, uint8_t voice, uint16_t step_size) {
    handle->lw_bridge[VOICE_STEP_SIZE(voice)] = step_size;
}

void fpga_set_velocity(fpga_handle_t *handle, uint8_t voice, uint8_t velocity) {
    handle->lw_bridge[VOICE_VELOCITY(voice)] = (uint16_t)velocity;
}

void fpga_set_slot_select(fpga_handle_t *handle, uint8_t voice, uint8_t slot) {
    handle->lw_bridge[VOICE_SLOT_SELECT(voice)] = (uint16_t)slot;
}

uint16_t note_to_step_size(uint8_t note) {
    double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
    uint32_t step = (uint32_t)((frequency * (1 << 20)) / SAMPLE_RATE);
    return (uint16_t)(step > 0xFFFF ? 0xFFFF : step);
}

int write_wavetable_to_fpga(fpga_handle_t *handle, uint8_t slot, const int16_t *samples, int n) {
    volatile uint16_t *slot_base = handle->lw_bridge
                                 + WAVETABLE_BASE_OFFSET
                                 + ((size_t)slot * WAVETABLE_SLOT_WORDS);
    for (int i = 0; i < n; i++)
        slot_base[i] = (uint16_t)samples[i];
    return 0;
}

int load_wavetable(fpga_handle_t *handle, uint8_t slot, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) { perror("load_wavetable"); return -1; }
    int16_t samples[TABLE_SIZE];
    size_t n = fread(samples, sizeof(int16_t), TABLE_SIZE, f);
    fclose(f);
    if ((int)n != TABLE_SIZE) {
        fprintf(stderr, "load_wavetable: expected %d samples, got %zu\n", TABLE_SIZE, n);
        return -1;
    }
    write_wavetable_to_fpga(handle, slot, samples, TABLE_SIZE);
    return 0;
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("midi_to_fpga: starting up\n");

    fpga_handle_t handle;
    if (fpga_init(&handle) < 0) {
        fprintf(stderr, "fpga_init failed — run with sudo?\n");
        return 1;
    }
    printf("mmap'd lw_bridge at %p\n", (void *)handle.lw_bridge);
    g_handle = &handle;
    signal(SIGINT, handle_sigint);

    printf("probe: writing voice0 note_on=0 ...");
    fpga_set_note_on(&handle, 0, 0);
    printf(" ok\n");
    printf("probe: writing wavetable[0][0]=0 ...");
    handle.lw_bridge[WAVETABLE_BASE_OFFSET] = 0;
    printf(" ok\n");

    int16_t samples[TABLE_SIZE];
    generate_wavetable(samples, WAVE_SINE);
    write_wavetable_to_fpga(&handle, 0, samples, TABLE_SIZE);
    generate_wavetable(samples, WAVE_SAW);
    write_wavetable_to_fpga(&handle, 1, samples, TABLE_SIZE);
    generate_wavetable(samples, WAVE_SQUARE);
    write_wavetable_to_fpga(&handle, 2, samples, TABLE_SIZE);
    generate_wavetable(samples, WAVE_TRI);
    write_wavetable_to_fpga(&handle, 3, samples, TABLE_SIZE);
    printf("Wavetables loaded into slots 0-3\n");

    int looped_samples[] = {1, 2, 5, 6, 7, 8, 17, 18};
    for (int i = 0; i < 8; i++)
        load_mod_sample(&handle, "_funky_stuff_.mod", looped_samples[i], 4 + i);
    printf("MOD samples loaded into slots 4-11\n");

    init_voices();
    for (int v = 0; v < NUM_VOICES; v++)
        fpga_set_slot_select(&handle, v, 4);

    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);

    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) continue;

        uint8_t status = midi_packet.status & MIDI_STATUS_MASK;

        if (status == MIDI_NOTE_ON && midi_packet.velocity > 0) {
            int v = allocate_voice();
            if (v < 0) continue;
            voice_states[v].note = midi_packet.note;
            fpga_set_step_size(&handle, v, note_to_step_size(midi_packet.note));
            fpga_set_velocity(&handle, v, midi_packet.velocity);
            fpga_set_note_on(&handle, v, 1);
        } else if (status == MIDI_NOTE_OFF
                   || (status == MIDI_NOTE_ON && midi_packet.velocity == 0)) {
            int v = find_voice(midi_packet.note);
            if (v < 0) continue;
            fpga_set_note_on(&handle, v, 0);
            voice_states[v].note = -1;
        }
    }
}
