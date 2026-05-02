#ifndef CONGESTION_TYPES_H
#define CONGESTION_TYPES_H

#include <stdint.h>

#ifdef _WIN32
#include <time.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#define SENT_HISTORY_SIZE 256
#define MSS 1460
#define ALPHA 0.125f
#define BETA 0.25f

struct sent_packet {
    uint32_t seq;
    uint32_t len;
#ifdef _WIN32
    struct timeb send_time;
#else
    struct timeval send_time;
#endif
    uint8_t retransmitted;
    uint8_t valid;
};

#endif