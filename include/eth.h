#pragma once

#include "ip.h"

// https://en.wikipedia.org/wiki/EtherType
#define ETHERTYPE_IPV4  0x0800
#define ETHERTYPE_ARP   0x0806
#define ETHERTYPE_IPV6  0x86DD

#define ETH_MAX_PKT_LEN 1518 // bytes
#define ETH_MAX_PAYLOAD_LEN 1500

typedef struct {
    uint8_t mac_dest[6];
    uint8_t mac_src[6];
    uint16_t ethertype;
    uint8_t buf[ETH_MAX_PAYLOAD_LEN];
} eth_pkt;

int send_ip_pkt_to_eth(uint8_t *pkt, uint32_t len);

uint8_t *recv_ip_pkt_from_eth();

