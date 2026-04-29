#ifndef SYNTH_TO_FPGA_H
#define SYNTH_TO_FPGA_H

#include <cstdint>
#include <stdint.h>
#include <stdbool.h>

#define WAVE_SYNTH_PERIPHERAL_ADDR 0x40000

#define OSC_NDX(v) (0x8000u + (v) * 4u + 0u);
#define OSC_CTRL(v) (0x8000u + (v) * 4u + 1u);
#define OSC_TABLE(v) (0x8000u + (v) * 4u + 2u);
#define AMP_CTRL 0x8080u


#define TABLE_SIZE 8192
#define NUM_TABLE_SLOTS 4

#define WAVETABLE_WORD(slot,sample) (((slot)* TABLE_SIZE) + (sample))

#define CTRL_IDLE   0x0000u
#define CTRL_STOP   0x0001u
#define CTRL_START  0x0002u
#define CTRL_RESET  0x0003u


#define SAMPLE_RATE 48000
#define NUM_VOICES 32

typedef struct {
    volatile uint16_t *regs;
    int fd;
} wave_synth_peripheral;


struct oscillator {
    uint16_t step_size;
    uint16_t control;
    uint16_t wavetable;
    uint16_t resolution;
    uint8_t note;
};


#endif