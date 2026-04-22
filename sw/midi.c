/*
 *
 * CSEE 4840 Final Project
 *
 * Name/UNI: Henry Minsky (hm3121), Opalina Khanna (ok2373), Sunny Fang (yf2610)
 */
#include "midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

/* Read one channel-voice message from the ALSA rawmidi stream.
 * Handles MIDI running status, skips realtime (0xF8-0xFF) and system
 * common/sysex (0xF0-0xF7) messages.  Blocks on read(); returns 0 on
 * success, -1 if the device closes.
 *
 * Note: the dwc2 USB host on this kernel sometimes redelivers the same
 * bulk-IN URB, which shows up as every MIDI byte arriving twice.  We
 * don't dedup here — the synth's note allocator is idempotent (note-on
 * checks find_note_slot, note-off only fires if a slot is in_use), so
 * doubled events are a no-op for playback. */
int midi_read(int fd, midi_event_t *evt) {
    static uint8_t running_status = 0;
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
        return 0;
    }
}

void midi_close(int fd) {
    if (fd >= 0) close(fd);
}
