#include "Vwave_table_synth.h"
#include <cstdio>
#include <cstdlib>
#include <verilated.h>
#include <verilated_vcd_c.h>

vluint64_t sim_time = 0;
int test_pass = 0;
int test_fail = 0;

#define WT_ADDR(slot, idx) ((1 << 17) | ((slot) << 12) | (idx))
#define CTRL_ADDR(voice, reg) (((voice) << 2) | (reg))

#define REG_NOTE_ON    0
#define REG_STEP_SIZE  1
#define REG_VELOCITY   2
#define REG_SLOT_SEL   3

void tick(Vwave_table_synth *dut, VerilatedVcdC *trace) {
    dut->clk = 0; dut->eval(); trace->dump(sim_time++);
    dut->clk = 1; dut->eval(); trace->dump(sim_time++);
}

void avalon_write(Vwave_table_synth *dut, VerilatedVcdC *trace,
                  uint32_t addr, uint16_t data) {
    dut->chipselect = 1;
    dut->write = 1;
    dut->address = addr;
    dut->writedata = data;
    tick(dut, trace);
    dut->chipselect = 0;
    dut->write = 0;
}

void do_reset(Vwave_table_synth *dut, VerilatedVcdC *trace) {
    dut->reset = 1;
    dut->chipselect = 0;
    dut->write = 0;
    dut->address = 0;
    dut->writedata = 0;
    dut->ready_left = 0;
    dut->ready_right = 0;
    for (int i = 0; i < 4; i++) tick(dut, trace);
    dut->reset = 0;
    tick(dut, trace);
}

int16_t trigger_sample_tick(Vwave_table_synth *dut, VerilatedVcdC *trace,
                            int *valid) {
    *valid = 0;
    int16_t result = 0;
    dut->ready_left = 1;
    dut->ready_right = 1;
    tick(dut, trace);
    dut->ready_left = 0;
    dut->ready_right = 0;
    for (int i = 0; i < 40; i++) {
        tick(dut, trace);
        if (dut->sample_valid) {
            *valid = 1;
            result = (int16_t)dut->sample;
        }
    }
    return result;
}

void check(const char *name, int expected, int actual) {
    if (expected == actual) {
        test_pass++;
    } else {
        test_fail++;
        printf("FAIL: %s — expected %d (0x%04X), got %d (0x%04X) (t=%lu)\n",
               name, expected, (uint16_t)expected,
               actual, (uint16_t)actual, (unsigned long)sim_time);
    }
}

void all_voices_off(Vwave_table_synth *dut, VerilatedVcdC *trace) {
    for (int v = 0; v < 8; v++)
        avalon_write(dut, trace, CTRL_ADDR(v, REG_NOTE_ON), 0);
    for (int i = 0; i < 4; i++) tick(dut, trace);
}

