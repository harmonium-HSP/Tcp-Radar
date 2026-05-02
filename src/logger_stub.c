#include <stdio.h>
#include <stdlib.h>

#ifdef UNIT_TESTING
void log_event(const char *event_type, const char *flow_key,
               const char *details, uint32_t cwnd, int rtt_ms) {
    (void)event_type;
    (void)flow_key;
    (void)details;
    (void)cwnd;
    (void)rtt_ms;
}
#endif