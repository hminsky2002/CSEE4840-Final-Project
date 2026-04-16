#ifndef _MODLOAD_H
#define _MODLOAD_H

#include "midi_to_fpga.h"

// Load a sample's loop region from a ProTracker MOD file into an FPGA
// wavetable slot. sample_num is 1-based (1..31). Converts 8-bit signed
// PCM to 16-bit, truncates or zero-pads to TABLE_SIZE.
// Returns 0 on success, -1 on error.
int load_mod_sample(fpga_handle_t *handle, const char *mod_path,
                    int sample_num, uint8_t slot);

#endif /* _MODLOAD_H */
