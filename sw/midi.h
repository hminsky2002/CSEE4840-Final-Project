#ifndef _MIDI_H
#define _MIDI_H

#include <stdint.h>

typedef struct {
    uint8_t status;
    uint8_t note;
    uint8_t attack;
} midi_event_t;


#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_PITCH_BEND 0xE0

#define MIDI_CC_VOLUME 0x07

#define MIDI_STATUS_MASK 0xF0
#define MIDI_CHANNEL_MASK 0x0F


int midi_open(void);
int midi_read(int fd, midi_event_t *evt);
void midi_close(int fd);

#endif 