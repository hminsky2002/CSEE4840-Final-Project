import argparse
import struct
import sys
import wave
from pathlib import Path


def load_wav_mono_i16(path):
    """Return (mono int16 samples, source sample rate) from a PCM .wav file"""
    with wave.open(str(path), "rb") as w:
        channels = w.getnchannels()
        samp_width = w.getsampwidth()
        rate = w.getframerate()
        n_frames = w.getnframes()
        raw = w.readframes(n_frames)

    if channels not in (1, 2):
        raise ValueError("unsupported channel count ")
    if samp_width not in (1, 2):
        raise ValueError("Unsupposrted sample width (must be 8 or 16 bit)")

    if samp_width == 2:
        count = len(raw) // 2
        frames = list(struct.unpack("<{}h".format(count), raw))
    else:
        frames = [(b - 128) << 8 for b in raw]

    if channels == 2:
        frames = [(frames[i] + frames[i + 1]) // 2 for i in range(0, len(frames), 2)]

    return frames, rate


def resample_linear(samples, target_n):
    """Resample to exactly target_n samples via linear interpolation."""
    n_in = len(samples)
    if n_in == target_n:
        return list(samples)
    out = [0] * target_n
    scale = (n_in - 1) / (target_n - 1) if target_n > 1 else 0
    for i in range(target_n):
        src = i * scale
        lo = int(src)
        hi = min(lo + 1, n_in - 1)
        frac = src - lo
        v = samples[lo] * (1.0 - frac) + samples[hi] * frac
        if v > 32767:
            v = 32767
        elif v < -32768:
            v = -32768
        out[i] = int(v)
    return out


TABLE_SIZE = 32768
MAX_SLOTS = 4
TARGET_RATE = 48000
DEFAULT_BASE_STEP = 357
DEFAULT_RESOLUTION = 8


ap = argparse.ArgumentParser(
    description="takes in newline seperated text file of wavs and converts into a binary"
)
ap.add_argument("manifest")
ap.add_argument("-o", "--output", default="wavetable.bin")

args = ap.parse_args()

# Header format (24 bytes for NUM_TABLE_SLOTS=4):
#   4B  magic "WTSY"
#   2B  version (=1)
#   2B  num_metadata_slots
#   per metadata slot (4B each):
#       2B  base_step
#       1B  resolution
#       1B  reserved (=0)
HEADER_MAGIC = b"WTSY"
HEADER_VERSION = 1


manifest_path = args.manifest

base = Path(manifest_path).resolve().parent

entries = []  # list of (path, base_step, resolution, path_str)

with open(manifest_path, "r") as f:
    for line_no, raw_line in enumerate(f, start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split(",")]
        path_str = parts[0]
        if len(parts) >= 2 and parts[1] != "":
            try:
                bs = int(parts[1])
            except ValueError:
                print(
                    "samples.txt line {}: invalid base_step {!r}".format(
                        line_no, parts[1]
                    )
                )
                sys.exit(1)
        else:
            bs = DEFAULT_BASE_STEP
        if len(parts) >= 3 and parts[2] != "":
            try:
                res = int(parts[2])
            except ValueError:
                print(
                    "samples.txt line {}: invalid resolution {!r}".format(
                        line_no, parts[2]
                    )
                )
                sys.exit(1)
        else:
            res = DEFAULT_RESOLUTION
        p = Path(path_str)
        if not p.is_absolute():
            p = base / p
        entries.append((p, bs, res, path_str))

if not entries:
    print("No wav files found you dope")
    sys.exit(1)

if len(entries) > MAX_SLOTS:
    print("Too many wavs put in you dope")
    sys.exit(1)


with open(args.output, "wb") as out:
    # Header (24 bytes for MAX_SLOTS=4)
    out.write(HEADER_MAGIC)
    out.write(struct.pack("<HH", HEADER_VERSION, MAX_SLOTS))
    for i in range(MAX_SLOTS):
        if i < len(entries):
            _, bs, res, _ = entries[i]
        else:
            bs = DEFAULT_BASE_STEP
            res = DEFAULT_RESOLUTION
        out.write(struct.pack("<HBB", bs, res, 0))

    # Audio: one TABLE_SIZE int16 block per slot listed in samples.txt
    for slot, (p, base_step, resolution, path_str) in enumerate(entries):
        raw, rate = load_wav_mono_i16(p)
        n_in = len(raw)
        if n_in != TABLE_SIZE:
            ratio = n_in / TABLE_SIZE
            effective_rate = int(round(rate * TABLE_SIZE / n_in))
            direction = "down" if n_in > TABLE_SIZE else "up"
            print(
                "NOTE: slot {} ({}): resampled {} {} -> {} samples "
                "(ratio {:.3f}, effective rate {} Hz)".format(
                    slot, path_str, n_in, direction, TABLE_SIZE, ratio, effective_rate
                )
            )
            stored = resample_linear(raw, TABLE_SIZE)
        else:
            stored = list(raw)
        out.write(struct.pack("<{}h".format(TABLE_SIZE), *stored))
        print(
            "slot {}: {} ({} Hz, {} samples in, base_step={}, resolution={})".format(
                slot, p, rate, n_in, base_step, resolution
            )
        )

print("Wrote {} slot(s) to {} (header + {} audio block(s))".format(
    len(entries), args.output, len(entries)
))
