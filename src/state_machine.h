#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "../include/tcp_states.h"
#include "flow.h"

enum tcp_state state_transition(enum tcp_state current, uint8_t tcp_flags, int is_outgoing);
enum tcp_state state_transition_with_log(struct tcp_flow *flow, uint8_t tcp_flags, int is_outgoing);

#endif