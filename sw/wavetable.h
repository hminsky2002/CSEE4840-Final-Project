#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include <stdint.h>
#include <stddef.h>

typedef enum { WAVE_SINE, WAVE_SAW, WAVE_SQUARE, WAVE_TRI } wave_type_t;

/* fill TABLE_SIZE int16_t entries in samples with the requested waveform. */
void generate_wavetable(int16_t *samples, wave_type_t type);

/*
 * Load a 16-bit PCM WAV file from `path` into a freshly malloc'd int16_t
 * buffer. Stereo files are downmixed to mono by averaging channels.
 *
 * On success returns 0, writes the buffer pointer to *samples_out (caller owns
 * it and must free()), writes the mono sample count to *n_samples_out, and
 * writes the source sample rate to *sample_rate_out.
 *
 * On failure returns -1 and prints a diagnostic to stderr.
 */
int load_wav(const char *path,
             int16_t **samples_out,
             size_t   *n_samples_out,
             uint32_t *sample_rate_out);

#endif /* _WAVETABLE_H */
