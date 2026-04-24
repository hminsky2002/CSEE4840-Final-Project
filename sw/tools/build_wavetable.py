#!/usr/bin/env python3
"""
Bake a manifest of .wav files into a single wavetable.bin for the FPGA synth.

Input: a manifest file (one wav path per line, blanks and '#' comments
skipped). Paths are resolved relative to the manifest's directory.

Output: a raw binary file. Each slot is exactly TABLE_SIZE (8192) int16 mono
samples, little-endian. Slots are concatenated in manifest order. Input wavs
are converted to mono (stereo averaged) and truncated or zero-padded to
TABLE_SIZE samples. 16-bit and 8-bit unsigned PCM sources are supported.

Usage:
    python3 build_wavetable.py samples.txt -o wavetable.bin

Python 3.5 compatible.
"""
import argparse
import struct
import sys
import wave
from pathlib import Path

TABLE_SIZE = 8192          # must match fpga_bridge.h
MAX_SLOTS  = 4             # must match MAX_WAVETABLE_SLOTS in wavetable.h


def load_wav_mono_i16(path):
    """Return a list of int16 mono samples from a PCM .wav file."""
    with wave.open(str(path), "rb") as w:
        channels   = w.getnchannels()
        samp_width = w.getsampwidth()
        n_frames   = w.getnframes()
        raw        = w.readframes(n_frames)

    if channels not in (1, 2):
        raise ValueError("{}: unsupported channel count {}".format(path, channels))
    if samp_width not in (1, 2):
        raise ValueError("{}: unsupported sample width {} bytes".format(path, samp_width))

    if samp_width == 2:
        # Signed 16-bit LE, interleaved by channel.
        count = len(raw) // 2
        frames = list(struct.unpack("<{}h".format(count), raw))
    else:
        # 8-bit WAV PCM is UNSIGNED (128 = silence). Shift into int16 range.
        frames = [(b - 128) << 8 for b in raw]

    if channels == 1:
        return frames
    # Stereo: average L/R pairs down to mono.
    return [(frames[i] + frames[i + 1]) // 2 for i in range(0, len(frames), 2)]


def fit_to_table(samples):
    """Truncate or zero-pad to exactly TABLE_SIZE samples."""
    if len(samples) >= TABLE_SIZE:
        return samples[:TABLE_SIZE]
    return samples + [0] * (TABLE_SIZE - len(samples))


def read_manifest(manifest_path):
    base = Path(manifest_path).resolve().parent
    entries = []
    with open(manifest_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            p = Path(line)
            if not p.is_absolute():
                p = base / p
            entries.append(p)
    return entries


def main():
    ap = argparse.ArgumentParser(description="Bake .wav manifest into wavetable.bin")
    ap.add_argument("manifest", help="Manifest file: one wav path per line")
    ap.add_argument("-o", "--output", default="wavetable.bin",
                    help="Output binary path (default: wavetable.bin)")
    args = ap.parse_args()

    paths = read_manifest(args.manifest)
    if not paths:
        print("No wav entries in {}".format(args.manifest), file=sys.stderr)
        return 1
    if len(paths) > MAX_SLOTS:
        print("Warning: {} entries, only first {} will be used".format(
            len(paths), MAX_SLOTS), file=sys.stderr)
        paths = paths[:MAX_SLOTS]

    with open(args.output, "wb") as out:
        for slot, p in enumerate(paths):
            samples = fit_to_table(load_wav_mono_i16(p))
            out.write(struct.pack("<{}h".format(TABLE_SIZE), *samples))
            print("slot {}: {}".format(slot, p))

    print("Wrote {} slot(s) to {}".format(len(paths), args.output))
    return 0


if __name__ == "__main__":
    sys.exit(main())
