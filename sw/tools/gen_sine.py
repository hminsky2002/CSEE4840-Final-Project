#!/usr/bin/env python3
"""
Write a single-cycle sine wave as int16 mono 48kHz .wav, sized to fit one
wavetable slot exactly (TABLE_SIZE = 8192 samples = 16KB).

Usage:
    python3 gen_sine.py [output.wav]   # default: sine_8192.wav
"""
import math
import struct
import sys
import wave

TABLE_SIZE  = 8192
SAMPLE_RATE = 48000
AMPLITUDE   = 32000   # leave a little headroom below INT16_MAX


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "sine_8192.wav"
    samples = [int(round(AMPLITUDE * math.sin(2.0 * math.pi * n / TABLE_SIZE)))
               for n in range(TABLE_SIZE)]
    raw = struct.pack("<{}h".format(TABLE_SIZE), *samples)
    with wave.open(out, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SAMPLE_RATE)
        w.writeframes(raw)
    print("Wrote {} ({} samples @ {} Hz)".format(out, TABLE_SIZE, SAMPLE_RATE))


if __name__ == "__main__":
    main()
