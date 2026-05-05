#include <stdint.h>
#include <math.h>
#include "fpga_bridge.h"
#include "synth_functions.h"



int osc_find_note_slot( struct oscillator *oscillators, uint8_t note){
    for(int i = 0; i < NUM_OSCILLATORS; i++){
        if(oscillators[i].in_use && oscillators[i].note == note){
            return i;
        }
    }
    return -1;
}


int osc_find_free_slot( struct oscillator *oscillators){
    for(int i = 0; i < NUM_OSCILLATORS; i++){
        if(!oscillators[i].in_use){
            return i;
        }
    }
    return -1;
}

uint16_t note_to_step_size(uint8_t note){
    double frequency = 440.0 * pow(2.0, (note-69) / 12.0);
    double step = frequency * 65536.0 / (double)SAMPLE_RATE;
    uint32_t rounded = (uint32_t)(step + 0.5);
    return (uint16_t)(rounded > 0xFFFFu ? 0xFFFFu : rounded);
}
