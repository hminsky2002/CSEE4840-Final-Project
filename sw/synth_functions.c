#include <stdint.h>
#include <math.h>
#include "fpga_bridge.h"
#include "synth_functions.h"
#include "wavetable.h"



int osc_find_note_slot( struct oscillator *oscillators, uint8_t note){
    int found = 0;
    for(int i = 0; i < NUM_OSCILLATORS; i++){
        if(oscillators[i].in_use && oscillators[i].note == note){
           oscillators[i].phase = ENV_RELEASE;
           found++;
        }
    }
    if(found >= 0){
        return found;
    }
    else {
        return -1;
    }
}


int osc_find_free_slot( struct oscillator *oscillators){
    for(int i = 0; i < NUM_OSCILLATORS; i++){
        if(!oscillators[i].in_use){
            return i;
        }
    }
    return -1;
}

uint16_t voice_step(uint8_t note, uint8_t slot){
    if(slot >= NUM_TABLE_SLOTS) slot = 0;
    double mult = pow(2.0, ((double)note - 60.0) / 12.0);
    double step = (double)wavetable_base_step[slot] * mult;
    uint32_t rounded = (uint32_t)(step + 0.5);
    return (uint16_t)(rounded > 0xFFFFu ? 0xFFFFu : rounded);
}
