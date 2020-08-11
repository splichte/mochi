#include <stdint.h>
#include "net.h"
#include "screen.h"

uint16_t htons(uint16_t i) {
    uint16_t o = 0;
    o |= (i >> 8) & 0xff;
    o |= (i << 8) & 0xff00;
    return o;
}

uint32_t hton(uint32_t i) {
    uint32_t o = 0;
    o |= (i >> 24) & 0xff;
    o |= (i >> 8) & 0xff00;
    o |= (i << 8) & 0xff0000;
    o |= (i << 24) & 0xff000000;
    return o;
}

uint16_t ntohs(uint16_t i) {
    return htons(i);
}

uint32_t ntoh(uint32_t i) {
    return hton(i);
}

uint8_t MAC_ADDR_ARR[6] = {
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56
};

void print_ip_addr(uint32_t addr) {
    print_byte(addr >> 24);
    print(".");
    print_byte((addr >> 16) & 0xff);
    print(".");
    print_byte((addr >> 8) & 0xff);
    print(".");
    print_byte((addr) & 0xff);
}

void print_ip_config() {
    if (ip_config.ip_addr == 0) {
        print("IP not configured.\n");
        return;
    }
    print("----- IP Configuration -----\n\n");
    print("Mochi IP address: "); print_ip_addr(ip_config.ip_addr); print("\n");
    print("Router IP address: "); print_ip_addr(ip_config.router_ip); print("\n");
    print("Subnet mask: "); print_ip_addr(ip_config.subnet_mask); print("\n");
    print("IP lease time (sec): "); print_word(ip_config.ip_lease_time);
    print("DHCP server: ");print_ip_addr(ip_config.dhcp_server); print("\n");

    print("DNS servers: \n");
    for (int i = 0; i < 255; i++) {
        if (ip_config.dns_servers[i] == 0) continue;
        print("  "); print_ip_addr(ip_config.dns_servers[i]); print("\n");
    }
    print("----------------------------\n");

}


