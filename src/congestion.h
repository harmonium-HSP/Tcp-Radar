#ifndef CONGESTION_H
#define CONGESTION_H

#include <stdint.h>
#include "flow.h"

void cc_on_data_send(struct tcp_flow *flow, uint32_t seq, uint32_t len, const void *now);
void cc_on_ack_receive(struct tcp_flow *flow, uint32_t ack_seq, const void *now);
void cc_on_retransmit(struct tcp_flow *flow, uint32_t seq);
void cc_update_cwnd_estimate(struct tcp_flow *flow);
int cc_detect_loss(struct tcp_flow *flow, uint32_t ack_seq);
void cc_init_flow(struct tcp_flow *flow);

#endif