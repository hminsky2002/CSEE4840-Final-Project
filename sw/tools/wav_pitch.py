"""
Estimate the fundamental pitch of a mono WAV and recommend a base_step
value for the wavetable synth (assuming resolution=0, TABLE_SIZE=32768,
output rate 48 kHz).

Usage:
    python3 tools/wav_pitch.py path/to/foo.wav
    python3 tools/wav_pitch.py wavs/03.wav --target-midi 60
"""

import argparse
import math
import wave
import numpy as np

TABLE_SIZE = 32768
OUTPUT_RATE = 48000


def load_mono_float(path):
    with wave.open(str(path), "rb") as w:
        n = w.getnframes()
        rate = w.getframerate()
        ch = w.getnchannels()
        sw_width = w.getsampwidth()
        raw = w.readframes(n)
    if sw_width == 1:
        arr = (np.frombuffer(raw, dtype=np.uint8).astype(np.float64) - 128.0) * 256.0
    elif sw_width == 2:
        arr = np.frombuffer(raw, dtype="<i2").astype(np.float64)
    else:
        raise SystemExit(f"unsupported sample width: {sw_width}")
    if ch == 2:
        arr = (arr[0::2] + arr[1::2]) * 0.5
    return arr, rate, sw_width * 8


def detect_fundamental(samples, rate, fmin=60.0, fmax=4000.0):
    """Return (best_freq, top_peaks) via Harmonic Product Spectrum on a centered window."""
    n = len(samples)
    half_win = min(8192, n // 2)
    mid = n // 2
    seg = samples[max(0, mid - half_win):mid + half_win]
    window = np.hanning(len(seg))
    mag = np.abs(np.fft.rfft(seg * window))
    freqs = np.fft.rfftfreq(len(seg), 1.0 / rate)
    fmax = min(fmax, rate / 2 * 0.9)
    band = (freqs >= fmin) & (freqs <= fmax)

    hps = mag.copy()
    for h in range(2, 6):
        dec = mag[::h]
        hps[:len(dec)] = hps[:len(dec)] * dec
    best_freq = freqs[np.argmax(np.where(band, hps, 0))]

    band_idx = np.where(band)[0]
    mag_band = mag[band_idx]
    peaks = []
    for i in range(2, len(mag_band) - 2):
        if (mag_band[i] > mag_band[i - 1] and mag_band[i] > mag_band[i + 1]
                and mag_band[i] > mag_band[i - 2] and mag_band[i] > mag_band[i + 2]):
            peaks.append((mag_band[i], freqs[band_idx[i]]))
    peaks.sort(reverse=True)
    return best_freq, peaks[:6]


def freq_to_midi(f):
    return 69 + 12 * math.log2(f / 440)


def midi_to_note(m):
    names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    n = int(round(m))
    return f"{names[n % 12]}{n // 12 - 1}"


def recommend_base_step(n_in, rate):
    """
    base_step (resolution=0) that plays the slot at the recording's native time-rate
    when pressed at the target MIDI note.

    Build pipeline always resamples to exactly TABLE_SIZE samples, so the audio
    fills the slot with no silence gap regardless of source length:
        effective_stored_rate = TABLE_SIZE / duration
        base_step = effective_stored_rate * 512 / OUTPUT_RATE
    """
    duration = n_in / rate
    eff_rate = TABLE_SIZE / duration
    return eff_rate, round(eff_rate * 512 / OUTPUT_RATE)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("wav")
    ap.add_argument("--target-midi", type=int, default=60,
                    help="MIDI note that should play the recording at its natural rate")
    args = ap.parse_args()

    samples, rate, bits = load_mono_float(args.wav)
    n = len(samples)
    duration = n / rate
    print(f"file: {args.wav}")
    print(f"  {n} samples, {rate} Hz, {bits}-bit, {duration:.3f} sec")
    if n != TABLE_SIZE:
        ratio = n / TABLE_SIZE
        direction = "down" if n > TABLE_SIZE else "up"
        print(f"  >> will be resampled {direction} to {TABLE_SIZE} (ratio {ratio:.3f}, "
              f"effective stored rate {TABLE_SIZE / duration:.0f} Hz)")
    else:
        print("  >> fits TABLE_SIZE exactly")

    best, peaks = detect_fundamental(samples, rate)
    midi_b = freq_to_midi(best)
    cents = (midi_b - round(midi_b)) * 100
    print(f"\nfundamental (HPS): {best:.2f} Hz  =  MIDI {midi_b:.2f}  "
          f"({midi_to_note(midi_b)} {cents:+.0f}c)")
    print("top spectral peaks (musical band):")
    for m, f in peaks:
        midi = freq_to_midi(f)
        print(f"  {f:7.2f} Hz  MIDI {midi:5.2f}  {midi_to_note(midi)}  (mag {m:.0f})")

    eff_rate, bs_recorded = recommend_base_step(n, rate)
    print(f"\nrecommended base_step values (resolution=0):")
    print(f"  {bs_recorded:4d}  MIDI {args.target_midi} plays at recorded rate "
          f"(sample-as-is, root-key sampler style)")

    detected_midi = round(midi_b)
    factor_align = 2 ** ((args.target_midi - detected_midi) / 12)
    bs_align = round(bs_recorded * factor_align)
    print(f"  {bs_align:4d}  recording's {midi_to_note(midi_b)} (~MIDI {detected_midi}) "
          f"sounds when MIDI {args.target_midi} is pressed (keyboard-aligned)")

    factor_to_c4 = 261.6256 / best
    bs_c4 = round(bs_recorded * factor_to_c4)
    print(f"  {bs_c4:4d}  MIDI {args.target_midi} plays the detected fundamental at C4 "
          f"(261.6 Hz)")

    bump = bs_recorded * (2 ** (1 / 12) - 1)
    print(f"\nsemitone bump at base_step={bs_recorded}: ~{bump:.1f}  "
          f"({'clean' if bump > 1.5 else 'chunky'} tracking near MIDI {args.target_midi})")
    print(f"\nsuggested manifest line:")
    print(f"  wavs/{args.wav.split('/')[-1]},{bs_recorded}")


if __name__ == "__main__":
    main()
