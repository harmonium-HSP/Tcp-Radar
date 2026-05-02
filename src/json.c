#include "json.h"
#include "flow.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

void json_escape(char *out, const char *in, size_t out_size) {
    size_t pos = 0;
    while (*in && pos < out_size - 1) {
        if (*in == '"' || *in == '\\') {
            if (pos < out_size - 2) {
                out[pos++] = '\\';
                out[pos++] = *in;
            }
        } else {
            out[pos++] = *in;
        }
        in++;
    }
    out[pos] = '\0';
}

void serialize_flow_metrics(char *buf, size_t buf_size, const struct tcp_flow *flow, const char *events[], int event_count) {
    if (!flow || !buf) {
        snprintf(buf, buf_size, "{}");
        return;
    }

    struct in_addr src_addr, dst_addr;
    src_addr.s_addr = htonl(flow->src_ip);
    dst_addr.s_addr = htonl(flow->dst_ip);

    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr, src_ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &dst_addr, dst_ip_str, INET_ADDRSTRLEN);

    time_t now = time(NULL);

    int written = snprintf(buf, buf_size,
        "{"
        "\"flow_id\":\"%s:%u->%s:%u\","
        "\"timestamp\":%ld,"
        "\"state\":\"%s\","
        "\"inflight_bytes\":%u,"
        "\"cwnd_estimate\":%u,"
        "\"rtt_ms\":%u,"
        "\"retrans_count\":%u,"
        "\"src_ip\":\"%s\","
        "\"dst_ip\":\"%s\","
        "\"src_port\":%u,"
        "\"dst_port\":%u",
        src_ip_str, flow->src_port,
        dst_ip_str, flow->dst_port,
        (long)now,
        state_to_string(flow->state),
        flow->cc.inflight_bytes,
        flow->cc.cwnd_estimate,
        flow->cc.rtt_ms,
        flow->cc.retrans_count,
        src_ip_str,
        dst_ip_str,
        flow->src_port,
        flow->dst_port
    );

    if (event_count > 0 && events && written < (int)buf_size) {
        written += snprintf(buf + written, buf_size - written, ",\"events\":[");
        for (int i = 0; i < event_count && written < (int)buf_size; i++) {
            if (i > 0) {
                written += snprintf(buf + written, buf_size - written, ",");
            }
            written += snprintf(buf + written, buf_size - written, "\"%s\"", events[i] ? events[i] : "");
        }
        if (written < (int)buf_size) {
            written += snprintf(buf + written, buf_size - written, "]");
        }
    }

    if (written < (int)buf_size) {
        snprintf(buf + written, buf_size - written, "}");
    }
}