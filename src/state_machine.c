#include "state_machine.h"
#include "flow.h"
#include "../include/logger.h"
#include "../include/tcp_states.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

enum tcp_state state_transition(enum tcp_state current, uint8_t tcp_flags, int is_outgoing) {
    int syn = tcp_flags & 0x02;
    int ack = tcp_flags & 0x10;
    int fin = tcp_flags & 0x01;
    int rst = tcp_flags & 0x04;

    if (rst) {
        return TCP_STATE_CLOSED;
    }

    switch (current) {
        case TCP_STATE_CLOSED:
            if (is_outgoing && syn && !ack) {
                return TCP_STATE_SYN_SENT;
            } else if (!is_outgoing && syn && !ack) {
                return TCP_STATE_SYN_RCVD;
            }
            break;

        case TCP_STATE_SYN_SENT:
            if (!is_outgoing && syn && ack) {
                return TCP_STATE_ESTABLISHED;
            }
            break;

        case TCP_STATE_SYN_RCVD:
            if (is_outgoing && ack) {
                return TCP_STATE_ESTABLISHED;
            }
            break;

        case TCP_STATE_ESTABLISHED:
            if (is_outgoing && fin) {
                return TCP_STATE_FIN_WAIT1;
            } else if (!is_outgoing && fin) {
                return TCP_STATE_CLOSE_WAIT;
            }
            break;

        case TCP_STATE_FIN_WAIT1:
            if (!is_outgoing && fin && ack) {
                return TCP_STATE_TIME_WAIT;
            } else if (!is_outgoing && ack) {
                return TCP_STATE_FIN_WAIT2;
            }
            break;

        case TCP_STATE_FIN_WAIT2:
            if (!is_outgoing && fin) {
                return TCP_STATE_TIME_WAIT;
            }
            break;

        case TCP_STATE_CLOSE_WAIT:
            if (is_outgoing && fin) {
                return TCP_STATE_LAST_ACK;
            }
            break;

        case TCP_STATE_LAST_ACK:
            if (!is_outgoing && ack) {
                return TCP_STATE_CLOSED;
            }
            break;

        case TCP_STATE_TIME_WAIT:
            return TCP_STATE_CLOSED;

        default:
            break;
    }

    return current;
}

enum tcp_state state_transition_with_log(struct tcp_flow *flow, uint8_t tcp_flags, int is_outgoing) {
    if (!flow) {
        return state_transition(TCP_STATE_CLOSED, tcp_flags, is_outgoing);
    }

    enum tcp_state old_state = flow->state;
    enum tcp_state new_state = state_transition(old_state, tcp_flags, is_outgoing);

    if (new_state != old_state) {
        char flow_key[128];
        char details[256];

#ifdef _WIN32
        struct in_addr src_addr;
        struct in_addr dst_addr;
        src_addr.S_un.S_addr = htonl(flow->src_ip);
        dst_addr.S_un.S_addr = htonl(flow->dst_ip);
        char src_ip_str[INET_ADDRSTRLEN];
        char dst_ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src_addr, src_ip_str, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &dst_addr, dst_ip_str, INET_ADDRSTRLEN);
#else
        struct in_addr src_addr = { .s_addr = htonl(flow->src_ip) };
        struct in_addr dst_addr = { .s_addr = htonl(flow->dst_ip) };
        char src_ip_str[INET_ADDRSTRLEN];
        char dst_ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src_addr, src_ip_str, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &dst_addr, dst_ip_str, INET_ADDRSTRLEN);
#endif

        snprintf(flow_key, sizeof(flow_key), "%s:%u-%s:%u",
                 src_ip_str, flow->src_port, dst_ip_str, flow->dst_port);
        snprintf(details, sizeof(details), "%s -> %s",
                 state_to_string(old_state), state_to_string(new_state));

        log_event("STATE_CHANGE", flow_key, details,
                  flow->cc.cwnd_estimate, flow->cc.rtt_ms);
    }

    return new_state;
}