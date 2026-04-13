#ifndef HPS_FPGA_BRIDGE_H
#define HPS_FPGA_BRIDGE_H

#include <stdint.h>

/*
TO CONFIRM:
- base address for board 
- bridge span size 
- register offsets
- wavetable base address and length 
- wavetable file format?
- sample rate - think i got this right but gonna double check anyway? 
- how is the MIDI byte data being delivered to this module
*/

// TEMPS
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00005000

// Harry and I need to confirm these 
#define STEP_SIZE_OFFSET 0x00
#define NOTE_ON_OFFSET 0x01
#define VELOCITY_OFFSET 0x02
#define WAVETABLE_OFFSET 0x03

#define WAVETABLE_LENGTH 1024
#define SAMPLE_RATE 48000

// MIDI constants 
#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_STATUS_MASK 0xF0
#define MIDI_CHANNEL_MASK 0x0F


typedef struct {
    uint8_t status;
    uint8_t note;
    uint8_t velocity;
} midi_msg_t;

typedef struct {
    volatile uint16_t *lw_bridge;
    int mem_fd;
} fpga_handle_t;


int fpga_init(fpga_handle_t *handle);

void fpga_cleanup(fpga_handle_t *handle);

int load_wavetable(fpga_handle_t *handle, const char *filepath);

uint16_t note_to_step_size(uint8_t midi_note);

void fpga_set_step_size(fpga_handle_t *handle, uint16_t step_size);

void fpga_set_note_on(fpga_handle_t *handle, uint8_t on);

void fpga_set_velocity(fpga_handle_t *handle, uint8_t velocity);

int read_midi_message(midi_msg_t *msg);

void handle_midi_message(fpga_handle_t *handle, midi_msg_t *msg);

#endif /* HPS_FPGA_BRIDGE_H */