# COMPILER SETTINGS
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm -lusb-1.0 -pthread

ifdef WAVETABLE_FILE
# usage: make WAVETABLE_FILE=/path/to/wavetable.bin 
	CFLAGS += -DWAVETABLE_FILE='"$(WAVETABLE_FILE)"'      
endif

# TARGETS
TARGET             = midi_to_fpga
MIDI_USB_TEST      = midi_to_usb_test
BRINGUP_TEST       = test_bringup

# SOURCES
SRCS = midi_to_fpga.c wavetable.c midi.c fpga_bridge.c
OBJS = $(SRCS:.c=.o)
DEPS = midi_to_fpga.h wavetable.h midi.h oscillator.h fpga_bridge.h

MIDI_USB_TEST_OBJS = midi_to_usb_test.o midi.o fpga_bridge.o
BRINGUP_TEST_OBJS  = test_bringup.o

# DEFAULT TARGET
all: $(TARGET) $(MIDI_USB_TEST) $(BRINGUP_TEST)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(MIDI_USB_TEST): $(MIDI_USB_TEST_OBJS)
	$(CC) $(CFLAGS) $(MIDI_USB_TEST_OBJS) -o $(MIDI_USB_TEST) $(LDFLAGS)

$(BRINGUP_TEST): $(BRINGUP_TEST_OBJS)
	$(CC) $(CFLAGS) $(BRINGUP_TEST_OBJS) -o $(BRINGUP_TEST) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(MIDI_USB_TEST_OBJS) $(BRINGUP_TEST_OBJS) \
	      $(TARGET) $(MIDI_USB_TEST) $(BRINGUP_TEST)

# PHONY TARGET
.PHONY: all clean