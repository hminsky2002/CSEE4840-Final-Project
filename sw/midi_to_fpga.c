#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "wavetable.h"

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

void fpga_set_step_size(fpga_handle_t *handle, uint32_t step_size) {
	handle->lw_bridge[STEP_SIZE_OFFSET] = step_size;
}

void fpga_set_note_on(fpga_handle_t *handle, uint8_t on) {
	handle->lw_bridge[NOTE_ON_OFFSET] = (uint32_t)on;
}

void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity) {
	handle->lw_bridge[VELOCITY_OFFSET] = (uint32_t)velocity;
}

/* set step_size */
uint32_t note_to_step_size(uint8_t note) {
	double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
	return (uint32_t)((frequency * (TABLE_SIZE)) / SAMPLE_RATE);
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
    for (int i = 0; i < TABLE_SIZE; i++)
        handle->lw_bridge[WAVETABLE_OFFSET + i] = (uint32_t)(uint16_t)samples[i];
    return 0;
}

int write_wavetable_to_fpga(fpga_handle_t *handle, const int16_t *samples, int n) {
    for (int i = 0; i < n; i++)
        handle->lw_bridge[WAVETABLE_OFFSET + i] = (uint32_t)(uint16_t)samples[i];
    return 0;
}

int main() {
    /* mmap at startup */
    fpga_handle_t handle; 
    fpga_init(&handle);
    
#ifdef WAVETABLE_FILE
    load_wavetable(&handle, WAVETABLE_FILE);
#else
    int16_t samples[TABLE_SIZE];
    generate_wavetable(samples, WAVE_SINE);
    write_wavetable_to_fpga(&handle, samples, TABLE_SIZE);
#endif

    /* read midi_input */
    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);

    /* main event loop that waits and reads midi input */
    midi_event_t midi_packet;
    while (1) {
        
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) continue;

        if ((midi_packet.status & 0xF0) == 0x90 && midi_packet.velocity > 0) {
            uint32_t step_size = note_to_step_size(midi_packet.note);
            fpga_set_step_size(&handle, step_size);
            fpga_set_velocity(&handle, midi_packet.velocity);
        } else if ((midi_packet.status & 0xF0) == 0x80
                   || midi_packet.velocity == 0) { // check this 
            fpga_set_note_on(&handle, 0);
        }
    }
}
