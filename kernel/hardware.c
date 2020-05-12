#include "hardware.h"

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
