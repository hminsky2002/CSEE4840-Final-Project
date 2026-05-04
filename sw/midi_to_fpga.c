#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "fpga_bridge.h"
#include "wavetable.h"


// our global array to track our oscillator states!
static struct oscillator oscillators[NUM_OSCILLATORS] = {0}; 





int main(int argc, char **argv){

    if(argc < 2){
        fprintf(stderr, "need to pass a *.bin file for loading wavetable in");
    }

    peripheral lw_bus;
    if(fpga_init(&lw_bus) != 0){
        return -1;
    }

    int loaded = load_wavetable_bin(argv[1], &lw_bus);

    if(loaded <= 0){
        fprintf(stderr, "No wavetables loaded, check validity of %s \n",argv[1]);
    }

}



