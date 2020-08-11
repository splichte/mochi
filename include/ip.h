#pragma once

#include "kalloc.h"

// https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

typedef struct {
    uint8_t version_ihl;
    uint8_t dscp_ecn;
    uint16_t total_len;
    uint16_t ident;
    uint16_t flags_frag_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} ip_hdr;

int send_pkt_to_ip(uint8_t *tpkt, uint16_t tlen, uint8_t protocol);

// could be TCP, UDP
uint8_t *recv_pkt_from_ip(uint8_t *protocol);