int16_t expected_single(int16_t raw, uint8_t vel) {
    int32_t product = (int32_t)raw * (int32_t)vel;
    int16_t scaled = (int16_t)((product >> 7) & 0xFFFF);
    return (int16_t)(scaled >> 3);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vwave_table_synth *dut = new Vwave_table_synth;

    Verilated::traceEverOn(true);
    VerilatedVcdC *trace = new VerilatedVcdC;
    dut->trace(trace, 5);
    trace->open("waveform.vcd");

    do_reset(dut, trace);
    int valid;

    // ── Setup: write test patterns into slots ─────────────────
    // Slot 0: constant +16384 (0x4000)
    for (int i = 0; i < 16; i++)
        avalon_write(dut, trace, WT_ADDR(0, i), 0x4000);

    // Slot 1: constant -16384 (0xC000 as uint16 = -16384 signed)
    for (int i = 0; i < 16; i++)
        avalon_write(dut, trace, WT_ADDR(1, i), 0xC000);

    // Slot 2: index-based ramp for wraparound test (sample[i] = i * 8)
    for (int i = 0; i < 4096; i++)
        avalon_write(dut, trace, WT_ADDR(2, i), (uint16_t)(i * 8));

    // ── Test 1: Velocity = 0 silences a voice ─────────────────
    printf("Test 1: Velocity=0 produces silence...\n");
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  0);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  0);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int16_t out = trigger_sample_tick(dut, trace, &valid);
    check("vel0_valid", 1, valid);
    check("vel0_sample", 0, out);
    all_voices_off(dut, trace);

    // ── Test 2: Negative wavetable samples ────────────────────
    printf("Test 2: Negative wavetable samples scale correctly...\n");
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  1);  // slot 1: -16384
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int16_t exp_neg = expected_single(-16384, 127);
    out = trigger_sample_tick(dut, trace, &valid);
    check("neg_valid", 1, valid);
    check("neg_sample", exp_neg, out);
    all_voices_off(dut, trace);

    // ── Test 3: Positive + negative cancel ────────────────────
    printf("Test 3: Positive + negative voices cancel out...\n");
    // Voice 0: slot 0 (+16384), vel=64
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  0);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  64);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);
    // Voice 1: slot 1 (-16384), vel=64
    avalon_write(dut, trace, CTRL_ADDR(1, REG_SLOT_SEL),  1);
    avalon_write(dut, trace, CTRL_ADDR(1, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(1, REG_VELOCITY),  64);
    avalon_write(dut, trace, CTRL_ADDR(1, REG_NOTE_ON),   1);

    out = trigger_sample_tick(dut, trace, &valid);
    check("cancel_valid", 1, valid);
    check("cancel_sample", 0, out);
    all_voices_off(dut, trace);

    // ── Test 4: Phase accumulator resets on note_off ──────────
    printf("Test 4: Phase resets on note_off...\n");
    // Play voice 0 in slot 2 (ramp) for 4 ticks to advance phase
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  2);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int16_t first_sample = trigger_sample_tick(dut, trace, &valid);
    // Advance a few ticks so phase moves forward
    trigger_sample_tick(dut, trace, &valid);
    trigger_sample_tick(dut, trace, &valid);

    // Turn off, wait for reset, turn back on
    all_voices_off(dut, trace);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  2);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int16_t restart_sample = trigger_sample_tick(dut, trace, &valid);
    check("phase_reset_valid", 1, valid);
    check("phase_reset_sample", first_sample, restart_sample);
    all_voices_off(dut, trace);

    // ── Test 5: Phase accumulator wraparound ──────────────────
    printf("Test 5: Phase accumulator wraps around table...\n");
    // Use a large step_size so the accumulator wraps in a few ticks.
    // step=16384 (0x4000): each tick advances index by 16384>>8 = 64.
    // After 64 ticks: index = 64*64 = 4096 → wraps to 0.
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  2);  // ramp
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 16384);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    first_sample = trigger_sample_tick(dut, trace, &valid); // index=0
    // Run 63 more ticks to reach index 4032, then 1 more wraps to index 0
    for (int i = 0; i < 63; i++)
        trigger_sample_tick(dut, trace, &valid);
    int16_t wrap_sample = trigger_sample_tick(dut, trace, &valid); // should be back to index 0
    check("wrap_valid", 1, valid);
    check("wrap_sample_matches_start", first_sample, wrap_sample);
    all_voices_off(dut, trace);

    // ── Test 6: All 8 voices active simultaneously ────────────
    printf("Test 6: All 8 voices active...\n");
    // All voices on slot 0 (constant +16384), vel=127, same step
    for (int v = 0; v < 8; v++) {
        avalon_write(dut, trace, CTRL_ADDR(v, REG_SLOT_SEL),  0);
        avalon_write(dut, trace, CTRL_ADDR(v, REG_STEP_SIZE), 256);
        avalon_write(dut, trace, CTRL_ADDR(v, REG_VELOCITY),  127);
        avalon_write(dut, trace, CTRL_ADDR(v, REG_NOTE_ON),   1);
    }

    // Expected: 8 voices each contributing the same scaled value.
    // Single voice scaled = (16384 * 127) >> 7 = 16256.
    // mix_acc = 16256 * 8 = 130048. sample = 130048 >> 3 = 16256.
    // But mix_acc is 19-bit signed: 130048 = 0x1FC00. [18:3] = 130048 >> 3 = 16256.
    int32_t single_scaled = ((int32_t)16384 * 127) >> 7;
    int16_t exp_all8 = (int16_t)((single_scaled * 8) >> 3);

    out = trigger_sample_tick(dut, trace, &valid);
    check("all8_valid", 1, valid);
    check("all8_sample", exp_all8, out);
    all_voices_off(dut, trace);

    // ── Test 7: Velocity=1 (minimum audible) ──────────────────
    printf("Test 7: Minimum velocity (vel=1)...\n");
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  0);  // +16384
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  1);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int16_t exp_min = expected_single(16384, 1);
    out = trigger_sample_tick(dut, trace, &valid);
    check("minvel_valid", 1, valid);
    check("minvel_sample", exp_min, out);
    all_voices_off(dut, trace);

    // ── Results ─────────────────────────────────────��─────────
    printf("\n=== Results: %d passed, %d failed ===\n", test_pass, test_fail);

    trace->close();
    delete dut;
    return test_fail > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
