#include "fpga_bridge.h"
#include "midi.h"
#include "synth_functions.h"
#include "wavetable.h"
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

// our global array to track our oscillator states!
static struct oscillator oscillators[NUM_OSCILLATORS] = {0};

static int global_wavetable = 0;

pthread_mutex_t osc_lock;

static uint8_t last_note = 0;
static bool have_last_note = false;

/* note class 0..11 = C, C#, D, D#, E, F, F#, G, G#, A, A#, B */
static const uint8_t NOTE_LETTER[12] = {
    SEG_C, SEG_C, SEG_D, SEG_D, SEG_E, SEG_F,
    SEG_F, SEG_G, SEG_G, SEG_A, SEG_A, SEG_B,
};
static const uint8_t NOTE_IS_SHARP[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
static const uint8_t SEG_DIGIT[10] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4,
                                      SEG_5, SEG_6, SEG_7, SEG_8, SEG_9};

/* HEX2, HEX1, HEX0 patterns per wavetable slot */
static const uint8_t TABLE_NAME[NUM_TABLE_SLOTS][3] = {
    {SEG_S, SEG_I, SEG_N}, /* slot 0: sin (sine) */
    {SEG_S, SEG_A, SEG_U}, /* slot 1: sau (sawtooth) */
    {SEG_S, SEG_Q, SEG_R}, /* slot 2: sqr (square) */
    {SEG_T, SEG_R, SEG_I}, /* slot 3: tri (triangle) */
};

static void update_display(peripheral *lw_bus) {
  /* HEX2..HEX0: current wavetable abbreviation (always shown) */
  int slot = global_wavetable;
  if (slot < 0 || slot >= NUM_TABLE_SLOTS)
    slot = 0;
  fpga_set_hex(lw_bus, 2, TABLE_NAME[slot][0]);
  fpga_set_hex(lw_bus, 1, TABLE_NAME[slot][1]);
  fpga_set_hex(lw_bus, 0, TABLE_NAME[slot][2]);

  /* HEX5..HEX3: most recently pressed note (blank until first note) */
  if (!have_last_note) {
    fpga_set_hex(lw_bus, 5, SEG_BLANK);
    fpga_set_hex(lw_bus, 4, SEG_BLANK);
    fpga_set_hex(lw_bus, 3, SEG_BLANK);
    return;
  }

  uint8_t cls = last_note % 12;
  int octave = (int)(last_note / 12) - 1; /* MIDI: note 0 = C-1 */
  if (octave < 0)
    octave = 0;
  if (octave > 9)
    octave = 9;

  fpga_set_hex(lw_bus, 5, NOTE_LETTER[cls]);
  fpga_set_hex(lw_bus, 4, NOTE_IS_SHARP[cls] ? SEG_S : SEG_BLANK);
  fpga_set_hex(lw_bus, 3, SEG_DIGIT[octave]);
}

