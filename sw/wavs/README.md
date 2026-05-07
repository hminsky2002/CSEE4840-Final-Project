# Wavetable library

Single-cycle waveforms used by the synth. Each `.wav` is mono 16-bit PCM. Files
under 8192 samples are linear-interpolated up to `TABLE_SIZE` (8192) by
`tools/build_wavetable.py` so that one file = one wavetable cycle, regardless
of source length. AKWF source files are 600 samples @ 44.1 kHz; `saw_8192.wav`
is already 8192 samples and passes through untouched.

## Available waveforms

Filename convention: `<short-description>_<3-char-display-label>.wav`. The
display label uses only segments the DE1-SoC seven-segment table supports
(`0-9 A B C D E F G I N Q R S T U`) so it can be shown verbatim on HEX2..HEX0.

| File                | Label | Source                     | Character                         |
|---------------------|-------|----------------------------|-----------------------------------|
| `saw_8192.wav`      | `SAU` | (built-in)                 | classic sawtooth, full 8192-cycle |
| `vocal_aaa.wav`     | `AAA` | `AKWF_hvoice_0010`         | vowel formant, vocal-like         |
| `bell_din.wav`      | `DIN` | `AKWF_fmsynth_0030`        | FM bell, inharmonic ringing       |
| `pluck_str.wav`     | `STR` | `AKWF_pluckalgo_0005`      | plucked-string snapshot           |
| `weird_bad.wav`     | `BAD` | `AKWF_hdrawn_0020`         | hand-drawn, irregular harmonics   |
| `theremin_eer.wav`  | `EER` | `AKWF_theremin_0010`       | smooth eerie sine-ish wail        |
| `fuzz_dis.wav`      | `DIS` | `AKWF_distorted_0040`      | clipped, gritty                   |
| `birds_brd.wav`     | `BRD` | `AKWF_birds_0010`          | chirpy, organic                   |
| `grain_dus.wav`     | `DUS` | `AKWF_granular_0020`       | textural, noise-like              |
| `arcade_arc.wav`    | `ARC` | `AKWF_vgame_0010`          | chiptune square-derivative        |
| `organ_air.wav`     | `AIR` | `AKWF_eorgan_0020`         | electronic organ                  |
| `drone_dub.wav`     | `DUB` | `AKWF_dbass_0010`          | sub-heavy bass                    |
| `fiddle_fid.wav`    | `FID` | `AKWF_violin_0010`         | bowed violin                      |

AKWF (Adventure Kid Waveforms) source: <https://github.com/KristofferKarlAxelEkstrand/AKWF-FREE>

## Building a wavetable bin

The bin packs up to `MAX_SLOTS` (4) wavs into a single file the device loads
at boot. List the wavs you want — in slot order — in `sw/samples.txt`, then:

```sh
python3 tools/build_wavetable.py samples.txt -o bin/akwf.bin
```

The current `samples.txt` selects the four most timbrally distinct picks
(`vocal_aaa`, `bell_din`, `pluck_str`, `weird_bad`). Swap any of the lines
for another file in this directory to change a slot.

## Updating the on-device labels

If you swap a wav in `samples.txt`, also update the corresponding entry in
`TABLE_NAME[]` near the top of `sw/midi_to_fpga.c` so HEX2..HEX0 match. Each
entry uses three `SEG_*` macros from `fpga_bridge.h` — pick three letters from
the 3-char label column above (or compose your own from the supported alphabet).

## Adding new waveforms

1. Drop a mono PCM `.wav` into this directory. Any sample rate / length is
   fine — the build script will mono-mix and resample.
2. Pick a 3-char display label using only `0-9 A B C D E F G I N Q R S T U`.
3. Rename the file `<short>_<label>.wav` to keep the convention.
4. Add it to `samples.txt` (replacing one of the four existing lines) and to
   `TABLE_NAME[]` in `midi_to_fpga.c`.
