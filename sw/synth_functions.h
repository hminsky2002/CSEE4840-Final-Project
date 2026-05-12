#ifndef _SYNTH_FUNCTIONS_H
#define _SYNTH_FUNCTIONS_H

#include <stdint.h>
#include <stdbool.h>

/*
* core struct, respresents a single oscillator and its corresponding registers in the fpga
*/
typedef enum { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } env_phase_t;
struct oscillator {
    uint16_t step_size;
    uint16_t control;
    uint16_t velocity;
    uint16_t wavetable_slot;
    uint16_t resolution; /* unused right now*/
    uint8_t note;
    bool in_use;

    env_phase_t phase;
    uint16_t    env_amp_q8;
    uint16_t    sustain;
    uint16_t    attack_step;
    uint16_t    decay_step;
    uint16_t    release_step;
    uint16_t    peak;
};

uint16_t note_to_step_size(uint8_t note);

int osc_find_free_slot( struct oscillator *oscillators);
int osc_find_note_slot( struct oscillator *oscillators, uint8_t note);

#endif