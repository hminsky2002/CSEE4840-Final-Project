/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Henry Minsky (hm3121), Opalina Khanna (ok2373), Sunny Fang (yf2610)
 */
#include <assert.h>
#include "usbmidi.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128
#define INPUT_BUFFER_SIZE 191


// // In functions:
// void* thread_func(void* arg) {
//     pthread_mutex_lock(&my_mutex);
//     // Critical section
//     pthread_mutex_unlock(&my_mutex);
//     return NULL;
// }
/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 *
 */

struct libusb_device_handle *midi;
uint8_t endpoint_address;

void *network_thread_f(void *);

int main() {
  int err;

  struct usb_midi_input packet;
  uint8_t buf[64];
  int transferred;

  /* open the MIDI controller */
  if ((midi = open_midi(&endpoint_address)) == NULL) {
    fprintf(stderr, "Did not find a midi controller\n");
    exit(1);
  }

  // static struct usb_midi_input prev_packet = {0};
  for (;;) {
    int r = libusb_bulk_transfer(midi, endpoint_address,
                          (unsigned char *)&packet, sizeof(packet),
                          &transferred, 500000); 

    // printf("endpoint address: 0x%02x\n", endpoint_address);

    if (r < 0) {
    fprintf(stderr, "bulk_transfer error: %s\n", libusb_error_name(r));
    continue;
    }
    printf("%02x %02x %02x %02x\n",
        ((uint8_t*)&packet)[0],
        ((uint8_t*)&packet)[1],
        ((uint8_t*)&packet)[2],
        ((uint8_t*)&packet)[3]);
  }

  return 0;
}
