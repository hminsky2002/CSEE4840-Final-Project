#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "fpga_bridge.h"
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
    if (written != (size_t)n) {        // check all samples were written
        perror("fwrite");
    }

    fclose(f);
}

static struct {
    int16_t *samples;
    size_t   len;
} g_slots[MAX_WAVETABLE_SLOTS] = {{0}};

int wavetable_load(int slot, const int16_t *src, size_t n) {
    if (slot < 0 || slot >= MAX_WAVETABLE_SLOTS) {
        return -1;
    }
    if (!src || n == 0) {
        return -1;
    }
    free(g_slots[slot].samples);
    g_slots[slot].samples = malloc(n * sizeof(int16_t));
    if (!g_slots[slot].samples) {
        perror("wavetable_load: malloc");
        g_slots[slot].len = 0;
        return -1;
    }
    memcpy(g_slots[slot].samples, src, n * sizeof(int16_t));
    g_slots[slot].len = n;
    return 0;
}

int16_t wavetable_read(int slot, size_t index) {
    if (slot < 0 || slot >= MAX_WAVETABLE_SLOTS) {
        return 0;
    }
    if (!g_slots[slot].samples || g_slots[slot].len == 0) {
        return 0;
    }
    return g_slots[slot].samples[index % g_slots[slot].len];
}

size_t wavetable_len(int slot) {
    if (slot < 0 || slot >= MAX_WAVETABLE_SLOTS) {
        return 0;
    }
    return g_slots[slot].len;
}

static void write_u32_le(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
    fwrite(b, 1, 4, f);
}

static void write_u16_le(FILE *f, uint16_t v) {
    uint8_t b[2] = { (uint8_t)v, (uint8_t)(v >> 8) };
    fwrite(b, 1, 2, f);
}

int write_wav(const char *path,
              const int16_t *samples,
              size_t n_samples,
              uint32_t sample_rate) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("write_wav: fopen"); return -1; }

    uint32_t data_bytes = (uint32_t)(n_samples * sizeof(int16_t));
    uint32_t riff_size  = 36 + data_bytes;

    fwrite("RIFF", 1, 4, f);
    write_u32_le(f, riff_size);
    fwrite("WAVE", 1, 4, f);

    fwrite("fmt ", 1, 4, f);
    write_u32_le(f, 16);                    /* fmt chunk size */
    write_u16_le(f, 1);                     /* PCM */
    write_u16_le(f, 1);                     /* channels = mono */
    write_u32_le(f, sample_rate);
    write_u32_le(f, sample_rate * 2);       /* byte rate = SR * channels * bytes/sample */
    write_u16_le(f, 2);                     /* block align = channels * bytes/sample */
    write_u16_le(f, 16);                    /* bits per sample */

    fwrite("data", 1, 4, f);
    write_u32_le(f, data_bytes);

    size_t written = fwrite(samples, sizeof(int16_t), n_samples, f);
    fclose(f);
    if (written != n_samples) {
        fprintf(stderr, "write_wav: %s: only wrote %zu of %zu samples\n", path, written, n_samples);
        return -1;
    }
    return 0;
}

static int read_u32_le(FILE *f, uint32_t *out) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return -1;
    *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return 0;
}

static int read_u16_le(FILE *f, uint16_t *out) {
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2) return -1;
    *out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
    return 0;
}

