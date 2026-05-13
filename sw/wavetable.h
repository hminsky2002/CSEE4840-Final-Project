#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include <stdint.h>
#include <stddef.h>
#include "fpga_bridge.h"


#define WAVETABLE_DEFAULT_BASE_STEP 357u
#define WAVETABLE_DEFAULT_SLOT_RESOLUTION 8u

/* per-slot pitch anchor and HW phase-increment shift, populated from
   the .meta sidecar at startup. Defaults preserve legacy single-cycle
   behavior if the manifest is missing. */
extern uint16_t wavetable_base_step[NUM_TABLE_SLOTS];
extern uint8_t  wavetable_slot_resolution[NUM_TABLE_SLOTS];

/* Loads wavetable bin generated from tools/build_wavetable.py.
   File layout: 24B header ("WTSY" magic, u16 version, u16 num_meta_slots,
   then per-slot u16 base_step + u8 resolution + u8 reserved), followed by
   one TABLE_SIZE-sample audio block per populated slot. Also populates
   wavetable_base_step[] and wavetable_slot_resolution[] from the header. */
int load_wavetable_bin(const char *path, peripheral *lw_bus);

#endif
