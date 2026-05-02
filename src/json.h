#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <time.h>
#include "flow.h"

void json_escape(char *out, const char *in, size_t out_size);
void serialize_flow_metrics(char *buf, size_t buf_size, const struct tcp_flow *flow, const char *events[], int event_count);

#endif