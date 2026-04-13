#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi_to_fpga.h"
#include "wavetable.h"

void generate_wavetable(int16_t *samples, wave_type_t type) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        float t = (float)i / TABLE_SIZE;
        float s;
        switch (type) {
            case WAVE_SINE:   s = sinf(2.0f * M_PI * t);                    break;
            case WAVE_SAW:    s = 2.0f * t - 1.0f;                          break;
            case WAVE_SQUARE: s = t < 0.5f ? 1.0f : -1.0f;                  break;
            case WAVE_TRI:    s = t < 0.5f ? 4.0f*t - 1.0f : 3.0f - 4.0f*t; break;
        }
        samples[i] = (int16_t)(s * 32767);
    }
}

/* 
 * example usage 
 * TODO: delete later
 */
// int main() {
//     int16_t samples[TABLE_SIZE];
//     generate_wavetable(samples, WAVE_SINE);

//     FILE *f = fopen("wavetable.txt", "w");
//     for (int i = 0; i < TABLE_SIZE; i++) {
//         fprintf(f, "%d %d\n", i, samples[i]);
//     }
//     fclose(f);
//     return 0;
// }

