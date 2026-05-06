import argparse
import math
import struct

SLOT_SIZE = 32768
MAX_SLOTS = 4

WAVES = ("sine", "saw", "square", "triangle")


def gen(kind, n, peak):
    out = []
    for i in range(n):
        t = i / n  # phase 0..1 across one cycle
        if kind == "sine":
            v = math.sin(2 * math.pi * t)
        elif kind == "saw":
            v = 2 * t - 1
        elif kind == "square":
            v = 1.0 if t < 0.5 else -1.0
        elif kind == "triangle":
            v = 4 * t - 1 if t < 0.5 else 3 - 4 * t
        else:
            raise ValueError("unknown wave: " + kind)
        out.append(max(-32768, min(32767, int(round(v * peak)))))
    return out


ap = argparse.ArgumentParser(description="generate single-cycle synth wavetables to a .bin")
ap.add_argument("waves", nargs="+", choices=WAVES, help="one wave per slot, up to %d" % MAX_SLOTS)
ap.add_argument("-o", "--output", default="wavetable.bin")
ap.add_argument("--peak", type=lambda s: int(s, 0), default=0x4000)

args = ap.parse_args()

if len(args.waves) > MAX_SLOTS:
    raise SystemExit("at most %d slots" % MAX_SLOTS)

with open(args.output, "wb") as f:
    for slot, kind in enumerate(args.waves):
        samples = gen(kind, SLOT_SIZE, args.peak)
        f.write(struct.pack("<{}h".format(SLOT_SIZE), *samples))
        print("slot {}: {} ({} samples, peak {})".format(slot, kind, SLOT_SIZE, args.peak))

print("wrote {} slot(s) to {}".format(len(args.waves), args.output))
