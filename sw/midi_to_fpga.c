#include "fpga_bridge.h"
#include "midi.h"
#include "synth_functions.h"
#include "wavetable.h"
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>

// our global array to track our oscillator states!
static struct oscillator oscillators[NUM_OSCILLATORS] = {0};

static int global_wavetable = 0;
static uint16_t global_amp = AMP_UNITY;

#define MIDI_CC_VOLUME 7

void *run_midi_reciever(void *arg) {
  peripheral *lw_bus = (peripheral *)arg;

  int midi_fd = midi_open();
  if (midi_fd < 0) {
    return NULL;
  }

  midi_event_t midi_packet;

  while (1) {
    if (midi_read(midi_fd, &midi_packet) < 0) {
      continue;
    }

    if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_ON) {
      int i = osc_find_free_slot(oscillators);

      if (i >= 0) {
        uint16_t step = note_to_step_size(midi_packet.note);
        oscillators[i].note = midi_packet.note;
        oscillators[i].step_size = step;
        oscillators[i].wavetable_slot = global_wavetable;
        oscillators[i].amplitude = global_amp;
        oscillators[i].in_use = true;

        fpga_voice_start(lw_bus, i, step, global_wavetable, global_amp);
      }

    } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF) {
      int i = osc_find_note_slot(oscillators, midi_packet.note);

      if (i >= 0) {
        oscillators[i].in_use = false;
        oscillators[i].note = 0;
        oscillators[i].step_size = 0;
        oscillators[i].wavetable_slot = 0;
        oscillators[i].amplitude = 0;
        fpga_kill_voice(lw_bus, i);
      }

    } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_CONTROL_CHANGE) {
      if (midi_packet.note == MIDI_CC_VOLUME) {
        global_amp = (uint16_t)midi_packet.attack << 9;
        for (int v = 0; v < NUM_OSCILLATORS; v++) {
          if (oscillators[v].in_use) {
            oscillators[v].amplitude = global_amp;
            fpga_set_amp(lw_bus, v, global_amp);
          }
        }
      }
    }
  }
  return NULL;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "need to pass a *.bin file for loading wavetable in");
  }

  peripheral lw_bus;
  if (fpga_init(&lw_bus) != 0) {
    return -1;
  }

  int loaded = load_wavetable_bin(argv[1], &lw_bus);

  if (loaded <= 0) {
    fprintf(stderr, "No wavetables loaded, check validity of %s \n", argv[1]);
  }

  pthread_t midi_thread;

  pthread_create(&midi_thread, NULL, run_midi_reciever, &lw_bus);

  pthread_join(midi_thread,NULL);
  return 0;
}
