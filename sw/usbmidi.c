#include "usbmidi.h"

#include <stdio.h>
#include <stdlib.h> 

/* References on libusb 1.0 and the USB HID/midi protocol
 *
 * http://libusb.org
 * https://web.archive.org/web/20210302095553/https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
 *
 * https://www.usb.org/sites/default/files/documents/hid1_11.pdf
 *
 * https://usb.org/sites/default/files/hut1_5.pdf
 */

/*
 * Find and return a USB midi device or NULL if not found
 * The argument con
 * 
 */
struct libusb_device_handle *open_midi(uint8_t *endpoint_address) {
  libusb_device **devs;
  struct libusb_device_handle *midi = NULL;
  struct libusb_device_descriptor desc;
  ssize_t num_devs, d;
  uint8_t i, k;
  
  /* Start the library */
  if ( libusb_init(NULL) < 0 ) {
    fprintf(stderr, "Error: libusb_init failed\n");
    exit(1);
  }

  /* Enumerate all the attached USB devices */
  if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
    fprintf(stderr, "Error: libusb_get_device_list failed\n");
    exit(1);
  }

  /* Look at each device, remembering the first HID device that speaks
     the midi protocol */
  for (d = 0 ; d < num_devs ; d++) {
    libusb_device *dev = devs[d];
    if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
      fprintf(stderr, "Error: libusb_get_device_descriptor failed\n");
      exit(1);
    }

    if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
      struct libusb_config_descriptor *config;
      libusb_get_config_descriptor(dev, 0, &config);
      for (i = 0 ; i < config->bNumInterfaces ; i++)	       
	for ( k = 0 ; k < config->interface[i].num_altsetting ; k++ ) {
	  const struct libusb_interface_descriptor *inter =
	    config->interface[i].altsetting + k ;
	  if ( inter->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
	       inter->bInterfaceSubClass == MIDI_STREAMING) {
	    int r;
	    if ((r = libusb_open(dev, &midi)) != 0) {
	      fprintf(stderr, "Error: libusb_open failed: %d\n", r);
	      exit(1);
	    }
	    if (libusb_kernel_driver_active(midi,i))
	      libusb_detach_kernel_driver(midi, i);
	    libusb_set_auto_detach_kernel_driver(midi, i);
	    if ((r = libusb_claim_interface(midi, i)) != 0) {
	      fprintf(stderr, "Error: libusb_claim_interface failed: %d\n", r);
	      exit(1);
	    }
	    const struct libusb_endpoint_descriptor *ep = NULL;
        for (int e = 0; e < inter->bNumEndpoints; e++) {
            if ((inter->endpoint[e].bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN) {
                ep = &inter->endpoint[e];
                break;
            }
        }
        if (!ep) continue;
        *endpoint_address = ep->bEndpointAddress;
	    goto found;
	  }
	}
    }
  }

 found:
  libusb_free_device_list(devs, 1);

  return midi;
}
