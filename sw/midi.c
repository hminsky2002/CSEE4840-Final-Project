#include <alsa/asoundlib.h>
#include <stdio.h>

int main() {
    snd_seq_t *seq_handle;
    snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
    snd_seq_set_client_name(seq_handle, "My MIDI Receiver");
    int port_id = snd_seq_create_simple_port(seq_handle, "Input Port",
                  SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                  SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    while (1) {
        snd_seq_event_t *ev;
        snd_seq_event_input(seq_handle, &ev);
        if (ev->type == SND_SEQ_EVENT_NOTEON) {
            printf("Note On: %d\n", ev->data.note.note);
        }
        snd_seq_free_event(ev);
    }
    return 0;
}