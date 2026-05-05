import argparse
import math
import struct

SLOT_SIZE = 32768

ap = argparse.ArgumentParser(
    description="generate a single-cycle bandlimited sawtooth (sum of 1/n harmonics) "
                "and emit a 64KB wavetable binary loadable by load_wavetable_bin")
ap.add_argument("-o", "--output", default="rich_saw.bin")
ap.add_argument("-n", "--harmonics", type=int, default=128,
                help="number of harmonics to sum (default 128)")
ap.add_argument("--headroom", type=float, default=0.92,
                help="peak normalization target as fraction of int16 range (default 0.92)")
args = ap.parse_args()

samples_f = [0.0] * SLOT_SIZE
two_pi_over_n = 2.0 * math.pi / SLOT_SIZE
for n in range(1, args.harmonics + 1):
    amp = 1.0 / n
    omega = two_pi_over_n * n
    for i in range(SLOT_SIZE):
        samples_f[i] += amp * math.sin(omega * i)

peak = max(abs(s) for s in samples_f)
scale = (args.headroom * 32767.0) / peak

samples_i = [max(-32768, min(32767, int(round(s * scale)))) for s in samples_f]

with open(args.output, "wb") as f:
    f.write(struct.pack("<{}h".format(SLOT_SIZE), *samples_i))

print("Wrote {} samples ({} bytes) to {} -- bandlimited saw, {} harmonics".format(
    SLOT_SIZE, SLOT_SIZE * 2, args.output, args.harmonics))
