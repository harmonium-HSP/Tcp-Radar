#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNIT_TESTING
void ws_broadcast(const char *data, size_t len) {
    (void)data;
    (void)len;
}

void serialize_flow_metrics(char *buffer, size_t size, const struct tcp_flow *flow, const char *events[], int event_count) {
    (void)flow;
    (void)events;
    (void)event_count;
    if (buffer && size > 0) {
        snprintf(buffer, size, "{}");
    }
}
#endif