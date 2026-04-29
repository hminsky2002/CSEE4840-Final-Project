/*
 *
 * CSEE 4840 Final Project
 *
 * Name/UNI: Henry Minsky (hm3121), Opalina Khanna (ok2373), Sunny Fang (yf2610)
 */
#include "midi.h"
#include <stdio.h>
#include <stdlib.h>
#include<stdint.h>
#include <unistd.h>
#include <fcntl.h>

int midi_open(void){
    char path[256];
    for(int card = 0; card < 8; card++){
        snprintf(path, sizeof(path), "/dev/snd/midiC%dD0",card);
        int fd = open(path, O_RDONLY);
        if(fd >= 0){
            return fd;
        }
    }
    fprintf(stderr, "midi open: no midi sound device found in /dev/snd\n");
    return -1;
}

int midi_read(int fd, midi_event_t *evt){
    static uint8_t running_status;

    uint8_t byte;

    while(1){

        if(read(fd,&byte,1) != 1){
            return -1;
        }

        if(byte >= 0xF8){
            continue;
        }
        if(byte >= 0xF0){
            running_status = 0;
            continue;
        }

        if (byte & 0x80){
            evt->status = byte; 
            running_status = byte;
            if(read(fd,&evt->note,1)!= 1){
                return -1;
            }
        }
        else{
            if(!(running_status & 0x80)){
                continue;
            }
            evt->status = running_status;
            evt->note = byte;
        }

        uint8_t kind = evt->status & 0xF0;
        if(kind == 0xC0 || kind == 0xD0){
            evt->attack = 0;
        }
        else{
            if(read(fd, &evt->attack,1) != 1){
                return -1;
            }
        }
        return 0;
    }

}

void midi_close(int fd){
    if(fd >= 0){
        close(fd);
    }
}