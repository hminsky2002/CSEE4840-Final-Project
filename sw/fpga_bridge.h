#ifndef FPGA_BRIDGE_H
#define FPGA_BRIDGE_H

#include <stdint.h>

/* 256 KB peripheral window: wavetable BRAM + 32 voices + amp_ctrl. */
#define WAVE_SYNTH_WIN_BYTES 0x40000

/* Word offsets (into a uint16_t *). */
#define OSC_STEP(v)   (0x8000u + (v) * 4u + 0u)
#define OSC_CTRL(v)   (0x8000u + (v) * 4u + 1u)
#define OSC_TABLE(v)  (0x8000u + (v) * 4u + 2u)
#define AMP_CTRL      0x8080u

/* Wavetable slot base (word offset). Each slot is 2048 samples. */
#define WAVETABLE_WORD(slot, sample)  (((slot) * 2048u) + (sample))

/* 2-bit ctrl encoding per oscillator. */
#define CTRL_IDLE   0x0000u
#define CTRL_STOP   0x0001u
#define CTRL_START  0x0002u
#define CTRL_RESET  0x0003u

#define TABLE_SIZE   2048
#define SAMPLE_RATE  48000
#define NUM_VOICES   32

typedef struct {
    volatile uint16_t *regs;   /* mmap'd peripheral window */
    int                fd;
} fpga_handle_t;

int  fpga_init(fpga_handle_t *handle);
void fpga_cleanup(fpga_handle_t *handle);

/* Raw per-voice register writes. */
void fpga_set_step(fpga_handle_t *handle, int voice, uint16_t step_size);
void fpga_set_ctrl(fpga_handle_t *handle, int voice, uint16_t ctrl);
void fpga_set_table(fpga_handle_t *handle, int voice, uint16_t slot);

/* Convenience: configure a voice and start it from a clean phase. */
void fpga_voice_start(fpga_handle_t *handle, int voice,
                      uint16_t step_size, uint16_t slot);
/* Stop a voice (phase held, output muted). */
void fpga_voice_stop(fpga_handle_t *handle, int voice);
/* Stop every voice (used on shutdown). */
void fpga_all_voices_off(fpga_handle_t *handle);

/* Copy up to TABLE_SIZE samples into the given wavetable slot. */
int  fpga_load_slot(fpga_handle_t *handle, int slot,
                    const int16_t *samples, int n);

#endif /* FPGA_BRIDGE_H */
