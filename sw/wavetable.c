#include <stdio.h>
#include <stdint.h>
#include "fpga_bridge.h"
#include "wavetable.h"

static int g_num_loaded_slots = 0;

int load_wavetable_bin(const char *path, fpga_handle_t *handle) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("load_wavetable_bin: fopen");
        return -1;
    }

    int16_t slot_buf[TABLE_SIZE];
    int slot = 0;
    while (slot < MAX_WAVETABLE_SLOTS) {
        size_t got = fread(slot_buf, sizeof(int16_t), TABLE_SIZE, f);
        if (got == 0) break;
        if (got != TABLE_SIZE) {
            fprintf(stderr, "load_wavetable_bin: %s: partial slot %d (%zu/%d samples)\n",
                    path, slot, got, TABLE_SIZE);
            fclose(f);
            return -1;
        }
        fpga_load_slot(handle, slot, slot_buf, TABLE_SIZE);
        printf("slot %d: loaded\n", slot);
        slot++;
    }
    fclose(f);

    g_num_loaded_slots = slot;
    return slot;
}

int wavetable_slot_valid(int slot) {
    return slot >= 0 && slot < g_num_loaded_slots;
}
