#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include <stdint.h>
#include <stddef.h>
#include "fpga_bridge.h"

#define MAX_WAVETABLE_SLOTS 4

/*
 * Load a pre-baked wavetable bank produced by tools/build_wavetable.py.
 * The file is raw little-endian int16, exactly TABLE_SIZE samples per slot,
 * up to MAX_WAVETABLE_SLOTS slots concatenated. Pushes each slot to the FPGA
 * and records how many were loaded.
 *
 * Returns the number of slots loaded, or -1 on failure.
 */
int load_wavetable_bin(const char *path, fpga_handle_t *handle);

/* True if `slot` is a valid index into the most recently loaded bank. */
int wavetable_slot_valid(int slot);

#endif /* _WAVETABLE_H */
