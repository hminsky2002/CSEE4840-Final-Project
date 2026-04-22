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
#include <stddef.h>
#include <string.h>

#define MIDI_LOG_CAP 4096

struct libusb_device_handle *midi;
uint8_t endpoint_address;

static midi_event_t midi_log[MIDI_LOG_CAP];
static size_t midi_log_count = 0;

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

    midi_log[midi_log_count % MIDI_LOG_CAP] = *evt;
    midi_log_count++;

    return 0;
}

void midi_dump_log(void) {
    size_t start = (midi_log_count > MIDI_LOG_CAP) ? midi_log_count - MIDI_LOG_CAP : 0;
    size_t n = midi_log_count - start;
    fprintf(stderr, "--- midi log: %zu events (showing last %zu) ---\n",
            midi_log_count, n);
    for (size_t i = 0; i < n; i++) {
        midi_event_t *e = &midi_log[(start + i) % MIDI_LOG_CAP];
        fprintf(stderr, "%02x %02x %02x %02x\n",
                e->cable, e->status, e->note, e->velocity);
    }
    fflush(stderr);
}

void midi_close(struct libusb_device_handle *midi) {
    libusb_close(midi);
    libusb_exit(NULL);
}