#include "congestion.h"
#include <stdio.h>
#include <string.h>

#include "websocket.h"
#include "json.h"
#include "../include/logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/time.h>
#include <arpa/inet.h>
#endif

#include "../include/congestion_types.h"
#include "../include/tcp_states.h"

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

static void log_loss_event(struct tcp_flow *flow, const char *event_type, const char *details) {
    if (!flow) return;

    char flow_key[128];
    struct in_addr src_addr = { .s_addr = htonl(flow->src_ip) };
    struct in_addr dst_addr = { .s_addr = htonl(flow->dst_ip) };
    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr, src_ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &dst_addr, dst_ip_str, INET_ADDRSTRLEN);

    snprintf(flow_key, sizeof(flow_key), "%s:%u-%s:%u",
             src_ip_str, flow->src_port, dst_ip_str, flow->dst_port);

    log_event(event_type, flow_key, details,
              flow->cc.cwnd_estimate, flow->cc.rtt_ms);
}

void cc_init_flow(struct tcp_flow *flow) {
    if (!flow) return;
    
    memset(&flow->cc, 0, sizeof(flow->cc));
    flow->cc.rto_ms = 1000;
    flow->cc.ssthresh_estimate = 0xFFFF;
}

void cc_on_data_send(struct tcp_flow *flow, uint32_t seq, uint32_t len, const void *now) {
    if (!flow || len == 0) return;

    flow->cc.last_sent_seq = seq + len - 1;

    uint8_t idx = flow->cc.sent_history_head;
    flow->cc.sent_history[idx].seq = seq;
    flow->cc.sent_history[idx].len = len;
#ifdef _WIN32
    flow->cc.sent_history[idx].send_time = *(struct timeb *)now;
#else
    flow->cc.sent_history[idx].send_time = *(struct timeval *)now;
#endif
    flow->cc.sent_history[idx].retransmitted = 0;
    flow->cc.sent_history[idx].valid = 1;

    flow->cc.sent_history_head = (flow->cc.sent_history_head + 1) % SENT_HISTORY_SIZE;

    int32_t diff = (int32_t)(flow->cc.last_sent_seq - flow->cc.last_acked_seq);
    if (diff > 0) {
        flow->cc.inflight_bytes = (uint32_t)diff;
    } else {
        flow->cc.inflight_bytes = 0;
    }

#ifdef _WIN32
    flow->cc.sent_time = *(struct timeb *)now;
#else
    flow->cc.sent_time = *(struct timeval *)now;
#endif
}

void cc_on_ack_receive(struct tcp_flow *flow, uint32_t ack_seq, const void *now) {
    if (!flow) return;

    if (ack_seq == flow->cc.dup_ack_seq) {
        flow->cc.dup_ack_count++;
    } else {
        flow->cc.dup_ack_count = 1;
        flow->cc.dup_ack_seq = ack_seq;
    }

    for (int i = 0; i < SENT_HISTORY_SIZE; i++) {
        if (flow->cc.sent_history[i].valid && 
            flow->cc.sent_history[i].seq + flow->cc.sent_history[i].len - 1 < ack_seq) {
            uint32_t rtt_ms = 0;
#ifdef _WIN32
            const struct timeb *now_b = (const struct timeb *)now;
            int64_t rtt_ms_int = (now_b->time - flow->cc.sent_history[i].send_time.time) * 1000 +
                               (now_b->millitm - flow->cc.sent_history[i].send_time.millitm);
            rtt_ms = (uint32_t)rtt_ms_int;
#else
            const struct timeval *now_tv = (const struct timeval *)now;
            int64_t rtt_us = (now_tv->tv_sec - flow->cc.sent_history[i].send_time.tv_sec) * 1000000 +
                             (now_tv->tv_usec - flow->cc.sent_history[i].send_time.tv_usec);
            rtt_ms = (uint32_t)(rtt_us / 1000);
#endif

            if (rtt_ms > 0 && flow->cc.rtt_ms == 0) {
                flow->cc.rtt_ms = rtt_ms;
                flow->cc.rtt_var_ms = rtt_ms / 2;
            } else if (rtt_ms > 0) {
                uint32_t delta = (rtt_ms > flow->cc.rtt_ms) ? 
                                 (rtt_ms - flow->cc.rtt_ms) : 
                                 (flow->cc.rtt_ms - rtt_ms);
                flow->cc.rtt_var_ms = (uint32_t)((1 - BETA) * flow->cc.rtt_var_ms + BETA * delta);
                flow->cc.rtt_ms = (uint32_t)((1 - ALPHA) * flow->cc.rtt_ms + ALPHA * rtt_ms);
            }

            flow->cc.rto_ms = flow->cc.rtt_ms + 4 * flow->cc.rtt_var_ms;
            if (flow->cc.rto_ms < 1000) flow->cc.rto_ms = 1000;

            flow->cc.sent_history[i].valid = 0;
        }
    }

    int32_t diff = (int32_t)(flow->cc.last_sent_seq - ack_seq);
    if (diff > 0) {
        flow->cc.inflight_bytes = (uint32_t)diff;
    } else {
        flow->cc.inflight_bytes = 0;
    }

    flow->cc.last_acked_seq = ack_seq;
}

