#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include <stdint.h>

typedef enum { WAVE_SINE, WAVE_SAW, WAVE_SQUARE, WAVE_TRI } wave_type_t;

/* fill TABLE_SIZE int16_t entries in samples with the requested waveform. */
void generate_wavetable(int16_t *samples, wave_type_t type);

#endif /* _WAVETABLE_H */