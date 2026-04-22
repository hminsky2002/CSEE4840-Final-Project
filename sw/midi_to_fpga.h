#ifndef HPS_FPGA_BRIDGE_H
#define HPS_FPGA_BRIDGE_H

#include <stdint.h>

// Lightweight HPS-to-FPGA bridge base address (fixed on Cyclone V)
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00200000

// Peripheral base: 0x0010 bytes from LW bridge base → 0x08 uint16_t words
// (confirmed via soc_system.qsys baseAddress = 0x0010)
#define SYNTH_BASE_WORD_OFFSET  0x08

// Register map (16-bit word offsets into lw_bridge pointer)
// Byte address = 0xFF200000 + offset * 2
#define NOTE_ON_OFFSET    (SYNTH_BASE_WORD_OFFSET + 0)  // bit 0: 1=playing, 0=off  (maps to hw enable_reg)
#define STEP_SIZE_OFFSET  (SYNTH_BASE_WORD_OFFSET + 1)  // phase accumulator step size (hw stub)
#define VELOCITY_OFFSET   (SYNTH_BASE_WORD_OFFSET + 2)  // amplitude control (hw stub)
#define WAVETABLE_OFFSET  (SYNTH_BASE_WORD_OFFSET + 3)  // wavetable data start (deferred — needs wider hw address bus)

#define TABLE_SIZE  2048
#define SAMPLE_RATE 48000

// MIDI constants (matched against the USB-MIDI CIN byte, not the MIDI status byte)
#define MIDI_NOTE_ON      0x09
#define MIDI_NOTE_OFF     0x08
#define MIDI_STATUS_MASK  0x0F
#define MIDI_CHANNEL_MASK 0x0F

typedef struct {
    volatile uint16_t *lw_bridge;
    int mem_fd;
} fpga_handle_t;

int  fpga_init(fpga_handle_t *handle);
void fpga_cleanup(fpga_handle_t *handle);

void fpga_set_note_on(fpga_handle_t *handle, uint8_t on);
void fpga_set_step_size(fpga_handle_t *handle, uint16_t step_size);
void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity);

uint16_t note_to_step_size(uint8_t midi_note);

int write_wavetable_to_fpga(fpga_handle_t *handle, const int16_t *samples, int n);
int load_wavetable(fpga_handle_t *handle, const char *filepath);

#endif /* HPS_FPGA_BRIDGE_H */