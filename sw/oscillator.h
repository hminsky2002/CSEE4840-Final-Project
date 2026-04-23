#include <stdint.h>
#include <stdbool.h>

struct oscillator {
    uint16_t step_size;
    uint16_t control;
    uint16_t wavetable;
    uint16_t resolution;
    uint32_t audio_step;   /* software phase step: f * 2^32 / SAMPLE_RATE */
    uint32_t phase;        /* software phase accumulator */
    uint8_t note;
    bool    in_use;
};
