/* hardware.c is mostly from ToaruOS */
#include "hardware.h"

// TODO: use uint8_t, uint16_t
unsigned char port_byte_in(unsigned short port) {
    unsigned char result;
    asm volatile ("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void port_byte_out(unsigned short port, unsigned char data) {
    asm volatile ("out %%al, %%dx" : :"a" (data), "d" (port));
}

unsigned short port_word_in(unsigned short port) {
    unsigned short result;
    asm volatile ("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_word_out(unsigned short port, unsigned short data) {
    asm volatile ("out %%ax, %%dx" : :"a" (data), "d" (port));
}

/* TODO: why can't you just loop port_word_out? that doesn't work, but why? */
void port_multiword_out(unsigned short port, unsigned char *data, unsigned long size) {
    asm volatile ("rep outsw" : "+S" (data), "+c" (size) : "d" (port));
}

void port_multiword_in(unsigned short port, unsigned char *data, unsigned long size) {
    // TODO: what does "memory" do?
    asm volatile ("rep insw" : "+D" (data), "+c" (size) : "d" (port) : "memory");
}

void port_dword_out(unsigned short port, unsigned int data) {
    asm volatile ("outl %%eax, %%dx" : : "dN" (port), "a" (data));
}

unsigned int port_dword_in(unsigned short port) {
    unsigned int rv;
    asm volatile ("inl %%dx, %%eax" : "=a" (rv) : "dN" (port));
    return rv;
}
