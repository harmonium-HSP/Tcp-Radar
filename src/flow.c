#include "flow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

#include "../include/tcp_states.h"
#include "../include/congestion_types.h"

flow_table_t g_flow_table;

static time_t default_get_time(void) {
    return time(NULL);
}

static time_t current_time_override = 0;

static uint32_t flow_hash_internal(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    uint32_t hash = 5381;
    hash = ((hash << 5) + hash) ^ src_ip;
    hash = ((hash << 5) + hash) ^ dst_ip;
    hash = ((hash << 5) + hash) ^ src_port;
    hash = ((hash << 5) + hash) ^ dst_port;
    return hash % FLOW_TABLE_SIZE;
}

static void normalize_flow(uint32_t *src_ip, uint32_t *dst_ip, uint16_t *src_port, uint16_t *dst_port) {
    if (*src_ip > *dst_ip || (*src_ip == *dst_ip && *src_port > *dst_port)) {
        uint32_t temp_ip = *src_ip;
        uint16_t temp_port = *src_port;
        *src_ip = *dst_ip;
        *src_port = *dst_port;
        *dst_ip = temp_ip;
        *dst_port = temp_port;
    }
}

void flow_init(void) {
    memset(&g_flow_table, 0, sizeof(g_flow_table));
    g_flow_table.get_time = default_get_time;
    current_time_override = 0;
}

static time_t get_current_time(void) {
    if (current_time_override != 0) {
        return current_time_override;
    }
    return default_get_time();
}

struct tcp_flow *flow_lookup_or_create(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    uint32_t n_src_ip = src_ip;
    uint32_t n_dst_ip = dst_ip;
    uint16_t n_src_port = src_port;
    uint16_t n_dst_port = dst_port;
    normalize_flow(&n_src_ip, &n_dst_ip, &n_src_port, &n_dst_port);

    uint32_t hash = flow_hash_internal(n_src_ip, n_dst_ip, n_src_port, n_dst_port);
    struct tcp_flow *current = g_flow_table.buckets[hash];

    while (current) {
        if (current->src_ip == n_src_ip && current->dst_ip == n_dst_ip &&
            current->src_port == n_src_port && current->dst_port == n_dst_port) {
            current->last_activity = get_current_time();
            return current;
        }
        current = current->next;
    }

    struct tcp_flow *new_flow = (struct tcp_flow *)malloc(sizeof(struct tcp_flow));
    if (!new_flow) {
        perror("malloc failed");
        return NULL;
    }

    memset(new_flow, 0, sizeof(struct tcp_flow));
    new_flow->src_ip = n_src_ip;
    new_flow->dst_ip = n_dst_ip;
    new_flow->src_port = n_src_port;
    new_flow->dst_port = n_dst_port;
    new_flow->state = TCP_STATE_CLOSED;
    new_flow->last_seq = 0;
    new_flow->last_ack = 0;
    new_flow->last_activity = get_current_time();
    new_flow->next = g_flow_table.buckets[hash];
    g_flow_table.buckets[hash] = new_flow;

    new_flow->cc.rto_ms = 1000;
    new_flow->cc.ssthresh_estimate = 0xFFFF;
    new_flow->cc.cwnd_estimate = MSS;

    return new_flow;
}

void flow_update_state(struct tcp_flow *flow, enum tcp_state new_state, uint32_t seq, uint32_t ack) {
    if (flow) {
        flow->state = new_state;
        flow->last_seq = seq;
        flow->last_ack = ack;
        flow->last_activity = get_current_time();
    }
}

