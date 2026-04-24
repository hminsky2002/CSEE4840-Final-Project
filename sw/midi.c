/*
 *
 * CSEE 4840 Final Project
 *
 * Name/UNI: Henry Minsky (hm3121), Opalina Khanna (ok2373), Sunny Fang (yf2610)
 */
#define _POSIX_C_SOURCE 199309L   /* expose clock_gettime / CLOCK_MONOTONIC */

#include "midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

int midi_open(void) {
    char path[32];
    for (int card = 0; card < 8; card++) {
        snprintf(path, sizeof(path), "/dev/snd/midiC%dD0", card);
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            return fd;
        }
    }
    fprintf(stderr, "midi_open: no /dev/snd/midiC*D0 device found\n");
    return -1;
}

/* The DE1-SoC's dwc2 USB host on kernel 4.19 has a split-transaction bug
 * that occasionally redelivers the same bulk-IN URB to a full-speed device
 * sitting behind a hub.  Symptom: every MIDI byte is duplicated sub-ms
 * apart.  We drop any event byte-identical to the previous one within
 * DEDUP_WINDOW_MS — well above the redelivery gap, well below the
 * fastest a human can re-press the same key. */
#define DEDUP_WINDOW_MS 5

static long ms_since(const struct timespec *a, const struct timespec *b) {
    return (a->tv_sec - b->tv_sec) * 1000 + (a->tv_nsec - b->tv_nsec) / 1000000;
}

/* Read one channel-voice message from the ALSA rawmidi stream.
 * Handles MIDI running status, skips realtime (0xF8-0xFF) and system
 * common/sysex (0xF0-0xF7) messages, drops dwc2-style duplicates.
 * Blocks on read(); returns 0 on success, -1 if the device closes. */
int midi_read(int fd, midi_event_t *evt) {
    static uint8_t running_status = 0;
    static midi_event_t last_evt = {0};
    static struct timespec last_ts = {0};
    uint8_t b;

    for (;;) {
        if (read(fd, &b, 1) != 1) return -1;

        if (b >= 0xF8) continue;          /* realtime: ignore */
        if (b >= 0xF0) {                   /* system common / sysex */
            running_status = 0;
            continue;
        }

        if (b & 0x80) {
            evt->status = b;
            running_status = b;
            if (read(fd, &evt->note, 1) != 1) return -1;
        } else {
            /* data byte; need a remembered status to interpret */
            if (!(running_status & 0x80)) continue;
            evt->status = running_status;
            evt->note   = b;
        }

        uint8_t kind = evt->status & 0xF0;
        if (kind == 0xC0 || kind == 0xD0) {
            evt->attack = 0;
        } else {
            if (read(fd, &evt->attack, 1) != 1) return -1;
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (evt->status == last_evt.status &&
            evt->note   == last_evt.note   &&
            evt->attack == last_evt.attack &&
            ms_since(&now, &last_ts) < DEDUP_WINDOW_MS) {
            continue;   /* dwc2 redelivery — drop */
        }
        last_evt = *evt;
        last_ts  = now;
        return 0;
    }
}

void midi_close(int fd) {
    if (fd >= 0) close(fd);
}
