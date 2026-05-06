# COMPILER SETTINGS
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm -pthread

ifdef WAVETABLE_FILE
# usage: make WAVETABLE_FILE=/path/to/wavetable.bin 
	CFLAGS += -DWAVETABLE_FILE='"$(WAVETABLE_FILE)"'      
endif

# TARGETS
TARGET  = midi_to_fpga hex_test

# SOURCES AND HEADERS
DEPS = wavetable.h midi.h fpga_bridge.h synth_functions.h

# midi_to_fpga sources
MIDI_SRCS = midi_to_fpga.c wavetable.c midi.c fpga_bridge.c synth_functions.c
MIDI_OBJS = $(MIDI_SRCS:.c=.o)

# hex_test sources
HEX_SRCS = hex_test.c
HEX_OBJS = $(HEX_SRCS:.c=.o)

# DEFAULT TARGET
all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

midi_to_fpga: $(MIDI_OBJS)
	$(CC) $(CFLAGS) $(MIDI_OBJS) -o $@ $(LDFLAGS)

hex_test: $(HEX_OBJS)
	$(CC) $(CFLAGS) $(HEX_OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(MIDI_OBJS) $(HEX_OBJS) $(TARGET)

# PHONY TARGET
.PHONY: all clean