#include "fpga_bridge.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
  peripheral lw_bus;
  if (fpga_init(&lw_bus) != 0) return -1;

  /* light every segment on every display */
  for (int i = 0; i < 6; i++) {
    lw_bus.regs[HEX_REG(i)] = 0x00;
    printf("wrote 0x00 to HEX_REG(%d) = 0x%X\n", i, HEX_REG(i));
  }

  sleep(5);

  /* blank them again */
  for (int i = 0; i < 6; i++) {
    lw_bus.regs[HEX_REG(i)] = 0x7F;
  }

  fpga_cleanup(&lw_bus);
  return 0;
}
