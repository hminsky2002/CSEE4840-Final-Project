#ifndef _USBMIDI_H
#define _USBMIDI_H

#include <libusb-1.0/libusb.h>

#define LIBUSB_CLASS_AUDIO 1
#define MIDI_STREAMING 3

struct usb_midi_input {
    uint8_t cin;     // cable number + code index number
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

/* Find and open a USB MIDI device.  Argument should point to
   space to store an endpoint address.  Returns NULL if no MIDI
   device was found. */
extern struct libusb_device_handle *open_midi(uint8_t *);

#endif
