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

uint16_t avalon_read(Vwave_table_synth *dut, VerilatedVcdC *trace,
                     uint32_t addr) {
    dut->chipselect = 1;
    dut->write = 0;
    dut->address = addr;
    tick(dut, trace); // posedge: BRAM/regs capture address, outputs register
    uint16_t val = dut->readdata;
    dut->chipselect = 0;
    return val;
}

void check(const char *name, uint16_t expected, uint16_t actual) {
    if (expected == actual) {
        test_pass++;
    } else {
        test_fail++;
        printf("FAIL: %s — expected 0x%04X, got 0x%04X (sim_time=%lu)\n",
               name, expected, actual, (unsigned long)sim_time);
    }
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

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vwave_table_synth *dut = new Vwave_table_synth;

    Verilated::traceEverOn(true);
    VerilatedVcdC *trace = new VerilatedVcdC;
    dut->trace(trace, 5);
    trace->open("waveform.vcd");

    do_reset(dut, trace);

    // ── Test 1: Write and read back wavetable slot 0 ──────────
    printf("Test 1: Write/read slot 0 samples...\n");
    for (int i = 0; i < 16; i++)
        avalon_write(dut, trace, WT_ADDR(0, i), 0xA000 + i);

    for (int i = 0; i < 16; i++) {
        uint16_t val = avalon_read(dut, trace, WT_ADDR(0, i));
        char msg[64];
        snprintf(msg, sizeof(msg), "slot0[%d]", i);
        check(msg, 0xA000 + i, val);
    }

    // ── Test 2: Write and read back wavetable slot 1 ──────────
    printf("Test 2: Write/read slot 1 samples...\n");
    for (int i = 0; i < 8; i++)
        avalon_write(dut, trace, WT_ADDR(1, i), 0xB000 + i);

    for (int i = 0; i < 8; i++) {
        uint16_t val = avalon_read(dut, trace, WT_ADDR(1, i));
        char msg[64];
        snprintf(msg, sizeof(msg), "slot1[%d]", i);
        check(msg, 0xB000 + i, val);
    }

    // ── Test 3: Verify slot 0 unaffected by slot 1 writes ────
    printf("Test 3: Slot 0 isolation check...\n");
    for (int i = 0; i < 16; i++) {
        uint16_t val = avalon_read(dut, trace, WT_ADDR(0, i));
        char msg[64];
        snprintf(msg, sizeof(msg), "slot0_recheck[%d]", i);
        check(msg, 0xA000 + i, val);
    }

    // ── Test 4: Control register write/read per voice ─────────
    printf("Test 4: Per-voice control registers...\n");
    for (int v = 0; v < 8; v++) {
        avalon_write(dut, trace, CTRL_ADDR(v, REG_NOTE_ON),   v & 1);
        avalon_write(dut, trace, CTRL_ADDR(v, REG_STEP_SIZE), 1000 + v * 100);
        avalon_write(dut, trace, CTRL_ADDR(v, REG_VELOCITY),  64 + v);
        avalon_write(dut, trace, CTRL_ADDR(v, REG_SLOT_SEL),  v);
    }

    for (int v = 0; v < 8; v++) {
        char msg[64];

        snprintf(msg, sizeof(msg), "voice%d_note_on", v);
        check(msg, v & 1, avalon_read(dut, trace, CTRL_ADDR(v, REG_NOTE_ON)));

        snprintf(msg, sizeof(msg), "voice%d_step_size", v);
        check(msg, 1000 + v * 100, avalon_read(dut, trace, CTRL_ADDR(v, REG_STEP_SIZE)));

        snprintf(msg, sizeof(msg), "voice%d_velocity", v);
        check(msg, 64 + v, avalon_read(dut, trace, CTRL_ADDR(v, REG_VELOCITY)));

        snprintf(msg, sizeof(msg), "voice%d_slot_sel", v);
        check(msg, v, avalon_read(dut, trace, CTRL_ADDR(v, REG_SLOT_SEL)));
    }

    // ── Test 5: Boundary — last sample in slot 31 ────────────
    printf("Test 5: Boundary write (slot 31, sample 4095)...\n");
    avalon_write(dut, trace, WT_ADDR(31, 4095), 0xDEAD);
    check("slot31[4095]", 0xDEAD, avalon_read(dut, trace, WT_ADDR(31, 4095)));

    // ── Results ───────────────────────────────────────────────
    printf("\n=== Results: %d passed, %d failed ===\n", test_pass, test_fail);

    trace->close();
    delete dut;
    return test_fail > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
