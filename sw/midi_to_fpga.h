#ifndef MIDI_TO_FPGA_H
#define MIDI_TO_FPGA_H

#include <stdint.h>

// MIDI constants (matched against the USB-MIDI CIN byte, not the MIDI status byte)
#define MIDI_NOTE_ON         0x09
#define MIDI_NOTE_OFF        0x08
#define MIDI_PROGRAM_CHANGE  0x0C
#define MIDI_STATUS_MASK     0x0F
#define MIDI_CHANNEL_MASK    0x0F

uint16_t note_to_step_size(uint8_t midi_note);

#endif /* MIDI_TO_FPGA_H */