struct tcp_flow *flow_lookup(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    uint32_t n_src_ip = src_ip;
    uint32_t n_dst_ip = dst_ip;
    uint16_t n_src_port = src_port;
    uint16_t n_dst_port = dst_port;
    normalize_flow(&n_src_ip, &n_dst_ip, &n_src_port, &n_dst_port);

    uint32_t hash = flow_hash_internal(n_src_ip, n_dst_ip, n_src_port, n_dst_port);
    struct tcp_flow *current = g_flow_table.buckets[hash];

    while (current) {
        if (current->src_ip == n_src_ip && current->dst_ip == n_dst_ip &&
            current->src_port == n_src_port && current->dst_port == n_dst_port) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void flow_delete(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    uint32_t n_src_ip = src_ip;
    uint32_t n_dst_ip = dst_ip;
    uint16_t n_src_port = src_port;
    uint16_t n_dst_port = dst_port;
    normalize_flow(&n_src_ip, &n_dst_ip, &n_src_port, &n_dst_port);

    uint32_t hash = flow_hash_internal(n_src_ip, n_dst_ip, n_src_port, n_dst_port);
    struct tcp_flow *current = g_flow_table.buckets[hash];
    struct tcp_flow *prev = NULL;

    while (current) {
        if (current->src_ip == n_src_ip && current->dst_ip == n_dst_ip &&
            current->src_port == n_src_port && current->dst_port == n_dst_port) {
            if (prev) {
                prev->next = current->next;
            } else {
                g_flow_table.buckets[hash] = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

uint32_t flow_hash(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    uint32_t n_src_ip = src_ip;
    uint32_t n_dst_ip = dst_ip;
    uint16_t n_src_port = src_port;
    uint16_t n_dst_port = dst_port;
    normalize_flow(&n_src_ip, &n_dst_ip, &n_src_port, &n_dst_port);
    return flow_hash_internal(n_src_ip, n_dst_ip, n_src_port, n_dst_port);
}

void flow_table_set_time_getter(time_t (*get_time)(void)) {
    g_flow_table.get_time = get_time;
}

time_t flow_table_get_current_time(void) {
    return get_current_time();
}

void flow_table_set_current_time(time_t time) {
    current_time_override = time;
}

void flow_table_get_stats(flow_stats_t *stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(flow_stats_t));
    
    for (int i = 0; i < FLOW_TABLE_SIZE; i++) {
        struct tcp_flow *current = g_flow_table.buckets[i];
        while (current) {
            stats->total_flows++;
            if (current->state != TCP_STATE_CLOSED) {
                stats->active_flows++;
            }
            current = current->next;
        }
    }
}

void flow_cleanup_timeout(time_t current_time, int timeout_seconds) {
    for (int i = 0; i < FLOW_TABLE_SIZE; i++) {
        struct tcp_flow *current = g_flow_table.buckets[i];
        struct tcp_flow *prev = NULL;

        while (current) {
            if (current->last_activity > 0 &&
                (current_time - current->last_activity) > timeout_seconds) {
                struct tcp_flow *to_delete = current;
                if (prev) {
                    prev->next = current->next;
                } else {
                    g_flow_table.buckets[i] = current->next;
                }
                current = current->next;
                free(to_delete);
            } else {
                prev = current;
                current = current->next;
            }
        }
    }
}

void flow_print(struct tcp_flow *flow) {
    if (!flow) return;

    struct in_addr src_addr, dst_addr;
    src_addr.s_addr = htonl(flow->src_ip);
    dst_addr.s_addr = htonl(flow->dst_ip);

    char src_ip_str[INET_ADDRSTRLEN];
    char dst_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src_addr, src_ip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &dst_addr, dst_ip_str, INET_ADDRSTRLEN);

    printf("%s:%u <-> %s:%u | state=%s\n",
           src_ip_str, flow->src_port,
           dst_ip_str, flow->dst_port,
           state_to_string(flow->state));
}

#ifdef UNIT_TESTING
void flow_clear_all(void) {
    for (int i = 0; i < FLOW_TABLE_SIZE; i++) {
        struct tcp_flow *current = g_flow_table.buckets[i];
        while (current) {
            struct tcp_flow *next = current->next;
            free(current);
            current = next;
        }
        g_flow_table.buckets[i] = NULL;
    }
}
#endif