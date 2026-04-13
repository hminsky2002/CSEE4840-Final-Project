#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi_to_fpga.h"

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

void fpga_set_step_size(fpga_handle_t *handle, uint16_t step_size) {
	handle->lw_bridge[STEP_SIZE_OFFSET] = step_size;
}

void fpga_set_note_on(fpga_handle_t *handle, uint8_t on) {
	handle->lw_bridge[NOTE_ON_OFFSET] = (uint16_t)on;
}

// Do we really need to set velocity of keypress?? 
void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity) {
	handle->lw_bridge[VELOCITY_OFFSET] = (uint16_t)velocity;
}

// FUNCTIONS BELOW FOR MIDI PARSING

int parse_midi_message(midi_msg_t *msg) {
	(void)msg; 
	return 0;
}

void handle_midi_message(fpga_handle_t *handle, midi_msg_t *msg) {
	(void)handle; 
	(void)msg; 
	return;
}

int main(int argc, char *argv[]) {
	(void)argc; 
	(void)argv; 
	return 0;
}