/*
 *
 * CSEE 4840 Final Project
 *
 * Name/UNI: Henry Minsky (hm3121), Opalina Khanna (ok2373), Sunny Fang (yf2610)
 */
#include "midi.h"
#include "usbmidi.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct libusb_device_handle *midi;
uint8_t endpoint_address;

struct libusb_device_handle *midi_open(uint8_t *endpoint_out) {
    midi = open_midi(endpoint_out);
    endpoint_address = *endpoint_out;
    return midi;
}

int midi_read(struct libusb_device_handle *midi,
                uint8_t endpoint_address,
                midi_event_t *evt) {
    int transferred;

    for (;;) {
        memset(evt, 0, sizeof(*evt));
        int r = libusb_bulk_transfer(midi, endpoint_address,
            (unsigned char *)evt, sizeof(*evt),
            &transferred, 1000);
        if (r != 0) continue;
        if (evt->status != 0x08 && evt->status != 0x09 &&
            evt->status != 0x18 && evt->status != 0x19) continue;
        break;
    }

    printf("%02x %02x %02x %02x\n",
        evt->status,
        evt->not_sure,
        evt->note,
        evt->attack
    );

    return 0;
}

void midi_close(struct libusb_device_handle *midi) {
    libusb_close(midi);
    libusb_exit(NULL);
}