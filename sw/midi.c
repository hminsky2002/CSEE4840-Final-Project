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
    static uint8_t buf[64];
    static int transferred = 0;
    static int pos = 0;

    if (pos + 4 > transferred) {
        int r = libusb_bulk_transfer(midi, endpoint_address,
            buf, sizeof(buf),
            &transferred, 500000);
        if (r < 0) { transferred = 0; pos = 0; return r; }
        pos = 0;
        printf("transferred=%d\n", transferred);
        if (transferred < 4) return -1;
    }

    evt->cable      = buf[pos + 0];
    evt->status     = buf[pos + 1];
    evt->note       = buf[pos + 2];
    evt->velocity   = buf[pos + 3];
    pos += 4;

    printf("%02x %02x %02x %02x\n",
        evt->cable,
        evt->status,
        evt->note,
        evt->velocity
    );

    return 0;
}

void midi_close(struct libusb_device_handle *midi) {
    libusb_close(midi);
    libusb_exit(NULL);
}