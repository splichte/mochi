#include <stdint.h>
#include "string.h"
#include "net.h"
#include "eth.h"
#include "e1000.h"

// TODO: reduce the amount of copies that are performed
int send_eth(uint8_t *ip_pkt, uint32_t len) {
    eth_pkt ep = { 0 };

    // FIXME: these are only for DHCP.
    memset(ep.mac_dest, 0xff, 6);
    ep.mac_src[0] = MAC_ADDR; // 6-byte quantity...weird

    memmove(ep.buf, ip_pkt, len);

    // how big do we want to send? 
    // should be size of IP/UDP message. but this works.
    ep.ethertype = htons(ETHERTYPE_IPV4); 

    return send_e1000(&ep);
}

