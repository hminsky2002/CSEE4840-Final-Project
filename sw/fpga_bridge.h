#ifndef FPGA_BRIDGE_H
#define FPGA_BRIDGE_H

#include <stdint.h>

// Lightweight HPS-to-FPGA bridge base address (fixed on Cyclone V)
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00200000

// Peripheral base: 0x0010 bytes from LW bridge base → 0x08 uint16_t words
// (confirmed via soc_system.qsys baseAddress = 0x0010)
#define SYNTH_BASE_WORD_OFFSET  0x08

// Register map (16-bit word offsets into lw_bridge pointer)
// Byte address = 0xFF200000 + offset * 2
#define NOTE_ON_OFFSET    (SYNTH_BASE_WORD_OFFSET + 0)
#define STEP_SIZE_OFFSET  (SYNTH_BASE_WORD_OFFSET + 1)
#define VELOCITY_OFFSET   (SYNTH_BASE_WORD_OFFSET + 2)
#define WAVETABLE_OFFSET  (SYNTH_BASE_WORD_OFFSET + 3)

#define TABLE_SIZE  2048
#define SAMPLE_RATE 48000

typedef struct {
    volatile uint16_t *lw_bridge;
    int mem_fd;
} fpga_handle_t;

int  fpga_init(fpga_handle_t *handle);
void fpga_cleanup(fpga_handle_t *handle);

void fpga_set_note_on(fpga_handle_t *handle, uint8_t on);
void fpga_set_step_size(fpga_handle_t *handle, uint16_t step_size);
void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity);

int write_wavetable_to_fpga(fpga_handle_t *handle, const int16_t *samples, int n);
int load_wavetable(fpga_handle_t *handle, const char *filepath);

#endif /* FPGA_BRIDGE_H */
