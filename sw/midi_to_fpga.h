#ifndef MIDI_TO_FPGA_H
#define MIDI_TO_FPGA_H

#include <stdint.h>

// Channel-voice MIDI status bytes (high nibble; low nibble is channel).
#define MIDI_NOTE_ON         0x90
#define MIDI_NOTE_OFF        0x80
#define MIDI_CONTROL_CHANGE  0xB0
#define MIDI_PROGRAM_CHANGE  0xC0
#define MIDI_PITCH_BEND      0xE0
#define MIDI_STATUS_MASK     0xF0
#define MIDI_CHANNEL_MASK    0x0F

uint16_t note_to_step_size(uint8_t midi_note);

#endif /* MIDI_TO_FPGA_H */
