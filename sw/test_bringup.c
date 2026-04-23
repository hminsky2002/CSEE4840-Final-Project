/*
 * Minimal bring-up test for the new wave_table_synth hardware + wave_synth
 * kernel driver. Writes a sawtooth into wavetable slot 0, starts voice 0 at
 * an A4-ish step, sleeps a few seconds, stops.
 *
 * Build:  gcc -Wall -O2 test_bringup.c -o test_bringup
 * Run:    sudo ./test_bringup      (needs /dev/wave_synth)
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* Byte size of the peripheral window (matches DTS: 0x40000). */
#define WAVE_SYNTH_WIN_BYTES 0x40000

/* Word offsets (into a uint16_t *). */
#define OSC_STEP(v)   (0x8000u + (v) * 4u + 0u)
#define OSC_CTRL(v)   (0x8000u + (v) * 4u + 1u)
#define OSC_TABLE(v)  (0x8000u + (v) * 4u + 2u)
#define AMP_CTRL      0x8080u

/* Wavetable slot base (word offset). Each slot is 2048 samples. */
#define WAVETABLE_WORD(slot, sample)  (((slot) * 2048u) + (sample))

/* Control register values. */
#define CTRL_STOP   0x0001u
#define CTRL_START  0x0002u
#define CTRL_RESET  0x0003u

int main(void) {
    int fd = open("/dev/wave_synth", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/wave_synth");
        return 1;
    }

    volatile uint16_t *regs = mmap(NULL, WAVE_SYNTH_WIN_BYTES,
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);
    if (regs == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* 1. Load a recognizable sawtooth into slot 0. */
    printf("writing sawtooth into wavetable slot 0...\n");
    for (int i = 0; i < 2048; i++) {
        int32_t s = (int32_t)i * 32 - 32768;  /* -32768 .. +32736 */
        regs[WAVETABLE_WORD(0, i)] = (uint16_t)(int16_t)s;
    }

    /* 2. Configure voice 0. step_size = round(440 * 65536 / 48000) = 601. */
    uint16_t step = 601;

    printf("configuring voice 0: step=%u (A4), slot=0\n", step);
    regs[OSC_STEP(0)]  = step;
    regs[OSC_TABLE(0)] = 0;
    regs[OSC_CTRL(0)]  = CTRL_RESET;    /* clear phase */
    regs[OSC_CTRL(0)]  = CTRL_START;    /* start sounding */

    /* 3. Amplifier full scale so the placeholder mix path can be heard
     *    (Phase 1 mix ignores amp_ctrl but writing it is harmless). */
    regs[AMP_CTRL] = 127;

    printf("playing for 3 seconds... (headphone out)\n");
    sleep(3);

    printf("stop.\n");
    regs[OSC_CTRL(0)] = CTRL_STOP;

    munmap((void *)regs, WAVE_SYNTH_WIN_BYTES);
    close(fd);
    return 0;
}
