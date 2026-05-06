"""
Generate a 4-slot band-limited sawtooth wavetable bin.

Each slot contains a different harmonic limit so that no harmonic exceeds
Nyquist (24 kHz) for the pitch range that slot serves:

  Slot  Note range   Harmonics   Highest note       Highest harmonic
  0     C0–B2 (0–47)    128      B2 ≈ 124 Hz → H128 ≈ 15.8 kHz
  1     C3–B4 (48–71)    32      B4 ≈ 494 Hz → H32  ≈ 15.8 kHz
  2     C5–B6 (72–95)     8      B6 ≈ 1976 Hz → H8  ≈ 15.8 kHz
  3     C7+   (96+)       4      B7 ≈ 3951 Hz → H4  ≈ 15.8 kHz

Use with wavetable.c / fpga_load_slot (4 × 32768 int16 samples per slot).
Pick the slot in software via note_to_slot(midi_note).
"""

import argparse
import math
import struct

SLOT_SIZE  = 32768
NUM_SLOTS  = 4
DEFAULT_PEAK = 0x4000   # quarter full-scale, matches AMP_DEFAULT in fpga_bridge.h

SLOT_HARMONICS = [128, 32, 8, 4]

ap = argparse.ArgumentParser(description=__doc__,
                             formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("-o", "--output", default="rich_saw_bandlimited.bin")
ap.add_argument("--peak", type=lambda s: int(s, 0), default=DEFAULT_PEAK,
                help="peak amplitude per slot (default 0x%X)" % DEFAULT_PEAK)
args = ap.parse_args()


def gen_saw_slot(n_harmonics, peak):
    two_pi_over_n = 2.0 * math.pi / SLOT_SIZE
    samples = [0.0] * SLOT_SIZE
    for n in range(1, n_harmonics + 1):
        a = 1.0 / n
        omega = two_pi_over_n * n
        for i in range(SLOT_SIZE):
            samples[i] += a * math.sin(omega * i)

    cur_peak = max(abs(s) for s in samples)
    scale = peak / cur_peak
    return [max(-32768, min(32767, int(round(s * scale)))) for s in samples]


with open(args.output, "wb") as f:
    for slot, n_harm in enumerate(SLOT_HARMONICS):
        samples = gen_saw_slot(n_harm, args.peak)
        f.write(struct.pack("<{}h".format(SLOT_SIZE), *samples))
        print("slot {}: {} harmonics, peak {}".format(slot, n_harm, args.peak))

print("wrote {} slots ({} bytes) to {}".format(
    NUM_SLOTS, NUM_SLOTS * SLOT_SIZE * 2, args.output))
