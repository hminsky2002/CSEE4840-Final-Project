import argparse
import struct
import sys
import wave
from pathlib import Path 

def load_wav_mono_i16(path):
    """"Return (mono int16 samples), source sample rate from a PCM .wav file"""
    with wave.open(str(path),"rb") as w:
        channels = w.getnchannels()
        samp_width = w.getsampwidth()
        rate = w.getframerate()
        n_frames = w.getnframes()
        raw = w.readframes(n_frames)

    if channels not in (1,2):
        raise ValueError("unsupported channel count ")
    if samp_width not in (1,2):
        raise ValueError("Unsupposrted sample width (must be 8 or 16 bit)")
    
    if samp_width == 2:
        count = len(raw) // 2
        frames = list(struct.unpack("<{}h".format(count),raw))
    else:
        frames = [(b-128) << 8 for b in raw]
    
    if channels == 2:
        frames = [(frames[i] + frames[i+1]) // 2 for i in range(0,len(frames),2)]
    
    return frames, rate 


TABLE_SIZE = 8192
MAX_SLOTS = 4
TARGET_RATE = 48000


def resample_to_table_size(samples):
    """Linear-interpolate a single-cycle waveform to exactly TABLE_SIZE samples.
    Treats input as one cycle so output is one cycle at the wavetable's resolution
    (preserves perceived pitch vs. an already-TABLE_SIZE wav)."""
    n = len(samples)
    if n == TABLE_SIZE:
        return samples
    out = [0] * TABLE_SIZE
    for i in range(TABLE_SIZE):
        pos = i * n / TABLE_SIZE
        i0 = int(pos)
        frac = pos - i0
        i1 = (i0 + 1) % n
        v = samples[i0] * (1.0 - frac) + samples[i1] * frac
        if v > 32767: v = 32767
        elif v < -32768: v = -32768
        out[i] = int(v)
    return out


ap = argparse.ArgumentParser(description="takes in newline seperated text file of wavs and converts into a binary")
ap.add_argument("manifest")
ap.add_argument("-o", "--output", default="wavetable.bin")

args = ap.parse_args()


manifest_path = args.manifest 

base = Path(manifest_path).resolve().parent 

paths = []

with open(manifest_path, "r") as f:
    for line in f:
        line = line.strip()
        p = Path(line)
        if not p.is_absolute():
            p = base / p 
        paths.append(p)

if not paths:
     print("No wav files found you dope")
     exit()

if len(paths) > MAX_SLOTS:
    print("Too many wavs put in you dope")
    exit() 

with open(args.output, "wb") as out:
    for slot, p in enumerate(paths):
        raw, rate = load_wav_mono_i16(p)
        n_in = len(raw)
        if n_in != TABLE_SIZE:
            raw = resample_to_table_size(raw)
            print("slot {}: {} ({} Hz, {} -> {} samples, resampled)".format(slot,p,rate,n_in,TABLE_SIZE))
        else:
            print("slot {}: {} ({} Hz, {} samples)".format(slot,p,rate,n_in))
        out.write(struct.pack("<{}h".format(TABLE_SIZE), *raw))

print("Wrote {} slot(s) to {}".format(len(paths),args.output))
