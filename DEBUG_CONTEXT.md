# Audio debug context — `sw-testbenching` branch

Context dump from Claude Code session. Resume from here.

## Problem

Running `test_bringup` on `sw-testbenching` produced clipping/distortion instead of the expected A4 sawtooth. HPS writes *were* getting through (sound started when test started, stopped when test stopped) and the codec path was known-good from prior work. Bug was in sample values, not the Avalon-ST plumbing.

## Branch landscape

- `main` — simple square wave, irrelevant.
- `load-wavetable-in` — **previously-working architecture**: single-file `wave_table_synth.sv`, 8 voices, 12 slots × 4096 samples, explicit `S_READ → S_WAIT → S_ACC` state machine gated by `ready_left & ready_right`. Full MIDI userspace stack in `sw/midi_to_fpga.{c,h}`, `sw/midi.{c,h}`, `sw/wavetable.{c,h}` — voice allocator, `note_to_step_size`, wavetable loader. Uses `/dev/mem` mmap (no kernel driver).
- `sw-testbenching` — current (broken): split into `wave_table_synth.sv` + `oscillator.sv`, 32 voices TDM, 16 slots × 2048 samples, free-running 48 kHz timer with `sample_pending` backpressure. New kernel driver `sw/driver/wave_synth.c` (mmap-only, no ioctl). `test_bringup.c` replaces MIDI app.

Divergence point: `9d52509` (merge of `harry-top-level-module`). 11 commits on `load-wavetable-in`, 30 on `sw-testbenching`.

## Root cause — identified

**`oscillator.sv` BRAM-read pipeline was off by one cycle.**

BRAM has 1-cycle synchronous read latency (`bram_rdata <= mem[bram_raddr]`). Old code tried to pipeline tightly:

```systemverilog
// OLD (buggy)
wire [4:0] prev_v = step[4:0] - 5'd1;
...
if (step > 6'd0) begin
    voice_idx   <= prev_v;
    voice_sample <= (ctrl[prev_v]==2'b10) ? bram_rdata : 16'h0;
    ...
end
```

`bram_rdata` is a **register**, so at cycle T's posedge the pre-edge value is `mem[bram_raddr pre-(T-1)]` — one cycle behind what you just presented. Result: every latch read one-cycle-stale data. Meanwhile the correct sample arrived one cycle later but was gated off because `voice_idx` had already advanced.

### Trace (single-voice test, sawtooth slot 0, `mem[0] = -32768`)

| cycle | step_pre | bram_rdata pre | voice_sample ← | voice_idx | ctrl gate | mix_acc gets |
|-------|----------|----------------|----------------|-----------|-----------|--------------|
| T=3   | 1 | mem[0] stale | mem[0] | 0 | ctrl[0]=2'b10 ✓ | **−32768** |
| T=4   | 2 | voice_0_sample ✓ | voice_0_sample | 1 | ctrl[1]=2'b00 ✗ | **0** |
| T=5…34| 3…32 | … | 0 | 2…31 | ctrl=0 | 0 |

Net: every output = constant `mem[0] = −32768` regardless of phase. DC rail → AC-coupled DAC produces the harsh artifacts heard during the test.

**Why `load-wavetable-in` didn't hit this**: its explicit `S_WAIT` state sits between `S_READ` and `S_ACC` — literally a NOP cycle that absorbs the BRAM latency. Structurally impossible to forget.

## Fix applied

`hw/quartus/wave-table-synth/oscillator.sv`:

- `prev_v = step - 1` → `consume_v = step - 2` (5-bit wrap: step=32 → 30, step=33 → 31)
- Consume guard: `if (step > 0)` → `if (step > 1)` (skip 2 cycles while BRAM latency clears)
- Sweep end: `if (step == 32)` → `if (step == 33)` (one extra cycle so voice 31 latches)

Sweep now runs 34 cycles (was 33). Still << 1041-cycle sample window, plenty of slack.

### Post-fix trace

| cycle | step_pre | bram_rdata pre | voice_sample ← | voice_idx |
|-------|----------|----------------|----------------|-----------|
| T=4 | 2 | voice_0_sample ✓ | voice_0_sample | 0 ✓ |
| T=5 | 3 | voice_1_sample | 0 (ctrl[1]=00) | 1 |
| … | … | … | … | … |
| T=35 | 33 | voice_31_sample | 0 (ctrl[31]=00) | 31 |
| T=35 | — | — | — | `sweep_done <= 1` |

On sweep 2 voice 0's phase has advanced to 153856 → addr 18 → mem[18] = sawtooth[18] = −32192 latches correctly. Subsequent sweeps produce a proper 440 Hz sawtooth.

## Verification checklist

1. Recompile in Quartus. Module port list is unchanged — no Qsys regen needed.
2. Flash and run `sudo ./sw/test_bringup`. Expect: clean A4 sawtooth that starts on `CTRL_START`, stops on `CTRL_STOP`.
3. If still wrong, SignalTap inside `soc_system0.wave_table_synth_0.u_oscillator`:
   - Signals: `voice_sample`, `voice_idx`, `voice_valid`, `bram_rdata`, `bram_raddr`, `step`, `phase[0]`, `mix_acc`, `sample`, `sweep_done`
   - Trigger: `sweep_done == 1`, depth 64
   - Check: at each `voice_valid` posedge, `voice_sample` should equal `mem[{table_sel[voice_idx], phase[voice_idx][23:13]}]` (address presented exactly 2 cycles earlier).
4. Multi-voice test: start voice 0 at A4 and voice 5 at E5; confirm both audible.

## Git state at session end

- Branch: `sw-testbenching`
- One uncommitted change: `M hw/quartus/wave-table-synth/oscillator.sv` (the fix — unstaged so you can review before committing).
- Stashes `stash@{0..4}` exist from earlier work; `stash@{0}` is WIP from this branch at an older commit, probably stale.

## Follow-up (non-blocking)

`load-wavetable-in` had a full MIDI → FPGA userspace app that's not ported to `sw-testbenching`. Once audio is confirmed working from `test_bringup`, port these onto the new register layout:

- `sw/midi_to_fpga.{c,h}` — main loop, voice allocator (`allocate_voice`, `find_voice`), `fpga_set_note_on/step_size/velocity/slot_select`, `note_to_step_size`, `load_wavetable`.
- `sw/midi.{c,h}` — USB MIDI parse.
- `sw/wavetable.{c,h}` — table generation (sine/saw/square/etc).

Register layout on `sw-testbenching` (from `test_bringup.c`):
- `OSC_STEP(v)   = 0x8000 + v*4 + 0` — 16-bit step
- `OSC_CTRL(v)   = 0x8000 + v*4 + 1` — 2-bit ctrl (01=stop, 10=start, 11=reset)
- `OSC_TABLE(v)  = 0x8000 + v*4 + 2` — 4-bit slot
- `AMP_CTRL      = 0x8080` — 7-bit (currently unused by mixer)
- Wavetable word offset: `slot * 2048 + sample_index`

Q-format difference from `load-wavetable-in`: now 24-bit phase with step_size left-shifted by 8 (Q11.13 effectively). `note_to_step_size` formula: `step = round(frequency * 65536 / 48000)` — for A4 (440 Hz): 601.

## Plan file

Full plan (if you want to re-read): `/Users/harryminsky/.claude/plans/ok-lets-back-way-transient-nebula.md`
