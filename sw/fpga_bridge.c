#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "fpga_bridge.h"

int fpga_init(fpga_handle_t *handle) {
    handle->fd = open("/dev/wave_synth", O_RDWR | O_SYNC);
    if (handle->fd < 0) {
        perror("open /dev/wave_synth");
        return -1;
    }

    handle->regs = mmap(NULL, WAVE_SYNTH_WIN_BYTES,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, handle->fd, 0);
    if (handle->regs == MAP_FAILED) {
        perror("mmap /dev/wave_synth");
        close(handle->fd);
        handle->fd = -1;
        handle->regs = NULL;
        return -1;
    }

    fpga_all_voices_off(handle);
    handle->regs[AMP_CTRL] = 127;
    return 0;
}

void fpga_cleanup(fpga_handle_t *handle) {
    if (handle->regs) {
        fpga_all_voices_off(handle);
        munmap((void *)handle->regs, WAVE_SYNTH_WIN_BYTES);
        handle->regs = NULL;
    }
    if (handle->fd >= 0) {
        close(handle->fd);
        handle->fd = -1;
    }
}

void fpga_set_step(fpga_handle_t *handle, int voice, uint16_t step_size) {
    if (voice < 0 || voice >= NUM_VOICES) return;
    handle->regs[OSC_STEP(voice)] = step_size;
}

void fpga_set_ctrl(fpga_handle_t *handle, int voice, uint16_t ctrl) {
    if (voice < 0 || voice >= NUM_VOICES) return;
    handle->regs[OSC_CTRL(voice)] = ctrl;
}

void fpga_set_table(fpga_handle_t *handle, int voice, uint16_t slot) {
    if (voice < 0 || voice >= NUM_VOICES) return;
    handle->regs[OSC_TABLE(voice)] = slot;
}

void fpga_voice_start(fpga_handle_t *handle, int voice,
                      uint16_t step_size, uint16_t slot) {
    if (voice < 0 || voice >= NUM_VOICES) return;
    handle->regs[OSC_STEP(voice)]  = step_size;
    handle->regs[OSC_TABLE(voice)] = slot;
    handle->regs[OSC_CTRL(voice)]  = CTRL_RESET;

    /* The hardware only clears phase[voice] on a sweep cycle where
     * ctrl==RESET is observed. One sample sweep is ~21 µs at 48 kHz;
     * sleep a few hundred µs to guarantee the reset lands. */
    struct timespec delay = {0, 200 * 1000L};  /* 200 µs */
    nanosleep(&delay, NULL);

    handle->regs[OSC_CTRL(voice)] = CTRL_START;
}

void fpga_voice_stop(fpga_handle_t *handle, int voice) {
    if (voice < 0 || voice >= NUM_VOICES) return;
    handle->regs[OSC_CTRL(voice)] = CTRL_STOP;
}

void fpga_all_voices_off(fpga_handle_t *handle) {
    for (int v = 0; v < NUM_VOICES; v++) {
        handle->regs[OSC_CTRL(v)] = CTRL_STOP;
    }
}

int fpga_load_slot(fpga_handle_t *handle, int slot,
                   const int16_t *samples, int n) {
    if (slot < 0 || slot >= 16) return -1;
    if (!samples || n <= 0) return -1;
    if (n > TABLE_SIZE) n = TABLE_SIZE;

    for (int i = 0; i < n; i++) {
        handle->regs[WAVETABLE_WORD(slot, i)] = (uint16_t)samples[i];
    }
    /* Zero-fill any tail so short WAVs don't read stale BRAM. */
    for (int i = n; i < TABLE_SIZE; i++) {
        handle->regs[WAVETABLE_WORD(slot, i)] = 0;
    }
    return 0;
}
