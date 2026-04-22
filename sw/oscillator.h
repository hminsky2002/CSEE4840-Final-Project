#include <stdint.h>
#include <stdbool.h>

struct oscillator {
    uint16_t step_size;    /* base step (no pitch modulation applied) */
    uint16_t control;
    uint16_t wavetable;
    uint16_t resolution;
    double   pitch_ratio;  /* per-voice pitch multiplier; 1.0 = base pitch */
    double   audio_step;   /* samples advanced per output tick (1.0 = native) */
    double   phase;        /* current sample position into the wavetable */
    uint8_t note;
    bool    in_use;
};
