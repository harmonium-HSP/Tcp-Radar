#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stddef.h>
#include "../include/protocol.h"

int parse_ethernet_header(const uint8_t *packet, size_t packet_len, struct ethernet_header *eth);
int parse_ip_header(const uint8_t *packet, size_t packet_len, struct ip_header *ip);
int parse_tcp_header(const uint8_t *packet, size_t packet_len, struct tcp_header *tcp);
int parse_packet(const uint8_t *packet, size_t packet_len,
                 struct ethernet_header *eth,
                 struct ip_header *ip,
                 struct tcp_header *tcp);

#endif