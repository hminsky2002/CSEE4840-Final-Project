#ifndef _MIDI_H
#define _MIDI_H

#include <stdint.h>

/*
 * One channel-voice MIDI message, parsed from the kernel's ALSA rawmidi
 * stream. 
 */
typedef struct {
    uint8_t status;
    uint8_t note;
    uint8_t attack;
} midi_event_t;

int  midi_open(void);
int  midi_read(int fd, midi_event_t *evt);
void midi_close(int fd);

#endif /* _MIDI_H */
