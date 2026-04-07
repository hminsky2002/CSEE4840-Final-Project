CFLAGS = -Wall

OBJECTS = midi.o usbmidi.o

midi : $(OBJECTS)
	cc $(CFLAGS) -o midi $(OBJECTS) -lusb-1.0 -pthread

midi.o : midi.c usbmidi.h
usbmidi.o : usbmidi.c usbmidi.h

.PHONY : clean
clean :
	rm -rf *.o midi
