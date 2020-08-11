#include <stdint.h>
#include "string.h"
#include "net.h"
#include "eth.h"
#include "e1000.h"

#include "kalloc.h"

// TODO: reduce the amount of copies that are performed
int send_ip_pkt_to_eth(uint8_t *ip_pkt, uint32_t len) {
    eth_pkt ep = { 0 };

    // FIXME: these are only for DHCP.
    memset(ep.mac_dest, 0xff, 6);
    memmove(ep.mac_src, MAC_ADDR_ARR, 6);

    memmove(ep.buf, ip_pkt, len);

    // how big do we want to send? 
    // should be size of IP/UDP message. but this works.
    ep.ethertype = htons(ETHERTYPE_IPV4); 

    return send_eth_to_e1000(&ep);
}

uint8_t *recv_ip_pkt_from_eth() {
    eth_pkt ep = recv_eth_from_e1000();

    uint16_t payload_len = ETH_MAX_PKT_LEN - 4;
    uint8_t *pkt = kmalloc(payload_len);

    // TODO: don't have every packet be max length, lol.
    // also deal with fragmentation.
    memmove(pkt, ep.buf, payload_len);

    return pkt;
}


