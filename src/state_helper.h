#ifndef STATE_HELPER_H
#define STATE_HELPER_H

#include "../include/tcp_states.h"

const char *state_to_string(enum tcp_state state);
const char *flags_to_string(uint8_t flags);

#endif