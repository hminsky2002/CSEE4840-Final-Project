#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "fpga_bridge.h"

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
