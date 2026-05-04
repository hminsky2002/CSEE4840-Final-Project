#ifndef _SYNTH_FUNCTIONS_H
#define _SYNTH_FUNCTIONS_H

#include <stdint.h>
#include <stdbool.h>

/*
* core struct, respresents a single oscillator and its corresponding registers in the fpga
*/

struct oscillator {
    uint16_t step_size;
    uint16_t control;
    uint16_t wavetable_slot;
    uint16_t resolution; /* unused right now*/
    uint8_t note;
    bool in_use;
};

uint16_t note_to_step_size(uint8_t note);


#endif