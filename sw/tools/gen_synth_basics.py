"""Emit a 4-slot wavetable .bin of one-cycle synth basics (sine/saw/square/triangle)
sized for the current HW (TABLE_SIZE=32768 int16 per slot).

By default, the saw/square/triangle are band-limited via truncated Fourier series
to suppress aliasing. Use --harmonics 0 to get the naive (discontinuous) versions.
"""

import argparse
import math
import struct
from pathlib import Path

TABLE_SIZE = 32768
NUM_SLOTS = 4
PEAK = 32767
DEFAULT_HARMONICS = 32  # clean below ~MIDI 60; gentle aliasing above


def sine(N, _harmonics, peak):
    return [int(round(peak * math.sin(2.0 * math.pi * i / N))) for i in range(N)]


def naive_saw(N, _harmonics, peak):
    return [int(round(-peak + (2 * peak) * i / (N - 1))) for i in range(N)]


def naive_square(N, _harmonics, peak):
    half = N // 2
    return [-peak if i < half else peak for i in range(N)]


def naive_triangle(N, _harmonics, peak):
    half = N // 2
    out = []
    for i in range(N):
        if i < half:
            out.append(int(round(-peak + (2 * peak) * i / (half - 1))))
        else:
            j = i - half
            out.append(int(round(peak - (2 * peak) * j / (half - 1))))
    return out


def _normalize(samples, peak):
    m = max(abs(x) for x in samples) or 1.0
    return [int(round(peak * x / m)) for x in samples]


def fourier_saw(N, H, peak):
    """sum_{n=1..H} sin(2*pi*n*t)/n — normalized to peak"""
    out = []
    for i in range(N):
        t = i / N
        s = 0.0
        for n in range(1, H + 1):
            s += math.sin(2.0 * math.pi * n * t) / n
        out.append(s)
    return _normalize(out, peak)


def fourier_square(N, H, peak):
    """sum over odd n <= H of sin(2*pi*n*t)/n — normalized to peak"""
    out = []
    for i in range(N):
        t = i / N
        s = 0.0
        n = 1
        while n <= H:
            s += math.sin(2.0 * math.pi * n * t) / n
            n += 2
        out.append(s)
    return _normalize(out, peak)


def fourier_triangle(N, H, peak):
    """sum over odd n <= H of (-1)^k * sin(2*pi*n*t)/n^2 — normalized to peak"""
    out = []
    for i in range(N):
        t = i / N
        s = 0.0
        n = 1
        sign = 1
        while n <= H:
            s += sign * math.sin(2.0 * math.pi * n * t) / (n * n)
            sign = -sign
            n += 2
        out.append(s)
    return _normalize(out, peak)


def clamp16(samples):
    return [max(-32768, min(32767, s)) for s in samples]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-o", "--output", default="sw/bin/synth_basics_32k.bin")
    ap.add_argument("-H", "--harmonics", type=int, default=DEFAULT_HARMONICS,
                    help="Max harmonic for band-limited saw/square/triangle. "
                         "0 = naive (discontinuous). Default: %(default)s")
    args = ap.parse_args()

    if args.harmonics == 0:
        slots = [
            ("sine",     sine),
            ("saw",      naive_saw),
            ("square",   naive_square),
            ("triangle", naive_triangle),
        ]
        mode = "naive"
    else:
        slots = [
            ("sine",     sine),
            ("saw",      fourier_saw),
            ("square",   fourier_square),
            ("triangle", fourier_triangle),
        ]
        mode = "band-limited (H={})".format(args.harmonics)

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    assert len(slots) == NUM_SLOTS, "slot count mismatch"

    print("mode: {}".format(mode))
    with out_path.open("wb") as f:
        for slot, (name, gen) in enumerate(slots):
            samples = clamp16(gen(TABLE_SIZE, args.harmonics, PEAK))
            assert len(samples) == TABLE_SIZE
            f.write(struct.pack("<{}h".format(TABLE_SIZE), *samples))
            print("slot {}: {} ({} samples, peak {})".format(
                slot, name, TABLE_SIZE, max(abs(s) for s in samples)))

    print("Wrote {} ({} bytes)".format(out_path, out_path.stat().st_size))


if __name__ == "__main__":
    main()
