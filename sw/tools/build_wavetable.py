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


SLOT_SIZE = 32768
MAX_SLOTS = 4
TARGET_RATE = 48000
DEFAULT_PEAK = 0x4000  # half-scale; one bit of polyphony headroom against the mix saturator


ap = argparse.ArgumentParser(description="takes in newline seperated text file of wavs and converts into a binary")
ap.add_argument("manifest")
ap.add_argument("-o", "--output", default="wavetable.bin")
ap.add_argument("--peak", type=lambda s: int(s, 0), default=DEFAULT_PEAK,
                help="per-slot normalized peak amplitude (default 0x%X = half-scale)" % DEFAULT_PEAK)

args = ap.parse_args()


def normalize_to_peak(frames, target_peak):
    cur_peak = max((abs(s) for s in frames), default=0)
    if cur_peak == 0:
        return frames
    scale = target_peak / cur_peak
    return [max(-32768, min(32767, int(round(s * scale)))) for s in frames]


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
        src_peak = max((abs(s) for s in raw), default=0)
        scaled = normalize_to_peak(raw, args.peak)
        if len(scaled) < SLOT_SIZE:
            scaled = scaled + [0] * (SLOT_SIZE - len(scaled))
        else:
            scaled = scaled[:SLOT_SIZE]
        out.write(struct.pack("<{}h".format(SLOT_SIZE), *scaled))
        print("slot {}: {} ({} Hz, {} samples, src peak {} -> {})".format(
            slot, p, rate, len(raw), src_peak, args.peak))

print("Wrote {} slot(s) to {}".format(len(paths),args.output))
