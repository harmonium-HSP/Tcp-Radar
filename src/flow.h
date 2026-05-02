#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/time.h>
#endif

#include "../include/tcp_states.h"
#include "../include/congestion_types.h"

#define FLOW_TABLE_SIZE 1024

struct tcp_flow {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    enum tcp_state state;
    uint32_t last_seq;
    uint32_t last_ack;
    time_t last_activity;
    struct tcp_flow *next;

    struct {
        uint32_t last_sent_seq;
        uint32_t last_acked_seq;
        uint32_t inflight_bytes;

#ifdef _WIN32
        struct timeb sent_time;
#else
        struct timeval sent_time;
#endif
        uint32_t rtt_ms;
        uint32_t rtt_var_ms;
        uint32_t rto_ms;

        uint32_t retrans_count;
        uint8_t dup_ack_count;
        uint32_t dup_ack_seq;

        uint32_t cwnd_estimate;
        uint32_t ssthresh_estimate;

        struct sent_packet sent_history[SENT_HISTORY_SIZE];
        uint8_t sent_history_head;
        uint8_t sent_history_tail;
    } cc;
};

typedef struct {
    struct tcp_flow *buckets[FLOW_TABLE_SIZE];
    time_t (*get_time)(void);
} flow_table_t;

extern flow_table_t g_flow_table;

void flow_init(void);
struct tcp_flow *flow_lookup_or_create(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
struct tcp_flow *flow_lookup(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
void flow_delete(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
void flow_update_state(struct tcp_flow *flow, enum tcp_state new_state, uint32_t seq, uint32_t ack);
void flow_cleanup_timeout(time_t current_time, int timeout_seconds);
void flow_print(struct tcp_flow *flow);
void flow_print_cc(struct tcp_flow *flow);

uint32_t flow_hash(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);

void flow_table_set_time_getter(time_t (*get_time)(void));
time_t flow_table_get_current_time(void);
void flow_table_set_current_time(time_t time);

typedef struct {
    size_t total_flows;
    size_t active_flows;
} flow_stats_t;
void flow_table_get_stats(flow_stats_t *stats);

#ifdef UNIT_TESTING
void flow_clear_all(void);
#endif

#endif