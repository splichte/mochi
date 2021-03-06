#include <stdint.h>
#include "screen.h"
#include "string.h"
#include "net.h"
#include "ip.h"

// https://en.wikipedia.org/wiki/IPv4_header_checksum
void ipv4_cksum(ip_hdr *ih) {
    // at this point, the checksum field itself should be 0.
    uint32_t chksum = 0;
    uint8_t *ihp = (uint8_t *) ih;
    for (int i = 0; i < sizeof(ip_hdr); i += 2) {
        uint16_t hi = ((uint16_t) ihp[i]) << 8;
        uint16_t lo = ((uint16_t) ihp[i+1]);
        chksum += hi + lo;
    }

    uint8_t carry_count = (chksum / 0x10000);

    uint16_t sum16 = chksum & 0xffff;

    uint32_t ochk = sum16 + carry_count;
    carry_count = (ochk / 0x10000);
    ochk = ochk - 0x10000 + carry_count;

    uint16_t f16 = ochk & 0xffff;
    f16 = ~(f16 & f16); // invert all bits
    ih->checksum = htons(f16);
}

void ip_cksum_test() {
    // example from Wikipedia
    uint8_t arr[] = {
        0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 
        0x40, 0x11, 0x00, 0x00, 0xc0, 0xa8, 0x00, 0x01, 
        0xc0, 0xa8, 0x00, 0xc7,
    };

    print("ip cksum test: \n");
    ipv4_cksum((ip_hdr *) arr); // expected: 0xb861
}


// can be either TCP or UDP
int send_pkt_to_ip(uint8_t *tpkt, uint16_t tlen, uint8_t protocol) {
    uint32_t ip_pkt_len = tlen + sizeof(ip_hdr);

    ip_hdr ip = { 0 };
    ip.version = 4;
    ip.header_len = 5;

    ip.ttl = 0xff; // just needs to be nonzero.

    ip.total_len = htons(ip_pkt_len);
 
    ip.protocol = protocol;

    ip.src_addr = hton(0);
    ip.dst_addr = hton(0xffffffff); // 255.255.255.255

    // need to be VERY careful about endianness.
    ipv4_cksum(&ip);

    uint8_t ip_pkt[ip_pkt_len]; // make a buffer big enough for the ip_hdr and the msg

    memmove(ip_pkt, &ip, sizeof(ip_hdr));
    memmove(ip_pkt + sizeof(ip_hdr), tpkt, tlen);

    return send_ip_pkt_to_eth(ip_pkt, ip_pkt_len);
}

uint8_t *recv_pkt_from_ip(uint8_t *protocol) {
    uint8_t *buf = recv_ip_pkt_from_eth();

    ip_hdr *hdr = (ip_hdr *) buf;
    *protocol = hdr->protocol;

    uint32_t payload_len = hdr->total_len - sizeof(ip_hdr);

    uint8_t *pkt = kmalloc(payload_len);

    memmove(pkt, buf + sizeof(ip_hdr), payload_len);

    kfree(buf); // don't need buf anymore. 

    return pkt;
}

