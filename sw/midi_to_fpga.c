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

static fpga_handle_t *g_handle = NULL;

static void handle_sigint(int sig) {
    (void)sig;
    if (g_handle) {
        fpga_set_note_on(g_handle, 0);
        fpga_cleanup(g_handle);
    }
    midi_dump_log();
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
    if (handle->lw_bridge) {
	munmap((void *)handle->lw_bridge, LW_BRIDGE_SPAN);
    }
    if (handle->mem_fd >= 0) {
	close(handle->mem_fd);
    }
}

void fpga_set_note_on(fpga_handle_t *handle, uint8_t on) {
	handle->lw_bridge[NOTE_ON_OFFSET] = (uint16_t)on;
}

void fpga_set_step_size(fpga_handle_t *handle, uint16_t step_size) {
	handle->lw_bridge[STEP_SIZE_OFFSET] = step_size;
}

void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity) {
	handle->lw_bridge[VELOCITY_OFFSET] = (uint16_t)velocity;
}

uint16_t note_to_step_size(uint8_t note) {
	double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
	uint32_t step = (uint32_t)((frequency * TABLE_SIZE) / SAMPLE_RATE);
	return (uint16_t)(step > 0xFFFF ? 0xFFFF : step);
}


int write_wavetable_to_fpga(fpga_handle_t *handle, const int16_t *samples, int n) {
    for (int i = 0; i < n; i+= 2)
        handle->lw_bridge[WAVETABLE_OFFSET + i/2] = (((uint32_t)(uint16_t)samples[i + 1] << 16) 
                                                    | ((uint32_t)(uint16_t)samples[i]));
    return 0;
}

int load_wavetable(fpga_handle_t *handle, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) { perror("load_wavetable"); return -1; }
    int16_t samples[TABLE_SIZE];
    size_t n = fread(samples, sizeof(int16_t), TABLE_SIZE, f);
    fclose(f);
    if ((int)n != TABLE_SIZE) {
        fprintf(stderr, "load_wavetable: expected %d samples, got %zu\n", TABLE_SIZE, n);
        return -1;
    }
    write_wavetable_to_fpga(handle, samples, TABLE_SIZE);
    return 0;
}

int main() {
    /* mmap at startup */
    fpga_handle_t handle;
    fpga_init(&handle);
    g_handle = &handle;
    signal(SIGINT, handle_sigint);

    /* Wavetable upload deferred — hardware address bus needs to be widened first */

    /* open MIDI device */
    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);

    /* main event loop */
    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) continue;

        if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_ON && midi_packet.velocity > 0) {
            uint16_t step_size = note_to_step_size(midi_packet.note);
            fpga_set_step_size(&handle, step_size);
            fpga_set_velocity(&handle, midi_packet.velocity);
            fpga_set_note_on(&handle, 1);
        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF
                   || midi_packet.velocity == 0) {
            fpga_set_note_on(&handle, 0);
        }
    }
}
