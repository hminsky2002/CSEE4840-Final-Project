#include <stdint.h>
#include <math.h>
#include "fpga_bridge.h"

uint16_t note_to_step_size(uint8_t note){
    double frequency = 440.0 * pow(2.0, (note-69) / 12.0);
    uint32_t step = (uint32_t)((frequency * TABLE_SIZE)) / SAMPLE_RATE;
    return (uint16_t)(step > 0xFFFF ? 0xFFFF : step);
}
