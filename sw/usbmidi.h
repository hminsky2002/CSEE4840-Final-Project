#ifndef _USBMIDI_H
#define _USBMIDI_H

#include <libusb-1.0/libusb.h>

#define LIBUSB_CLASS_AUDIO 1
#define MIDI_STREAMING 3

extern struct libusb_device_handle *open_midi(uint8_t *);

#endif
