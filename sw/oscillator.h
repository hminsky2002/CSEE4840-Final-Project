#include <stdint.h>
#include <stdbool.h>

struct oscillator {
    uint16_t step_size;
    uint16_t control;
    uint16_t wavetable;
    uint16_t reserved;
    uint8_t note;
    bool    in_use;
};
