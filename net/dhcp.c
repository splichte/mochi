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
    // what's the standard way of doing this?
} dhcp_msg;

#define DHCP_SRC_PORT   68
#define DHCP_DST_PORT   67

// this is just "DHCP discover"
void test_transmit() {
    //===================== DHCP PACKET
    //
    // https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol#Operation


    // zero out eth frame
    // for DHCP discover
    uint8_t dhcp_options[] = {
        0x63, 0x82, 0x53, 0x63,
        0x35, 0x01, 0x01,
        0x32, 0x04, 0xc0, 0xa8, 0x01, 0x64,
        0x37, 0x04, 0x01, 0x03, 0x0f, 0x06,
        0xff };

    uint16_t dhcp_pkt_len = sizeof(dhcp_msg) + sizeof(dhcp_options);
    uint8_t dhcp_pkt[dhcp_pkt_len];
    dhcp_msg dm = { 0 };
    dm.hlen = 0x06;
    dm.htype = 0x01;
    dm.op = 0x01;
    dm.xid = hton(0x3903f326);
    dm.chaddr[15] = MAC_ADDR; // which byte do I assign to? lol

    memmove(dhcp_pkt, &dm, sizeof(dhcp_msg));
    memmove(dhcp_pkt + sizeof(dhcp_msg), dhcp_options, sizeof(dhcp_options));

    send_udp(dhcp_pkt, dhcp_pkt_len, DHCP_SRC_PORT, DHCP_DST_PORT);
}

