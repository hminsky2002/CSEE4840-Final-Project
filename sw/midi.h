#ifndef _MIDI_H
#define _MIDI_H

#include <stdint.h>

/*
 * One channel-voice MIDI message, parsed from the kernel's ALSA rawmidi
 * stream.  `status` is the full MIDI status byte (e.g. 0x90 = note on,
 * channel 0), so callers should compare `status & 0xF0` against the
 * 0x80/0x90/0xB0/0xC0 etc. constants in midi_to_fpga.h.
 *
 * `note` is data byte 1 (note number / CC number / program number).
 * `attack` is data byte 2 (velocity / CC value); 0 for messages that
 * carry only one data byte (program change, channel pressure).
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
