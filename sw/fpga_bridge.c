#include "fpga_bridge.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

int fpga_init(peripheral *lw_bus) {
  lw_bus->fd = open("/dev/wave_synth", O_RDWR | O_SYNC);

  if (lw_bus->fd < 0) {
    perror("open /dev/wave_synth");
    return -1;
  }

  lw_bus->regs = mmap(NULL, WAVE_SYNTH_PERIPHERAL_BYTES, PROT_READ | PROT_WRITE,
                      MAP_SHARED, lw_bus->fd, 0);

  if (lw_bus->regs == MAP_FAILED) {
    perror("mmap /dev/wave_synth");
    close(lw_bus->fd);
    lw_bus->fd = -1;
    lw_bus->regs = NULL;
    return -1;
  }

  fpga_kill_voices(lw_bus);
  lw_bus->regs[AMP_CTRL] = 127;
  return 0;
}

void fpga_kill_voices(peripheral *lw_bus) {
  for (int i = 0; i < NUM_OSCILLATORS; i++) {
    lw_bus->regs[OSC_CTRL(i)] = CTRL_STOP;
  }
}

void fpga_cleanup(peripheral *lw_bus) {
  if (lw_bus->regs) {
    fpga_kill_voices(lw_bus);
    munmap((void *)lw_bus->regs, WAVE_SYNTH_PERIPHERAL_BYTES);
    lw_bus->regs = NULL;
  }
  if (lw_bus->fd >= 0) {
    close(lw_bus->fd);
    lw_bus->fd = -1;
  }
}

void fpga_set_step(peripheral *lw_bus, int voice, uint16_t step_size) {
  if (voice < 0 || voice >= NUM_OSCILLATORS) {
    return;
  }
  lw_bus->regs[OSC_STEP(voice)] = step_size;
}

void fpga_set_ctrl(peripheral *lw_bus, int voice, uint16_t ctrl) {
  if (voice < 0 || voice >= NUM_OSCILLATORS) {
    return;
  }
  lw_bus->regs[OSC_CTRL(voice)] = ctrl;
}

void fpga_set_table(peripheral *lw_bus, int voice, uint16_t slot) {
  if (voice < 0 || voice >= NUM_OSCILLATORS) {
    return;
  }
  if (slot >= NUM_TABLE_SLOTS) {
    return;
  }
  lw_bus->regs[OSC_TABLE(voice)] = slot;
}

void fpga_set_amp(peripheral *lw_bus, uint16_t amp) {
  lw_bus->regs[AMP_CTRL] = amp;
}

void fpga_set_hex(peripheral *lw_bus, int idx, uint8_t pattern) {
  if (idx < 0 || idx >= 6) {
    return;
  }
  lw_bus->regs[HEX_REG(idx)] = pattern & 0x7Fu;
}

void fpga_voice_start(peripheral *lw_bus, int voice, uint16_t step_size,
                      uint16_t slot) {
  if (voice < 0 || voice >= NUM_OSCILLATORS) {
    return;
  }
  if (slot >= NUM_TABLE_SLOTS) {
    return;
  }
  lw_bus->regs[OSC_STEP(voice)] = step_size;
  lw_bus->regs[OSC_TABLE(voice)] = slot;
  lw_bus->regs[OSC_CTRL(voice)] = CTRL_RESET;

  struct timespec delay = {0, 200 * 1000L};
  nanosleep(&delay, NULL);

  lw_bus->regs[OSC_CTRL(voice)] = CTRL_START;
}

void fpga_kill_voice(peripheral *lw_bus, int voice) {
  if (voice < 0 || voice >= NUM_OSCILLATORS) {
    return;
  }
  lw_bus->regs[OSC_CTRL(voice)] = CTRL_STOP;
}

int fpga_load_slot(peripheral *lw_bus, int slot, const int16_t *samples,
                    int n) {
  if (slot < 0 || slot >= NUM_TABLE_SLOTS) {
    return -1;
  }
  if (!samples || n <= 0) {
    return -1;
  }
  if(n > TABLE_SIZE){
    n = TABLE_SIZE;
  }

  for(int i = 0; i < n; i++){
    lw_bus->regs[WAVETABLE_WORD(slot, i)] = (uint16_t)samples[i];
  }

  
  for (int i = n; i < TABLE_SIZE; i++){
    lw_bus->regs[WAVETABLE_WORD(slot, i)] = 0;
  }

  return 0;
}