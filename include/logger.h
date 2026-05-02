#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stddef.h>

void log_init(void);
void log_event(const char *event_type, const char *flow_key,
               const char *details, uint32_t cwnd, int rtt_ms);
void log_close(void);

#ifdef UNIT_TESTING
void log_event(const char *event_type, const char *flow_key,
               const char *details, uint32_t cwnd, int rtt_ms);
#endif

#endif