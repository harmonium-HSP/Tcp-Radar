#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*packet_callback_t)(const uint8_t *packet, size_t len);

bool capture_init(const char *interface, packet_callback_t callback);
bool capture_start(void);
bool capture_stop(void);

#endif