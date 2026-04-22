#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include "midi.h"
#include "midi_to_fpga.h"
#include "oscillator.h"

#define NUM_OSCILLATORS 32

uint16_t note_to_step_size(uint8_t note) {
	double frequency = 440.0 * pow(2.0, (note - 69) / 12.0);
	uint32_t step = (uint32_t)((frequency * TABLE_SIZE) / SAMPLE_RATE);
	return (uint16_t)(step > 0xFFFF ? 0xFFFF : step);
}

int find_free_slot(struct oscillator *array){
    int i = 0;
    for(i = 0; i < NUM_OSCILLATORS; i++){
        if(!array[i].in_use){
            return i;
        }
    }
    return -1;
}

int find_note(struct oscillator *array, uint8_t note){
    int i = 0;
    for(i = 0; i < NUM_OSCILLATORS; i++){
        if(array[i].in_use && array[i].note == note){
            return i;
        }
    }
    return -1;
}

int clear_from_array(struct oscillator *array, uint8_t note){
    int cleared = 0;
    for(int i = 0; i < NUM_OSCILLATORS; i++){
        if(array[i].in_use && array[i].note == note){
            array[i] = (struct oscillator){0};
            cleared++;
        }
    }
    return cleared;
}

void panic_release_all(struct oscillator *array){
    for(int i = 0; i < NUM_OSCILLATORS; i++){
        array[i] = (struct oscillator){0};
    }
}

void print_active_oscillators(struct oscillator *array){
    int i = 0;
    for(i = 0; i < NUM_OSCILLATORS; i++){
        if(array[i].in_use){
            printf("  [%2d] note=%3u step=0x%04x control=0x%04x wavetable=0x%04x\n",
                   i, array[i].note, array[i].step_size,
                   array[i].control, array[i].wavetable);
        }
    }
}

int main(){

    struct oscillator array[NUM_OSCILLATORS] = {0};

    uint8_t endpoint;
    struct libusb_device_handle *midi_device = midi_open(&endpoint);
    if(midi_device == NULL){
        return -1;
    }

    midi_event_t midi_packet;

    while (1) {
        if (midi_read(midi_device, endpoint, &midi_packet) < 0) continue;

        if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_ON && midi_packet.velocity > 0) {
            if (find_note(array, midi_packet.note) != -1) continue;
            int index = find_free_slot(array);
            if(index == -1){
                continue;
            }
            array[index].step_size = note_to_step_size(midi_packet.note);
            array[index].in_use = 1;
            array[index].control = 1;
            array[index].note = midi_packet.note;
            array[index].wavetable = 0;
        } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF
                   || midi_packet.velocity == 0) {
            if (clear_from_array(array, midi_packet.note) == 0) {
                panic_release_all(array);
            }
        }

        print_active_oscillators(array);
    }



    return 0;
}