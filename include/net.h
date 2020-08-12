#pragma once

// TODO: acquire mac addr in the "proper" way
// initialize_e1000 should set it and write it to a file 
// or global process variable or something. 
// multiple levels of the stack need it.
// #define MAC_ADDR 0x82 // from debugging output of qemu.


// is this right? or is it the other way around?
extern uint8_t MAC_ADDR_ARR[6]; 

// little-endian
#define MAC_ADDR_LO 0x12005452
#define MAC_ADDR_HI 0x00005634

uint16_t htons(uint16_t i);
uint32_t hton(uint32_t i);
uint16_t ntohs(uint16_t i);
uint32_t ntoh(uint32_t i);


typedef struct {
    uint32_t ip_addr;
    uint32_t router_ip;
    uint32_t subnet_mask;
    uint32_t ip_lease_time;
    uint32_t dhcp_server;
    uint32_t dns_servers[256];
} ip_config_t;

extern ip_config_t ip_config;

void print_ip_config();

void dhcp_bootstrap_ip();


