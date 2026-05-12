#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "fpga_bridge.h"
#include "wavetable.h"

uint16_t wavetable_base_step[NUM_TABLE_SLOTS] = {
    WAVETABLE_DEFAULT_BASE_STEP,
    WAVETABLE_DEFAULT_BASE_STEP,
    WAVETABLE_DEFAULT_BASE_STEP,
    WAVETABLE_DEFAULT_BASE_STEP,
};

uint8_t wavetable_slot_resolution[NUM_TABLE_SLOTS] = {
    WAVETABLE_DEFAULT_SLOT_RESOLUTION,
    WAVETABLE_DEFAULT_SLOT_RESOLUTION,
    WAVETABLE_DEFAULT_SLOT_RESOLUTION,
    WAVETABLE_DEFAULT_SLOT_RESOLUTION,
};

int load_wavetable_bin(const char *path, peripheral *lw_bus){
    FILE *f = fopen(path,"rb");

    if(!f){
        perror("load_wavetable_bin: fopen");
        return -1;
    }

    /* Header: 4B magic "WTSY", u16 version, u16 num_meta_slots,
       then per-slot {u16 base_step, u8 resolution, u8 reserved}. */
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "WTSY", 4) != 0) {
        fprintf(stderr, "wavetable: bad magic in %s\n", path);
        fclose(f);
        return -1;
    }
    uint16_t version, num_meta;
    if (fread(&version, 2, 1, f) != 1 || fread(&num_meta, 2, 1, f) != 1) {
        fprintf(stderr, "wavetable: short read on header\n");
        fclose(f);
        return -1;
    }
    if (version != 1) {
        fprintf(stderr, "wavetable: unsupported version %u\n", version);
        fclose(f);
        return -1;
    }
    for (uint16_t i = 0; i < num_meta; i++) {
        uint16_t bs; uint8_t res, reserved;
        if (fread(&bs, 2, 1, f) != 1
            || fread(&res, 1, 1, f) != 1
            || fread(&reserved, 1, 1, f) != 1) {
            fprintf(stderr, "wavetable: short read on slot %u metadata\n", i);
            fclose(f);
            return -1;
        }
        if (i < NUM_TABLE_SLOTS) {
            wavetable_base_step[i] = bs;
            wavetable_slot_resolution[i] = res;
        }
    }

    static int16_t slot_buf[TABLE_SIZE];
    int slot = 0;

    while(slot < NUM_TABLE_SLOTS){
        size_t tableslot = fread(slot_buf, sizeof(int16_t), TABLE_SIZE, f);

        if(tableslot == 0){
            break;
        }

        if (tableslot != TABLE_SIZE){
            fprintf(stderr, "wavetable: malformed audio block at slot %d (got %zu samples)\n",
                    slot, tableslot);
            fclose(f);
            return -1;
        }
        fpga_load_slot(lw_bus, slot, slot_buf, TABLE_SIZE);
        printf("slot %d: loaded (base_step=%u, resolution=%u)\n",
               slot, wavetable_base_step[slot], wavetable_slot_resolution[slot]);
        slot++;
    }
    fclose(f);

    return slot;
}
