# COMPILER SETTINGS
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm -pthread

ifdef WAVETABLE_FILE
# usage: make WAVETABLE_FILE=/path/to/wavetable.bin 
	CFLAGS += -DWAVETABLE_FILE='"$(WAVETABLE_FILE)"'      
endif

# TARGET
TARGET  = midi_to_fpga hex_test

# SOURCES
SRCS = midi_to_fpga.c hex_test.c wavetable.c midi.c fpga_bridge.c synth_functions.c
OBJS = $(SRCS:.c=.o)
DEPS = wavetable.h midi.h fpga_bridge.h synth_functions.h

# DEFAULT TARGET
all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

# PHONY TARGET
.PHONY: all clean