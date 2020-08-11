#include "screen.h"
#include "hardware.h"
#include "interrupts.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "devices.h"
#include "string.h"
#include "net.h"
#include "udp.h"


#define DHCP_OPT_MSG_TYPE       53
#define DHCP_OPT_SUBNET_MASK    1
#define DHCP_OPT_ROUTER         3
#define DHCP_OPT_IP_LEASE_TIME  51
#define DHCP_OPT_SERVER         54
#define DHCP_OPT_DNS_SERVERS    6

#define DHCP_MSG_TYPE_DISCOVER  1
#define DHCP_OPT_PARAM_REQ_LIST 55

#define DHCP_OPT_END            255

// unset, at the start. 
ip_config_t ip_config = { 0 };

/* https://tools.ietf.org/html/rfc2131#section-2 */
// https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol#Discovery
typedef struct __attribute__((packed)) {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;

    uint32_t xid;

    uint16_t secs;
    uint16_t flags;

    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;

    uint8_t chaddr[16];

    char sname[64];

    char file[128];

    /* options go here */
} dhcp_hdr;

#define DHCP_SRC_PORT   68
#define DHCP_DST_PORT   67

void send_dhcp_discover() {
    //===================== DHCP PACKET
    //
    // https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol#Operation

    uint8_t dhcp_options[] = {
        0x63, 0x82, 0x53, 0x63,
        0x35, 0x01, 0x01,
        0x32, 0x04, 0xc0, 0xa8, 0x01, 0x64,
        0x37, 0x04, 0x01, 0x03, 0x0f, 0x06,
        0xff };

    uint16_t dhcp_pkt_len = sizeof(dhcp_hdr) + sizeof(dhcp_options);
    uint8_t dhcp_pkt[dhcp_pkt_len];
    dhcp_hdr dm = { 0 };
    dm.hlen = 0x06;
    dm.htype = 0x01;
    dm.op = 0x01;
    dm.xid = hton(0x3903f326);
    // I'm sending "no broadcast". it's not receiving?

    memmove(dm.chaddr, MAC_ADDR_ARR, 6); 

    memmove(dhcp_pkt, &dm, sizeof(dhcp_hdr));
    memmove(dhcp_pkt + sizeof(dhcp_hdr), dhcp_options, sizeof(dhcp_options));

    send_pkt_to_udp(dhcp_pkt, dhcp_pkt_len, DHCP_SRC_PORT, DHCP_DST_PORT);
}

void read_dhcp_offer() {
    uint16_t src_port, dst_port;
    uint8_t *pkt = recv_pkt_from_udp(&src_port, &dst_port);


    // pkt should be a DHCP offer. 
    dhcp_hdr *hdr = (dhcp_hdr *) pkt;

    ip_config.ip_addr = ntoh(hdr->yiaddr);

    // parse options
    uint8_t *p = ((uint8_t *) hdr) + sizeof(dhcp_hdr);
    p += 4; // skip the magic DHCP cookie.

    while (*p != DHCP_OPT_END) {
        switch (*p) {
            case DHCP_OPT_MSG_TYPE: {
                p += 3;
                break;
            }
            case DHCP_OPT_SUBNET_MASK: {
                p += 2;
                ip_config.subnet_mask = ntoh(*((uint32_t *) p));
                p += 4;
                break;
            }
            case DHCP_OPT_ROUTER: {
                p += 2;
                ip_config.router_ip = ntoh(*((uint32_t *) p));
                p += 4;
                break;
            }
            case DHCP_OPT_IP_LEASE_TIME: {
                p += 2;
                ip_config.ip_lease_time = ntoh(*((uint32_t *) p));
                p += 4;
                break;
            }
            case DHCP_OPT_SERVER: {
                p += 2;
                ip_config.dhcp_server = ntoh(*((uint32_t *) p));
                p += 4;
                break;
            }
            case DHCP_OPT_DNS_SERVERS: {
                p++;
                uint8_t len = *p++;
                for (int i = 0; i < len / 4; i++, p += 4) {
                    ip_config.dns_servers[i] = ntoh(*((uint32_t *) p));
                }
                break;
            }
            default:  // FIXME. need to read the option type to know 
                      // how much to advance p
                p++;
                break;
        }
    }
}

void send_dhcp_request() {

}

void read_dhcp_ack() {

}

void test_transmit() {
    send_dhcp_discover();

    read_dhcp_offer();

    // debugging
    print_ip_config();

    send_dhcp_request();

    read_dhcp_ack();
}


