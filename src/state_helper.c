#include "state_helper.h"
#include "../include/tcp_states.h"
#include <stdio.h>
#include <string.h>

const char *state_to_string(enum tcp_state state) {
    switch (state) {
        case TCP_STATE_CLOSED:      return "CLOSED";
        case TCP_STATE_LISTEN:       return "LISTEN";
        case TCP_STATE_SYN_SENT:     return "SYN_SENT";
        case TCP_STATE_SYN_RCVD:     return "SYN_RCVD";
        case TCP_STATE_ESTABLISHED:  return "ESTABLISHED";
        case TCP_STATE_FIN_WAIT1:    return "FIN_WAIT1";
        case TCP_STATE_FIN_WAIT2:    return "FIN_WAIT2";
        case TCP_STATE_CLOSE_WAIT:   return "CLOSE_WAIT";
        case TCP_STATE_LAST_ACK:     return "LAST_ACK";
        case TCP_STATE_TIME_WAIT:    return "TIME_WAIT";
        default:                      return "UNKNOWN";
    }
}

const char *flags_to_string(uint8_t flags) {
    static char buf[32];
    int pos = 0;
    buf[0] = '\0';
    
    if (flags & 0x01) pos += snprintf(buf + pos, sizeof(buf) - pos, "FIN ");
    if (flags & 0x02) pos += snprintf(buf + pos, sizeof(buf) - pos, "SYN ");
    if (flags & 0x04) pos += snprintf(buf + pos, sizeof(buf) - pos, "RST ");
    if (flags & 0x10) pos += snprintf(buf + pos, sizeof(buf) - pos, "ACK ");
    if (flags & 0x08) pos += snprintf(buf + pos, sizeof(buf) - pos, "PSH ");
    if (flags & 0x20) pos += snprintf(buf + pos, sizeof(buf) - pos, "URG ");
    
    return buf;
}