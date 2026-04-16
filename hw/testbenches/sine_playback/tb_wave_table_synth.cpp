#include "Vwave_table_synth.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <verilated.h>
#include <verilated_vcd_c.h>

vluint64_t sim_time = 0;
int test_pass = 0;
int test_fail = 0;

#define TABLE_SIZE 4096
#define WT_ADDR(slot, idx) ((1 << 17) | ((slot) << 12) | (idx))
#define CTRL_ADDR(voice, reg) (((voice) << 2) | (reg))

#define REG_NOTE_ON    0
#define REG_STEP_SIZE  1
#define REG_VELOCITY   2
#define REG_SLOT_SEL   3

static int16_t sine_table[TABLE_SIZE];

void generate_sine(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        double t = (double)i / TABLE_SIZE;
        sine_table[i] = (int16_t)(sin(2.0 * M_PI * t) * 32767.0);
    }
}

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
        printf("FAIL: %s — expected %d, got %d\n", name, expected, actual);
    }
}

void check_near(const char *name, int expected, int actual, int tolerance) {
    int diff = expected - actual;
    if (diff < 0) diff = -diff;
    if (diff <= tolerance) {
        test_pass++;
    } else {
        test_fail++;
        printf("FAIL: %s — expected %d ±%d, got %d (off by %d)\n",
               name, expected, tolerance, actual, diff);
    }
}

int16_t expected_output(int16_t raw, uint8_t vel) {
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

    generate_sine();
    do_reset(dut, trace);

    // ── Load full 4096-sample sine into slot 0 ────────────────
    printf("Loading 4096-sample sine wave into slot 0...\n");
    for (int i = 0; i < TABLE_SIZE; i++)
        avalon_write(dut, trace, WT_ADDR(0, i), (uint16_t)sine_table[i]);
    printf("Done.\n");

    // ── Test 1: Verify BRAM contents match what was written ───
    printf("Test 1: Readback verification (spot check 32 samples)...\n");
    for (int i = 0; i < TABLE_SIZE; i += 128) {
        dut->chipselect = 1;
        dut->write = 0;
        dut->address = WT_ADDR(0, i);
        tick(dut, trace);
        uint16_t val = dut->readdata;
        dut->chipselect = 0;
        char msg[64];
        snprintf(msg, sizeof(msg), "readback[%d]", i);
        check(msg, (int)(uint16_t)sine_table[i], (int)val);
    }

    // ── Test 2: Play back sine, verify output tracks table ────
    printf("Test 2: DDS playback matches sine table...\n");

    // step=256 → index advances by 1 each tick (256 >> 8 = 1)
    // vel=127 → near-unity scaling
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  0);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int valid;
    for (int t = 0; t < 64; t++) {
        int16_t out = trigger_sample_tick(dut, trace, &valid);
        int16_t exp = expected_output(sine_table[t], 127);
        char msg[64];
        snprintf(msg, sizeof(msg), "sine_tick%d", t);
        check(msg, exp, out);
    }

    // ── Test 3: Sine shape sanity ─────────────────────────────
    printf("Test 3: Sine wave shape validation...\n");

    // Reset phase: turn off, wait, turn back on
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON), 0);
    for (int i = 0; i < 4; i++) tick(dut, trace);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    // Collect one full cycle (4096 ticks with step=256 → index 0..4095)
    int16_t samples[4096];
    for (int t = 0; t < 4096; t++)
        samples[t] = trigger_sample_tick(dut, trace, &valid);

    // Check: sample[0] near zero (sine starts at 0)
    check_near("sine_start_zero", 0, samples[0], 2);

    // Check: sample[1024] near positive peak (quarter cycle)
    int16_t peak_exp = expected_output(32767, 127);
    check_near("sine_quarter_peak", peak_exp, samples[1024], 4);

    // Check: sample[2048] near zero (half cycle)
    check_near("sine_half_zero", 0, samples[2048], 2);

    // Check: sample[3072] near negative peak (three-quarter)
    int16_t neg_peak_exp = expected_output(-32768, 127);
    check_near("sine_3q_trough", neg_peak_exp, samples[3072], 4);

    // Check: output is monotonically increasing from 0 to peak (first quarter)
    int mono_ok = 1;
    for (int t = 1; t <= 1024; t++) {
        if (samples[t] < samples[t - 1]) {
            mono_ok = 0;
            printf("  monotonic break at tick %d: %d -> %d\n",
                   t, samples[t-1], samples[t]);
            break;
        }
    }
    if (mono_ok) test_pass++;
    else test_fail++;

    // ── Results ───────────────────────────────────────────────
    printf("\n=== Results: %d passed, %d failed ===\n", test_pass, test_fail);

    trace->close();
    delete dut;
    return test_fail > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
