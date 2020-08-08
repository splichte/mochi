#include <stdint.h>
#include "string.h"
#include "net.h"
#include "ip.h"

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} udp_hdr;


int send_udp(uint8_t *pkt, uint16_t len, uint16_t src_port, uint16_t dst_port) {
    //====================== UDP PACKET
    //
    uint16_t udp_pkt_len = sizeof(udp_hdr) + len;
    uint8_t udp_pkt[udp_pkt_len];

    udp_hdr udp = { 0 };
    udp.src_port = htons(src_port);
    udp.dst_port = htons(dst_port);
    udp.len = htons(udp_pkt_len);

    memmove(udp_pkt, &udp, sizeof(udp_hdr));
    memmove(udp_pkt + sizeof(udp_hdr), pkt, len);

    // send
    return send_ip(udp_pkt, udp_pkt_len, IP_PROTOCOL_UDP);
}

