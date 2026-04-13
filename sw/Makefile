# ─── COMPILER SETTINGS ────────────────────────────────────────────────────────
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm                          # required for pow() in note_to_step_size

# ─── TARGET ───────────────────────────────────────────────────────────────────
TARGET  = midi_to_fpga

# ─── SOURCES & OBJECTS ────────────────────────────────────────────────────────
SRCS    = midi_to_fpga.c wavetable.c
OBJS    = $(SRCS:.c=.o)
DEPS    = midi_to_fpga.h

# ─── DEFAULT TARGET ───────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# ─── COMPILE EACH .c TO .o ────────────────────────────────────────────────────
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# ─── CLEAN ────────────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(TARGET)

# ─── PHONY TARGETS ────────────────────────────────────────────────────────────
.PHONY: all clean
CFLAGS = -Wall

OBJECTS = midi.o usbmidi.o

midi : $(OBJECTS)
	cc $(CFLAGS) -o midi $(OBJECTS) -lusb-1.0 -pthread

midi.o : midi.c usbmidi.h
usbmidi.o : usbmidi.c usbmidi.h

.PHONY : clean
clean :
	rm -rf *.o midi