void cc_on_retransmit(struct tcp_flow *flow, uint32_t seq) {
    if (!flow) return;
    flow->cc.retrans_count++;

    for (int i = 0; i < SENT_HISTORY_SIZE; i++) {
        if (flow->cc.sent_history[i].valid && 
            flow->cc.sent_history[i].seq == seq) {
            flow->cc.sent_history[i].retransmitted = 1;
            break;
        }
    }
}

void cc_update_cwnd_estimate(struct tcp_flow *flow) {
    if (!flow) return;

    if (flow->cc.inflight_bytes > flow->cc.cwnd_estimate) {
        flow->cc.cwnd_estimate = flow->cc.inflight_bytes;
    }

    if (flow->cc.cwnd_estimate == 0) {
        flow->cc.cwnd_estimate = MSS;
    }

    if (flow->cc.cwnd_estimate < flow->cc.ssthresh_estimate) {
        flow->cc.cwnd_estimate += MSS;
    } else {
        uint32_t increment = (MSS * MSS) / flow->cc.cwnd_estimate;
        if (increment < 1) increment = 1;
        flow->cc.cwnd_estimate += increment;
    }
}

int cc_detect_loss(struct tcp_flow *flow, uint32_t ack_seq) {
    if (!flow) return 0;

    if (flow->cc.dup_ack_count >= 3) {
        flow->cc.dup_ack_count = 0;
        log_loss_event(flow, "LOSS_DETECTED", "DUP_ACK");
        return 1;
    }

#ifdef _WIN32
    struct timeb now;
    ftime(&now);

    for (int i = 0; i < SENT_HISTORY_SIZE; i++) {
        if (flow->cc.sent_history[i].valid &&
            !flow->cc.sent_history[i].retransmitted &&
            flow->cc.sent_history[i].seq + flow->cc.sent_history[i].len - 1 >= ack_seq) {
            int64_t elapsed_ms = (now.time - flow->cc.sent_history[i].send_time.time) * 1000 +
                               (now.millitm - flow->cc.sent_history[i].send_time.millitm);

            if (elapsed_ms > (int64_t)flow->cc.rto_ms) {
                log_loss_event(flow, "LOSS_DETECTED", "TIMEOUT");
                return 2;
            }
        }
    }
#else
    struct timeval now;
    gettimeofday(&now, NULL);

    for (int i = 0; i < SENT_HISTORY_SIZE; i++) {
        if (flow->cc.sent_history[i].valid &&
            !flow->cc.sent_history[i].retransmitted &&
            flow->cc.sent_history[i].seq + flow->cc.sent_history[i].len - 1 >= ack_seq) {
            int64_t elapsed_us = (now.tv_sec - flow->cc.sent_history[i].send_time.tv_sec) * 1000000 +
                                 (now.tv_usec - flow->cc.sent_history[i].send_time.tv_usec);
            int64_t elapsed_ms = elapsed_us / 1000;

            if (elapsed_ms > (int64_t)flow->cc.rto_ms) {
                log_loss_event(flow, "LOSS_DETECTED", "TIMEOUT");
                return 2;
            }
        }
    }
#endif

    return 0;
}

void flow_print_cc(struct tcp_flow *flow) {
    if (!flow) return;

    struct in_addr src_addr, dst_addr;
    src_addr.s_addr = htonl(flow->src_ip);
    dst_addr.s_addr = htonl(flow->dst_ip);

    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr, src_ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &dst_addr, dst_ip_str, INET_ADDRSTRLEN);

    printf("[FLOW] %s:%u -> %s:%u\n", src_ip_str, flow->src_port, dst_ip_str, flow->dst_port);
    printf("       inflight=%u cwnd_est=%u rtt=%ums retrans=%u\n",
           flow->cc.inflight_bytes,
           flow->cc.cwnd_estimate,
           flow->cc.rtt_ms,
           flow->cc.retrans_count);

    char json_buffer[1024];
    serialize_flow_metrics(json_buffer, sizeof(json_buffer), flow, NULL, 0);
    ws_broadcast(json_buffer, strlen(json_buffer));
}