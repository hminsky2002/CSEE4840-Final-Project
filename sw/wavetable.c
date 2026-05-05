#include <stdio.h>
#include <stdint.h>
#include "fpga_bridge.h"
#include "wavetable.h"

int load_wavetable_bin(const char *path, peripheral *lw_bus){
    FILE *f = fopen(path,"rb");

    if(!f){
        perror("load_wavetable_bin: fopen");
        return -1;
    }

    int16_t slot_buf[SLOT_SIZE];
    int slot = 0;

    while(slot < NUM_TABLE_SLOTS){
        size_t tableslot = fread(slot_buf, sizeof(int16_t),SLOT_SIZE, f);

        if(tableslot == 0){
            break;
        }

        if (tableslot != SLOT_SIZE){
            fprintf(stderr,"wavetable binary file is malformed, wrong number of samples read");
            fclose(f);
            return -1;
        }
        fpga_load_slot(lw_bus,slot,slot_buf,SLOT_SIZE);
        printf("slot %d: loaded\n", slot);
        slot++;
    }
    fclose(f);

    return slot;
}