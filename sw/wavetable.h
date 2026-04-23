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

/*
 * Given an int16_t buffer of length n, return in *trimmed_n_out the largest
 * power-of-two <= n, and in *resolution_out the log2 of that. Buffer itself
 * is untouched; the caller just stops indexing past *trimmed_n_out.
 * Returns 0 on success, -1 if n == 0.
 */
int trim_to_pow2(size_t    n,
                 size_t   *trimmed_n_out,
                 uint16_t *resolution_out);

/* Allocate the private wavetable memory and populate it from src. 0 on success. */
int     wavetable_init(const int16_t *src, size_t n);
/* Return sample at index (masked to the wavetable length for safety). */
int16_t wavetable_read(size_t index);
/* Length of the wavetable memory (0 if uninitialized). */
size_t  wavetable_len(void);

/* Write a mono 16-bit PCM .wav file. Returns 0 on success. */
int write_wav(const char *path,
              const int16_t *samples,
              size_t n_samples,
              uint32_t sample_rate);

#endif /* _WAVETABLE_H */
