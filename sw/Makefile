# COMPILER SETTINGS
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm -lusb-1.0 -pthread

ifdef WAVETABLE_FILE
# usage: make WAVETABLE_FILE=/path/to/wavetable.bin 
	CFLAGS += -DWAVETABLE_FILE='"$(WAVETABLE_FILE)"'      
endif

# TARGET
TARGET      = midi_to_fpga

# SOURCES
SRCS = midi_to_fpga.c wavetable.c midi.c fpga_bridge.c
OBJS = $(SRCS:.c=.o)
DEPS = midi_to_fpga.h wavetable.h midi.h oscillator.h fpga_bridge.h

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