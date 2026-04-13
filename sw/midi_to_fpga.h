#ifndef HPS_FPGA_BRIDGE_H
#define HPS_FPGA_BRIDGE_H

#include <stdint.h>

/*
TO CONFIRM:
- base address for board 
- bridge span size 
- register offsets (incl wavetable)
*/

// TEMPS
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00005000 // need to confirm

// TODO: confirm these 
#define STEP_SIZE_OFFSET 0x00
#define NOTE_ON_OFFSET   0x01
#define VELOCITY_OFFSET  0x02
#define WAVETABLE_OFFSET 0x03 // wavetable occupies [0x03 .. 0x03 + TABLE_SIZE - 1]

#define TABLE_SIZE  2048
#define SAMPLE_RATE 48000

// MIDI constants 
#define MIDI_NOTE_ON      0x90
#define MIDI_NOTE_OFF     0x80
#define MIDI_STATUS_MASK  0xF0 // check these two
#define MIDI_CHANNEL_MASK 0x0F

typedef struct {
    volatile uint32_t *lw_bridge;
    int mem_fd;
} fpga_handle_t;

int  fpga_init(fpga_handle_t *handle);
void fpga_cleanup(fpga_handle_t *handle);

void fpga_set_step_size(fpga_handle_t *handle, uint32_t step_size);
void fpga_set_note_on(fpga_handle_t *handle, uint8_t on);
void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity);

uint32_t note_to_step_size(uint8_t midi_note);

int write_wavetable_to_fpga(fpga_handle_t *handle, const int16_t *samples, int n);
int load_wavetable(fpga_handle_t *handle, const char *filepath);

#endif /* HPS_FPGA_BRIDGE_H */