"""Generate a 4-slot sine-wave wavetable bin (all slots identical, for debugging)."""
import math
import struct

SLOT_SIZE = 32768
NUM_SLOTS = 4
PEAK = 0x4000

samples = [int(round(PEAK * math.sin(2.0 * math.pi * i / SLOT_SIZE)))
           for i in range(SLOT_SIZE)]

with open("bins/sine.bin", "wb") as f:
    for _ in range(NUM_SLOTS):
        f.write(struct.pack("<{}h".format(SLOT_SIZE), *samples))

print("wrote 4 slots of sine to bins/sine.bin")
