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


SLOT_SIZE = 8192
MAX_SLOTS = 4
TARGET_RATE = 48000


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
        out.write(struct.pack("<{}h".format(SLOT_SIZE), *raw))
        print("slot {}: {} ({} Hz, {} samples)".format(slot,p,rate,len(raw)))

print("Wrote {} slot(s) to {}".format(len(paths),args.output))