int load_wav(const char *path,
             int16_t **samples_out,
             size_t   *n_samples_out,
             uint32_t *sample_rate_out) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("load_wav: fopen"); return -1; }

    char riff[4], wave[4];
    uint32_t riff_size;
    if (fread(riff, 1, 4, f) != 4 || read_u32_le(f, &riff_size) != 0 || fread(wave, 1, 4, f) != 4) {
        fprintf(stderr, "load_wav: %s: short read on RIFF header\n", path);
        fclose(f); return -1;
    }
    if (memcmp(riff, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) {
        fprintf(stderr, "load_wav: %s: not a RIFF/WAVE file\n", path);
        fclose(f); return -1;
    }

    uint16_t audio_format = 0, channels = 0, bits_per_sample = 0;
    uint32_t sample_rate = 0;
    int have_fmt = 0;

    while (1) {
        char id[4];
        uint32_t size;
        if (fread(id, 1, 4, f) != 4) {
            fprintf(stderr, "load_wav: %s: reached EOF before data chunk\n", path);
            fclose(f); return -1;
        }
        if (read_u32_le(f, &size) != 0) {
            fprintf(stderr, "load_wav: %s: short read on chunk size\n", path);
            fclose(f); return -1;
        }

        if (memcmp(id, "fmt ", 4) == 0) {
            uint16_t block_align;
            uint32_t byte_rate;
            if (read_u16_le(f, &audio_format) != 0 ||
                read_u16_le(f, &channels) != 0 ||
                read_u32_le(f, &sample_rate) != 0 ||
                read_u32_le(f, &byte_rate) != 0 ||
                read_u16_le(f, &block_align) != 0 ||
                read_u16_le(f, &bits_per_sample) != 0) {
                fprintf(stderr, "load_wav: %s: short read on fmt chunk\n", path);
                fclose(f); return -1;
            }
            (void)byte_rate; (void)block_align;
            if (size > 16) {
                fseek(f, size - 16, SEEK_CUR);
            }
            if (size & 1) {
                fseek(f, 1, SEEK_CUR);
            }
            have_fmt = 1;
        } else if (memcmp(id, "data", 4) == 0) {
            if (!have_fmt) {
                fprintf(stderr, "load_wav: %s: data chunk before fmt chunk\n", path);
                fclose(f); return -1;
            }
            if (audio_format != 1) {
                fprintf(stderr, "load_wav: %s: not PCM (audio_format=%u)\n", path, audio_format);
                fclose(f); return -1;
            }
            if (bits_per_sample != 8 && bits_per_sample != 16) {
                fprintf(stderr, "load_wav: %s: bits_per_sample=%u, expected 8 or 16\n", path, bits_per_sample);
                fclose(f); return -1;
            }
            if (channels != 1 && channels != 2) {
                fprintf(stderr, "load_wav: %s: channels=%u, expected 1 or 2\n", path, channels);
                fclose(f); return -1;
            }

            size_t bytes_per_sample = bits_per_sample / 8;
            size_t total_samples = size / bytes_per_sample;
            size_t mono_samples = total_samples / channels;
            int16_t *buf = malloc(mono_samples * sizeof(int16_t));
            if (!buf) { perror("load_wav: malloc"); fclose(f); return -1; }

            if (bits_per_sample == 16) {
                if (channels == 1) {
                    if (fread(buf, sizeof(int16_t), mono_samples, f) != mono_samples) {
                        fprintf(stderr, "load_wav: %s: short read on data\n", path);
                        free(buf); fclose(f); return -1;
                    }
                } else {
                    int16_t pair[2];
                    for (size_t i = 0; i < mono_samples; i++) {
                        if (fread(pair, sizeof(int16_t), 2, f) != 2) {
                            fprintf(stderr, "load_wav: %s: short read on stereo data\n", path);
                            free(buf); fclose(f); return -1;
                        }
                        buf[i] = (int16_t)(((int32_t)pair[0] + (int32_t)pair[1]) / 2);
                    }
                }
            } else {
                /* 8-bit PCM in WAV is UNSIGNED (0-255, 128=silence).
                 * Center around zero, then shift into the top byte of int16. */
                if (channels == 1) {
                    for (size_t i = 0; i < mono_samples; i++) {
                        uint8_t u;
                        if (fread(&u, 1, 1, f) != 1) {
                            fprintf(stderr, "load_wav: %s: short read on data\n", path);
                            free(buf); fclose(f); return -1;
                        }
                        buf[i] = (int16_t)(((int16_t)u - 128) << 8);
                    }
                } else {
                    uint8_t pair[2];
                    for (size_t i = 0; i < mono_samples; i++) {
                        if (fread(pair, 1, 2, f) != 2) {
                            fprintf(stderr, "load_wav: %s: short read on stereo data\n", path);
                            free(buf); fclose(f); return -1;
                        }
                        int16_t l = (int16_t)(((int16_t)pair[0] - 128) << 8);
                        int16_t r = (int16_t)(((int16_t)pair[1] - 128) << 8);
                        buf[i] = (int16_t)(((int32_t)l + (int32_t)r) / 2);
                    }
                }
            }

            *samples_out = buf;
            *n_samples_out = mono_samples;
            *sample_rate_out = sample_rate;
            fclose(f);
            return 0;
        } else {
            if (fseek(f, size + (size & 1), SEEK_CUR) != 0) {
                fprintf(stderr, "load_wav: %s: fseek failed skipping chunk\n", path);
                fclose(f); return -1;
            }
        }
    }
}
