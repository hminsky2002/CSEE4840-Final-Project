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
TEST_TARGET = test_bench

# SOURCES
SRCS = midi_to_fpga.c wavetable.c midi.c usbmidi.c
OBJS = $(SRCS:.c=.o)
DEPS = midi_to_fpga.h wavetable.h midi.h usbmidi.h oscillator.h

TEST_SRCS = test_bench.c midi.c usbmidi.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

# DEFAULT TARGET
all: $(TARGET) $(TEST_TARGET)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) $(TEST_OBJS) -o $(TEST_TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)

# PHONY TARGET
.PHONY: all clean