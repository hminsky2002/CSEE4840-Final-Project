#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi_to_fpga.h"

int load_wavetable(fpga_handle_t *handle, const char *filepath){
	FILE *f = fopen(filepath, "rb");
	if (!f) {
		perror("Failed to open wavetable file");
		return -1;
	}
	int16_t samples[WAVETABLE_LENGTH];
	size_t n = fread(samples, sizeof(int16_t), WAVETABLE_LENGTH, f);
	fclose(f);
	if (n != WAVETABLE_LENGTH) {
		fprintf(stderr, "Failed to read complete wavetable: expected %d samples, got %zu\n", WAVETABLE_LENGTH, n);
		return -1;
	}
	// writing into FPGA memory 
	for (size_t i = 0; i < WAVETABLE_LENGTH; i++) {
		// might have to do some math here for unsigned???
		// MAKE THIS IOCTL
		handle->lw_bridge[WAVETABLE_OFFSET + i] = (uint16_t)(samples[i]); 
	}
	return 0;
}

uint16_t note_to_step_size(uint8_t note) {
	double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
	uint16_t step_size = (uint16_t)((frequency * (WAVETABLE_LENGTH)) / SAMPLE_RATE);
	return step_size;
}

