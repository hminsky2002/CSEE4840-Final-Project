#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "fpga_bridge.h"
#include "oscillator.h"
#include "wavetable.h"

void *run_midi_reciever(){

    /* open MIDI device */
    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);

    /* main event loop */
    midi_event_t midi_packet;
    while (1) {
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) {
            continue;
        }

        
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
