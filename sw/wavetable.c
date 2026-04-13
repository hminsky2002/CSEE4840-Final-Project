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

void save_wavetable_bin(int16_t *samples, int n, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {                          // always check fopen
        perror("fopen");
        return;
    }

    size_t written = fwrite(samples, sizeof(int16_t), n, f);
    if (written != n) {                // check all samples were written
        perror("fwrite");
    }

    fclose(f);
}

/* 
 * example usage: save wavetable to txt file and print
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

/* 
 * example usage: save wavetable to bin file
 * TODO: delete later
 */
// int main() {
//     int16_t samples[TABLE_SIZE];
//     generate_wavetable(samples, WAVE_SINE);
//     save_wavetable_bin(samples, TABLE_SIZE, "wavetable.bin");
//     return 0;
// }
