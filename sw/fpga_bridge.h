#ifndef SYNTH_TO_FPGA_H
#define SYNTH_TO_FPGA_H

#include <stdint.h>
#include <stdbool.h>

#define WAVE_SYNTH_PERIPHERAL_BYTES 0x40000

#define OSC_STEP(v) (0x8000u + (v) * 4u + 0u)
#define OSC_CTRL(v) (0x8000u + (v) * 4u + 1u)
#define OSC_TABLE(v) (0x8000u + (v) * 4u + 2u)
#define OSC_AMP(v) (0x8000u + (v) * 4u + 3u)
#define AMP_CTRL 0x8080u

#define HEX_REG(i)   (0x8100u + (i))

/* DE1-SoC HEX segments are active-low. bit order: gfedcba */
#define SEG_BLANK 0x7Fu
#define SEG_0 0x40u
#define SEG_1 0x79u
#define SEG_2 0x24u
#define SEG_3 0x30u
#define SEG_4 0x19u
#define SEG_5 0x12u
#define SEG_6 0x02u
#define SEG_7 0x78u
#define SEG_8 0x00u
#define SEG_9 0x10u
#define SEG_A 0x08u
#define SEG_B 0x03u
#define SEG_C 0x46u
#define SEG_D 0x21u
#define SEG_E 0x06u
#define SEG_F 0x0Eu
#define SEG_G 0x10u
#define SEG_S 0x12u
#define SEG_I 0x79u
#define SEG_N 0x2Bu
#define SEG_R 0x2Fu
#define SEG_T 0x07u
#define SEG_U 0x41u
#define SEG_Q 0x18u

#define TABLE_SIZE 8192
#define NUM_TABLE_SLOTS 4

#define WAVETABLE_WORD(slot,sample) (((slot)* TABLE_SIZE) + (sample))

#define CTRL_IDLE   0x0000u
#define CTRL_STOP   0x0001u
#define CTRL_START  0x0002u
#define CTRL_RESET  0x0003u

#define SAMPLE_RATE 48000
#define NUM_OSCILLATORS 32

/* adsr macros */
#define ENV_TICK_NS             1000000L
#define ENV_PEAK                255
#define ENV_SUSTAIN_LEVEL       192
#define ENV_ATTACK_PER_TICK     32
#define ENV_DECAY_PER_TICK      2 
#define ENV_RELEASE_PER_TICK    1

typedef struct {
    volatile uint16_t *regs;
    int fd;
} peripheral;

int fpga_init(peripheral *lw_bus);
void fpga_kill_voices(peripheral *lw_bus);
void fpga_cleanup(peripheral *lw_bus);

/* per voice setters */
void fpga_set_step(peripheral *lw_bus, int voice, uint16_t step_size);
void fpga_set_ctrl(peripheral *lw_bus, int voice, uint16_t ctrl);
void fpga_set_table(peripheral *lw_bus, int voice, uint16_t slot);
void fpga_set_amp(peripheral *lw_bus, int voice, uint16_t amp);
void fpga_set_hex(peripheral *lw_bus, int idx, uint8_t pattern);
void fpga_voice_start(peripheral *lw_bus, int voice, uint16_t step_size,
                      uint16_t slot);
void fpga_kill_voice(peripheral *lw_bus, int voice);
int fpga_load_slot(peripheral *lw_bus, int slot, const int16_t *samples,
                   int n);

/* master amp*/
void fpga_set_master_amp(peripheral *lw_bus, uint16_t amp);

#endif