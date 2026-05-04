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




int main(int argc, char **argv){

    peripheral lw_bus;
    if(fpga_init(&lw_bus) != 0){
        return -1;
    }

    

}



