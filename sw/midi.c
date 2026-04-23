/*
 *
 * CSEE 4840 Final Project
 *
 * Name/UNI: Henry Minsky (hm3121), Opalina Khanna (ok2373), Sunny Fang (yf2610)
 */
#include "midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define LIBUSB_CLASS_AUDIO 1
#define MIDI_STREAMING     3

/*
 * Find and open a USB MIDI device; writes the IN endpoint address to
 * *endpoint_out and returns the claimed device handle (or NULL on failure).
 */
struct libusb_device_handle *midi_open(uint8_t *endpoint_out) {
    libusb_device **devs;
    struct libusb_device_handle *midi = NULL;
    struct libusb_device_descriptor desc;
    ssize_t num_devs, d;
    uint8_t i, k;

    if (libusb_init(NULL) < 0) {
        fprintf(stderr, "Error: libusb_init failed\n");
        exit(1);
    }

    if ((num_devs = libusb_get_device_list(NULL, &devs)) < 0) {
        fprintf(stderr, "Error: libusb_get_device_list failed\n");
        exit(1);
    }

    for (d = 0; d < num_devs; d++) {
        libusb_device *dev = devs[d];
        if (libusb_get_device_descriptor(dev, &desc) < 0) {
            fprintf(stderr, "Error: libusb_get_device_descriptor failed\n");
            exit(1);
        }

        if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
            struct libusb_config_descriptor *config;
            libusb_get_config_descriptor(dev, 0, &config);
            for (i = 0; i < config->bNumInterfaces; i++) {
                for (k = 0; k < config->interface[i].num_altsetting; k++) {
                    const struct libusb_interface_descriptor *inter =
                        config->interface[i].altsetting + k;
                    if (inter->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                        inter->bInterfaceSubClass == MIDI_STREAMING) {
                        int r;
                        if ((r = libusb_open(dev, &midi)) != 0) {
                            fprintf(stderr, "Error: libusb_open failed: %d\n", r);
                            exit(1);
                        }
                        if (libusb_kernel_driver_active(midi, i)) {
                            libusb_detach_kernel_driver(midi, i);
                        }
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
                        if (!ep) {
                            continue;
                        }
                        *endpoint_out = ep->bEndpointAddress;
                        goto found;
                    }
                }
            }
        }
    }

 found:
    libusb_free_device_list(devs, 1);
    return midi;
}
int midi_read(struct libusb_device_handle *midi,
              uint8_t endpoint_address,
              midi_event_t *evt) {
    int transferred;
    midi_event_t[4] buf;

    for (;;) {
        memset(evt, 0, sizeof(*evt));
        int r = libusb_bulk_transfer(midi, endpoint_address,
                                     (unsigned char *)buf, 4 * sizeof(midi_event_t),
                                     &transferred, 1000);
        if (r != 0) {
            continue;
        }
        if (transferred < 4) {
            continue;  
        }
        break;
    }
    evt = buf;

    printf("transferred %d bytes\n", transferred);
    printf("%02x %02x %02x %02x\n",
           evt->status,
           evt->not_sure,
           evt->note,
           evt->attack);

    return 0;
}

void midi_close(struct libusb_device_handle *midi) {
    libusb_close(midi);
    libusb_exit(NULL);
}
