/* Important resources:
 * [1] - IBM PS/2 Hardware Interface Technical Reference (May 1988)
 * [2] - Intel 80386 Programmer's Manual (1986)
 */

#pragma once

#include <stdint.h>

// definitions for PIC controllers. see [1]
#define PIC1A   0x20
#define PIC1B   (PIC1A + 1)

#define PIC2A   0xA0
#define PIC2B   (PIC2A + 1)

#define PIC_EOI 0x20

// trying this stuff out.
#define pause_interrupts()  asm volatile ("cli")
#define resume_interrupts()  asm volatile ("sti")
#define ack_interrupt_pic1()   port_byte_out(PIC1A, PIC_EOI)
#define ack_interrupt_pic2()   port_byte_out(PIC2A, PIC_EOI)


// from GCC docs:
// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html
struct interrupt_frame {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};


void setup_interrupt_controller();
void setup_interrupt_descriptor_table();


