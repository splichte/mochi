#pragma once

// TODO: acquire mac addr in the "proper" way
// initialize_e1000 should set it and write it to a file 
// or global process variable or something. 
// multiple levels of the stack need it.
#define MAC_ADDR 0x82; // from debugging output of qemu.

uint16_t htons(uint16_t i);

uint32_t hton(uint32_t i);

void test_transmit();
