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

// Trigger one sample tick and wait for the DDS engine to finish.
// Returns the output sample. Sets *valid = 1 if sample_valid pulsed.
int16_t trigger_sample_tick(Vwave_table_synth *dut, VerilatedVcdC *trace,
                            int *valid) {
    *valid = 0;
    int16_t result = 0;

    // Rising edge on ready signals
    dut->ready_left = 1;
    dut->ready_right = 1;
    tick(dut, trace); // sample_tick fires

    // Deassert ready so we get exactly one tick
    dut->ready_left = 0;
    dut->ready_right = 0;

    // Run enough clocks for the engine: 8 voices x 3 states + overhead
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

// Compute expected velocity-scaled, mixer-divided output for a single voice
int16_t expected_output(int16_t raw_sample, uint8_t velocity) {
    int32_t product = (int32_t)raw_sample * (int32_t)velocity;
    int16_t scaled = (int16_t)(product >> 7);
    // mix_acc[18:3] = divide by 8 (only 1 voice active, others contribute 0)
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

    // ── Setup: write a ramp into slot 0 (sample[i] = i * 256) ──
    // This makes values large enough to survive velocity scaling.
    for (int i = 0; i < 64; i++)
        avalon_write(dut, trace, WT_ADDR(0, i), (uint16_t)(i * 256));

    // ── Test 1: Single voice, constant output ──────────────────
    printf("Test 1: Single voice, constant wavetable value...\n");

    // Write 0x2000 to slot 1 samples 0-15
    for (int i = 0; i < 16; i++)
        avalon_write(dut, trace, WT_ADDR(1, i), 0x2000);

    // Configure voice 0: slot 1, step=256, vel=127, note_on
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  1);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    int16_t exp = expected_output(0x2000, 127);
    int valid;

    for (int tick_num = 0; tick_num < 4; tick_num++) {
        int16_t out = trigger_sample_tick(dut, trace, &valid);
        char msg[64];
        snprintf(msg, sizeof(msg), "const_tick%d_valid", tick_num);
        check(msg, 1, valid);
        snprintf(msg, sizeof(msg), "const_tick%d_sample", tick_num);
        check(msg, exp, out);
    }

    // Turn off voice 0 for next test
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON), 0);
    // Let phase accumulator reset
    for (int i = 0; i < 4; i++) tick(dut, trace);

    // ── Test 2: Phase accumulator walks through ramp ───────────
    printf("Test 2: Phase accumulator walking ramp in slot 0...\n");

    // Configure voice 0: slot 0 (ramp), step=256, vel=127, note_on
    // step=256 means phase_acc advances by 256 each tick.
    // index = phase_acc[19:8], so index advances by 1 each tick.
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  0);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    for (int tick_num = 0; tick_num < 8; tick_num++) {
        int16_t raw_sample = (int16_t)(tick_num * 256);
        int16_t exp_val = expected_output(raw_sample, 127);
        int16_t out = trigger_sample_tick(dut, trace, &valid);
        char msg[64];
        snprintf(msg, sizeof(msg), "ramp_tick%d_valid", tick_num);
        check(msg, 1, valid);
        snprintf(msg, sizeof(msg), "ramp_tick%d_sample", tick_num);
        check(msg, exp_val, out);
    }

    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON), 0);
    for (int i = 0; i < 4; i++) tick(dut, trace);

    // ── Test 3: Two voices mix together ────────────────────────
    printf("Test 3: Two-voice mixing...\n");

    // Voice 0: slot 1 (constant 0x2000), vel=127
    avalon_write(dut, trace, CTRL_ADDR(0, REG_SLOT_SEL),  1);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(0, REG_NOTE_ON),   1);

    // Voice 1: slot 1 (constant 0x2000), vel=127
    avalon_write(dut, trace, CTRL_ADDR(1, REG_SLOT_SEL),  1);
    avalon_write(dut, trace, CTRL_ADDR(1, REG_STEP_SIZE), 256);
    avalon_write(dut, trace, CTRL_ADDR(1, REG_VELOCITY),  127);
    avalon_write(dut, trace, CTRL_ADDR(1, REG_NOTE_ON),   1);

    {
        // Two voices each contributing the same amount → double the single-voice output
        int32_t single_scaled = ((int32_t)0x2000 * 127) >> 7;
        int16_t exp_mix = (int16_t)((single_scaled * 2) >> 3);

        int16_t out = trigger_sample_tick(dut, trace, &valid);
        check("mix2_valid", 1, valid);
        check("mix2_sample", exp_mix, out);
    }

    // ── Test 4: Silent when all voices off ─────────────────────
    printf("Test 4: Silence when all voices off...\n");
    for (int v = 0; v < 8; v++)
        avalon_write(dut, trace, CTRL_ADDR(v, REG_NOTE_ON), 0);
    for (int i = 0; i < 4; i++) tick(dut, trace);

    {
        int16_t out = trigger_sample_tick(dut, trace, &valid);
        check("silence_valid", 1, valid);
        check("silence_sample", 0, out);
    }

    // ── Results ────────────────────────────────────────────────
    printf("\n=== Results: %d passed, %d failed ===\n", test_pass, test_fail);

    trace->close();
    delete dut;
    return test_fail > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
