#ifndef SYNTH_TO_FPGA_H
#define SYNTH_TO_FPGA_H

#include <stdint.h>
#include <stdbool.h>

#define WAVE_SYNTH_PERIPHERAL_BYTES 0x80000

#define OSC_STEP(v) (0x20000u + (v) * 4u + 0u)
#define OSC_CTRL(v) (0x20000u + (v) * 4u + 1u)
#define OSC_TABLE(v) (0x20000u + (v) * 4u + 2u)


#define SLOT_SIZE  32768
#define NUM_TABLE_SLOTS 4

#define WAVETABLE_WORD(slot,sample) (((slot)* SLOT_SIZE) + (sample))

#define CTRL_IDLE   0x0000u
#define CTRL_STOP   0x0001u
#define CTRL_START  0x0002u
#define CTRL_RESET  0x0003u


#define SAMPLE_RATE 48000
#define NUM_OSCILLATORS 32

typedef struct {
    volatile uint16_t *regs;
    int fd;
} peripheral;

int fpga_init(peripheral *lw_bus);
void fpga_kill_voices(peripheral *lw_bus);
void fpga_cleanup(peripheral *lw_bus);
void fpga_set_step(peripheral *lw_bus, int voice, uint16_t step_size);
void fpga_set_ctrl(peripheral *lw_bus, int voice, uint16_t ctrl);
void fpga_set_table(peripheral *lw_bus, int voice, uint16_t slot);
void fpga_voice_start(peripheral *lw_bus, int voice, uint16_t step_size,
                      uint16_t slot);
void fpga_kill_voice(peripheral *lw_bus, int voice);
int fpga_load_slot(peripheral *lw_bus, int slot, const int16_t *samples,
                   int n);

#endif