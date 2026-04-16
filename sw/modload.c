#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "modload.h"

#define MOD_NUM_SAMPLES   31
#define MOD_TITLE_LEN     20
#define MOD_SAMPLE_DESC   30
#define MOD_NAME_LEN      22
#define MOD_SEQ_OFFSET    952
#define MOD_SEQ_LEN       128
#define MOD_PATTERN_START 1084
#define MOD_PATTERN_SIZE  1024

typedef struct {
    char     name[MOD_NAME_LEN + 1];
    uint16_t length;      // in bytes
    uint8_t  finetune;
    uint8_t  volume;
    uint16_t loop_start;  // in bytes
    uint16_t loop_length; // in bytes
} mod_sample_t;

static uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)(p[0] << 8 | p[1]);
}

int load_mod_sample(fpga_handle_t *handle, const char *mod_path,
                    int sample_num, uint8_t slot) {
    if (sample_num < 1 || sample_num > MOD_NUM_SAMPLES) {
        fprintf(stderr, "modload: sample_num %d out of range 1..31\n", sample_num);
        return -1;
    }

    FILE *f = fopen(mod_path, "rb");
    if (!f) { perror("modload: fopen"); return -1; }

    // Parse all 31 sample descriptors
    mod_sample_t samples[MOD_NUM_SAMPLES];
    fseek(f, MOD_TITLE_LEN, SEEK_SET);

    for (int i = 0; i < MOD_NUM_SAMPLES; i++) {
        uint8_t desc[MOD_SAMPLE_DESC];
        if (fread(desc, 1, MOD_SAMPLE_DESC, f) != MOD_SAMPLE_DESC) {
            fprintf(stderr, "modload: truncated sample descriptor %d\n", i + 1);
            fclose(f);
            return -1;
        }
        memcpy(samples[i].name, desc, MOD_NAME_LEN);
        samples[i].name[MOD_NAME_LEN] = '\0';
        samples[i].length      = read_be16(desc + 22) * 2;
        samples[i].finetune    = desc[24];
        samples[i].volume      = desc[25];
        samples[i].loop_start  = read_be16(desc + 26) * 2;
        samples[i].loop_length = read_be16(desc + 28) * 2;
    }

    // Find highest pattern number in sequence to compute sample data offset
    uint8_t seq[MOD_SEQ_LEN];
    fseek(f, MOD_SEQ_OFFSET, SEEK_SET);
    if (fread(seq, 1, MOD_SEQ_LEN, f) != MOD_SEQ_LEN) {
        fprintf(stderr, "modload: truncated pattern sequence\n");
        fclose(f);
        return -1;
    }

    uint8_t max_pattern = 0;
    for (int i = 0; i < MOD_SEQ_LEN; i++) {
        if (seq[i] > max_pattern)
            max_pattern = seq[i];
    }

    long sample_data_offset = MOD_PATTERN_START
                            + (long)(max_pattern + 1) * MOD_PATTERN_SIZE;

    // Compute byte offset of the target sample within sample data
    long sample_offset = 0;
    for (int i = 0; i < sample_num - 1; i++)
        sample_offset += samples[i].length;

    mod_sample_t *s = &samples[sample_num - 1];

    if (s->loop_length <= 2) {
        fprintf(stderr, "modload: sample %d (\"%s\") has no loop region\n",
                sample_num, s->name);
        fclose(f);
        return -1;
    }

    printf("modload: sample %d \"%s\" — len=%u loop_start=%u loop_len=%u → slot %d",
           sample_num, s->name, s->length, s->loop_start, s->loop_length, slot);

    int n_samples = s->loop_length;
    if (n_samples > TABLE_SIZE) {
        printf(" (truncated to %d)", TABLE_SIZE);
        n_samples = TABLE_SIZE;
    }
    printf("\n");

    // Read the loop region (signed 8-bit PCM)
    long loop_file_offset = sample_data_offset + sample_offset + s->loop_start;
    fseek(f, loop_file_offset, SEEK_SET);

    uint8_t *raw = malloc(n_samples);
    if (!raw) { perror("modload: malloc"); fclose(f); return -1; }

    size_t got = fread(raw, 1, n_samples, f);
    fclose(f);

    if ((int)got != n_samples) {
        fprintf(stderr, "modload: expected %d bytes, read %zu\n", n_samples, got);
        free(raw);
        return -1;
    }

    // Convert 8-bit signed → 16-bit signed, zero-pad to TABLE_SIZE
    int16_t out[TABLE_SIZE];
    memset(out, 0, sizeof(out));
    for (int i = 0; i < n_samples; i++)
        out[i] = (int16_t)((int8_t)raw[i]) << 8;

    free(raw);

    write_wavetable_to_fpga(handle, slot, out, TABLE_SIZE);
    return 0;
}
