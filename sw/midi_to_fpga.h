#ifndef HPS_FPGA_BRIDGE_H
#define HPS_FPGA_BRIDGE_H

#include <stdint.h>

// Lightweight HPS-to-FPGA bridge base address (fixed on Cyclone V)
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00200000

// Peripheral base: 0x80000 bytes from LW bridge base = 0x40000 uint16_t words
// (wave_table_synth_0 at base 0x00080000 in Qsys, 512KB span, 18-bit address)
#define SYNTH_BASE_WORD_OFFSET  0x40000

// Per-voice control registers (address[17]=0)
// Hardware decodes address[4:2] = voice, address[1:0] = register
#define NUM_VOICES 8
#define VOICE_NOTE_ON(v)     (SYNTH_BASE_WORD_OFFSET + (v)*4 + 0)
#define VOICE_STEP_SIZE(v)   (SYNTH_BASE_WORD_OFFSET + (v)*4 + 1)
#define VOICE_VELOCITY(v)    (SYNTH_BASE_WORD_OFFSET + (v)*4 + 2)
#define VOICE_SLOT_SELECT(v) (SYNTH_BASE_WORD_OFFSET + (v)*4 + 3)

// Wavetable data region (address[17]=1)
// address[16:12] = slot (0-31), address[11:0] = sample index (0-4095)
#define WAVETABLE_BASE_OFFSET  (SYNTH_BASE_WORD_OFFSET + (1 << 17))
#define WAVETABLE_SLOT_WORDS   4096
#define WAVETABLE_NUM_SLOTS    12

#define TABLE_SIZE  4096
#define SAMPLE_RATE 48000

// MIDI constants
#define MIDI_NOTE_ON      0x90
#define MIDI_NOTE_OFF     0x80
#define MIDI_STATUS_MASK  0xF0
#define MIDI_CHANNEL_MASK 0x0F

typedef struct {
    volatile uint16_t *lw_bridge;
    int mem_fd;
} fpga_handle_t;

int  fpga_init(fpga_handle_t *handle);
void fpga_cleanup(fpga_handle_t *handle);

void fpga_set_note_on(fpga_handle_t *handle, uint8_t voice, uint8_t on);
void fpga_set_step_size(fpga_handle_t *handle, uint8_t voice, uint16_t step_size);
void fpga_set_velocity(fpga_handle_t *handle, uint8_t voice, uint8_t velocity);
void fpga_set_slot_select(fpga_handle_t *handle, uint8_t voice, uint8_t slot);

uint16_t note_to_step_size(uint8_t midi_note);

int write_wavetable_to_fpga(fpga_handle_t *handle, uint8_t slot, const int16_t *samples, int n);
int load_wavetable(fpga_handle_t *handle, uint8_t slot, const char *filepath);

#endif /* HPS_FPGA_BRIDGE_H */
