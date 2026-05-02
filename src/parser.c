#include "parser.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <string.h>

int parse_ethernet_header(const uint8_t *packet, size_t packet_len, struct ethernet_header *eth) {
    if (!packet || !eth || packet_len < sizeof(struct ethernet_header)) {
        return -1;
    }
    
    memcpy(eth->dst_mac, packet, 6);
    memcpy(eth->src_mac, packet + 6, 6);
    eth->ethertype = (packet[12] << 8) | packet[13];
    
    return 0;
}

int parse_ip_header(const uint8_t *packet, size_t packet_len, struct ip_header *ip) {
    if (!packet || !ip || packet_len < sizeof(struct ip_header)) {
        return -1;
    }
    
    memcpy(ip, packet, sizeof(struct ip_header));
    
    return 0;
}

int parse_tcp_header(const uint8_t *packet, size_t packet_len, struct tcp_header *tcp) {
    if (!packet || !tcp || packet_len < sizeof(struct tcp_header)) {
        return -1;
    }
    
    memcpy(tcp, packet, sizeof(struct tcp_header));
    
    return 0;
}

int parse_packet(const uint8_t *packet, size_t packet_len,
                 struct ethernet_header *eth,
                 struct ip_header *ip,
                 struct tcp_header *tcp) {
    if (parse_ethernet_header(packet, packet_len, eth) != 0) {
        return -1;
    }
    
    if (eth->ethertype != ETHERTYPE_IP) {
        return -1;
    }
    
    if (parse_ip_header(packet + sizeof(struct ethernet_header), 
                       packet_len - sizeof(struct ethernet_header), ip) != 0) {
        return -1;
    }
    
    if (ip->protocol != IP_PROTOCOL_TCP) {
        return -1;
    }
    
    if (parse_tcp_header(packet + sizeof(struct ethernet_header) + sizeof(struct ip_header),
                        packet_len - sizeof(struct ethernet_header) - sizeof(struct ip_header),
                        tcp) != 0) {
        return -1;
    }
    
    return 0;
}