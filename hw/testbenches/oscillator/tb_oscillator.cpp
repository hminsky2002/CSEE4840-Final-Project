#include "Voscillator.h"
#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <verilated.h>
#include <verilated_vcd_c.h>

#define MAX_SIM_TIME 500
#define VERIF_START_TIME 7
vluint64_t sim_time = 0;
vluint64_t posedge_cnt = 0;

void dut_reset(Voscillator *dut, vluint64_t &sim_time) {
  dut->rst = 0;
  if (sim_time >= 3 && sim_time < 6) {
    dut->rst = 1;
    dut->step = 0;
    dut->sample_en = 0;
  }
}

int main(int argc, char **argv, char **env) {
  srand(time(NULL));
  Verilated::commandArgs(argc, argv);
  Voscillator *dut = new Voscillator;

  Verilated::traceEverOn(true);
  VerilatedVcdC *m_trace = new VerilatedVcdC;
  dut->trace(m_trace, 5);
  m_trace->open("waveform.vcd");

  while (sim_time < MAX_SIM_TIME) {
    dut_reset(dut, sim_time);

    dut->clk ^= 1;
    dut->eval();

    if (dut->clk == 1) {
      posedge_cnt++;
      if (sim_time >= 7) {
        dut->step = 50;
      }
      if (sim_time >= VERIF_START_TIME) {
        dut->sample_en = ((posedge_cnt % 10) < 3) ? 1 : 0;
      }
    }

    m_trace->dump(sim_time);
    sim_time++;
  }

  m_trace->close();
  delete dut;
  exit(EXIT_SUCCESS);
}