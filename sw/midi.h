#ifndef _MIDI_H
#define _MIDI_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>

typedef struct {
    uint8_t cable;
    uint8_t status;
    uint8_t note;
    uint8_t velocity;
} midi_event_t;

struct libusb_device_handle *midi_open(uint8_t *endpoint_out);
int  midi_read(struct libusb_device_handle *midi, uint8_t endpoint, midi_event_t *evt);
void midi_close(struct libusb_device_handle *midi);
void midi_dump_log(void);

#endif /* _MIDI_H */
