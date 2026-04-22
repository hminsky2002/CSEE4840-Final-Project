#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "midi.h"
#include "oscillator.h"

void *run_midi_reciever(){

    /* open MIDI device */
    int midi_fd = midi_open();
    if (midi_fd < 0) return NULL;

    /* main event loop */
    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_fd, &midi_packet) < 0) {
            continue;
        }
        printf("%02x %02x %02x\n",
               midi_packet.status, midi_packet.note, midi_packet.attack);
    }
    return NULL;
}


int main() {
    /* mmap at startup */

    pthread_t midi_thread;
    pthread_create(&midi_thread, NULL, run_midi_reciever, NULL);
    pthread_join(midi_thread, NULL);   /* runs forever; exit via SIGINT */
    return 0;
}
