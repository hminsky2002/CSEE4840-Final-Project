#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include <stdint.h>
#include <stddef.h>
#include "fpga_bridge.h"


/* Loads wavetable bin generated from tools/build_wavetable.py. 
   File is raw little-endian int16, with exactly TABLE_SIZE samples 
   per slot. 
*/
int load_wavetable_bin(const char *path, peripheral *lw_bus);

#endif 