double AMP_STEP = 1.0 / 128;

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
      uint16_t step = 0;
      pthread_mutex_lock(&osc_lock);
      int i = osc_find_free_slot(oscillators);
      if (i >= 0) {
        step = note_to_step_size(midi_packet.note);
        oscillators[i].note = midi_packet.note;
        oscillators[i].step_size = step;
        oscillators[i].wavetable_slot = global_wavetable;
        oscillators[i].in_use = true;
        oscillators[i].phase = ENV_ATTACK;
        oscillators[i].sustain = ((double) AMP_STEP * midi_packet.attack )* ENV_SUSTAIN_LEVEL;
      }
      pthread_mutex_unlock(&osc_lock);

      if (i >= 0) {
        fpga_voice_start(lw_bus, i, step, global_wavetable);
        last_note = midi_packet.note;
        have_last_note = true;
        update_display(lw_bus);
      }

    } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_NOTE_OFF) {
      pthread_mutex_lock(&osc_lock);
      int i = osc_find_note_slot(oscillators, midi_packet.note);

      pthread_mutex_unlock(&osc_lock);

      if (i >= 0) {
        update_display(lw_bus);
      }
    } else if ((midi_packet.status & MIDI_STATUS_MASK) == MIDI_PITCH_BEND) {
      if (midi_packet.note == 0x7F && midi_packet.attack == 0x7F) {
        global_wavetable = (global_wavetable + 1) % NUM_TABLE_SLOTS;
        for (int j = 0; j < NUM_OSCILLATORS; j++) {
          if (oscillators[j].in_use) {
            pthread_mutex_lock(&osc_lock);
            oscillators[j].wavetable_slot = global_wavetable;
            pthread_mutex_unlock(&osc_lock);
            fpga_set_table(lw_bus, j, global_wavetable);
          }
        }
        update_display(lw_bus);
      }
    } else if (midi_packet.status == 0xB1 && midi_packet.note == 0x07) {
      uint16_t amp = (uint16_t)(midi_packet.attack & 0x7F) << 1;
      fpga_set_master_amp(lw_bus, amp);
    }
  }
  return NULL;
}

void *run_adsr_envelope(void *arg) {
  peripheral *lw_bus = (peripheral *)arg;
  struct timespec tick = {0, ENV_TICK_NS};
  static uint16_t last_written[NUM_OSCILLATORS] = {0};

  while (1) {
    clock_nanosleep(CLOCK_MONOTONIC, 0, &tick, NULL);
    /* iterate over each oscillator */

    for (int i = 0; i < NUM_OSCILLATORS; i++) {
      struct oscillator *curr = &oscillators[i];
      uint16_t curr_amp;
      bool kill_voice = false;
      pthread_mutex_lock(&osc_lock);
      switch (curr->phase) {
      case (ENV_IDLE):
        curr->env_amp_q8 = 0;
        break;
      case (ENV_ATTACK):
        if (curr->env_amp_q8 + ENV_ATTACK_PER_TICK >= ENV_PEAK) {
          curr->phase = ENV_DECAY;
          curr->env_amp_q8 = ENV_PEAK;
        } else {
          curr->env_amp_q8 += ENV_ATTACK_PER_TICK;
          /* no phase change */
        }
        break;
      case (ENV_DECAY):
        if (curr->env_amp_q8 - ENV_DECAY_PER_TICK <= curr->sustain) {
          curr->phase = ENV_SUSTAIN;
          curr->env_amp_q8 = curr->sustain;
        } else {
          /* no phase change */
          curr->env_amp_q8 -= ENV_DECAY_PER_TICK;
        }
        break;
      case (ENV_SUSTAIN):
        curr->env_amp_q8 = curr->sustain;
        break;
      case (ENV_RELEASE):
        if (curr->env_amp_q8 <= ENV_RELEASE_PER_TICK) {
          curr->phase = ENV_IDLE;
          curr->env_amp_q8 = 0;
          curr->in_use = false;
          curr->note = 0;
          curr->step_size = 0;
          curr->wavetable_slot = 0;
          kill_voice = true;
        } else {
          /* no phase change */
          curr->env_amp_q8 -= ENV_RELEASE_PER_TICK;
        }
      }
      curr_amp = curr->env_amp_q8;
      pthread_mutex_unlock(&osc_lock);
      if (kill_voice)
        fpga_kill_voice(lw_bus, i);
      if (curr_amp != last_written[i]) {
        fpga_set_amp(lw_bus, i, curr_amp);
        last_written[i] = curr_amp;
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

  update_display(&lw_bus);

  pthread_t midi_thread;
  pthread_t adsr_thread;
  pthread_mutex_init(&osc_lock, NULL);

  pthread_create(&midi_thread, NULL, run_midi_reciever, &lw_bus);
  pthread_create(&adsr_thread, NULL, run_adsr_envelope, &lw_bus);

  pthread_join(midi_thread, NULL);
  return 0;
}
