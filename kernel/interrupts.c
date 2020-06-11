#pragma once

#include "devices.h"
#include <stdint.h>
#include <stdbool.h>
#include "../drivers/screen.h"
#include "hardware.h"

// NOTE: exceptions push different things on the stack than do interrupts.
// these handlers are used for un-assigned interrupts, so we can have 
// a generic handler that fires and logs what int # came.
#define INT(i) \
     __attribute__((interrupt)) \
    void interrupt_##i(struct interrupt_frame *frame) { \
        asm volatile ("cli"); \
        print_byte(i);\
        if (i >= 32 && i < 48) { \
            if (i > 39 && i < 48) { \
                port_byte_out(PIC2A, PIC_EOI); \
            } \
            port_byte_out(PIC1A, PIC_EOI); \
        } \
        asm volatile ("sti"); \
    } \

#define EXCEPTION(i) \
     __attribute__((interrupt)) \
    void interrupt_##i(struct interrupt_frame *frame, uint32_t error_code) { \
        asm volatile ("cli"); \
        print_byte(i); \
        asm volatile ("sti"); \
    } \

EXCEPTION(0);
EXCEPTION(1);
EXCEPTION(2);
EXCEPTION(3);
EXCEPTION(4);
EXCEPTION(5);
EXCEPTION(6);
EXCEPTION(7);
EXCEPTION(8);
EXCEPTION(9);
EXCEPTION(10);
EXCEPTION(11);
EXCEPTION(12);
EXCEPTION(13);
INT(14);
INT(15);
INT(16);
INT(17);
INT(18);
INT(19);
INT(20);
INT(21);
INT(22);
INT(23);
INT(24);
INT(25);
INT(26);
INT(27);
INT(28);
INT(29);
INT(30);
INT(31);

/* NOTE:
 *  Operation Command Byte 1 (in setup_descriptor_table) 
 *  prevents PIC IRQ lines other than 0 (timer) and 1 (keyboard)
 *  from firing.
 */

// INT(32); - timer -- don't need.
// INT(33); - keyboard -- don't need.
INT(34);
INT(35);
INT(36);
INT(37);
INT(38);
INT(39);
INT(40);
INT(41);
INT(42);
INT(43);
INT(44);
INT(45);
INT(46);
INT(47);

// definitions for the Interrupt Descriptor Table (IDT)
// see [2]. the table is composed of "gates" 
// "selector" refers to a "TSS descriptor", which 
// is an entry in the Global Descriptor Table (GDT).
//
// 8-byte descriptor
typedef struct __attribute__((packed)) {
    uint16_t offset_15_0;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t offset_31_16;
} idt_gate;
// offset to a location in the GDT

// keyboard is at 33 (32 + 1)
#define N_IDT_ENTRIES 256
static idt_gate idt[N_IDT_ENTRIES];

typedef struct __attribute__((packed)) {
    uint16_t idt_limit;
    uint32_t idt_base;
} idt_register;

idt_register idtr;

// PIC1 and PIC2. See [1]
void setup_interrupt_controller() {
    // interrupts must be already disabled from 
    // when we switched to protected mode. 

    // Initialization Command Byte 1 (init)
    // set required bits 0, 3, 4 == 0001 1001 == 0x19
    // ToaruOS does 0x10 | 0x01. i.e. uses edge-triggered, 
    // not level-triggered. PS/2 manual says to use 
    // level-triggered?
    port_byte_out(PIC1A, 0x10 | 0x9);
    port_byte_out(PIC2A, 0x10 | 0x9);

    // Initialization Command Byte 2
    // offset by 32 to the start of Intel's "ok" range
    // for interrupts. 
    // 32 == 0010 0000 = 0x20
    port_byte_out(PIC1B, 0x20);
    port_byte_out(PIC2B, 0x28);

    // Initialization Command Byte 3
    // 0x04 to PIC1 and 0x02 to PIC2 to set their 
    // communication with each other (see PS/2 reference)
    port_byte_out(PIC1B, 0x04);
    port_byte_out(PIC2B, 0x02);

    // Initialization Command Byte 4 (final)
    // set 80286/80386 mode (0x00). 
    // ToaruOS does 0x01, though? 
    // I'm reading the 8259A manual and not sure 
    // what to set, or how it matters.
    port_byte_out(PIC1B, 0x01);
    port_byte_out(PIC2B, 0x01); 
}

__attribute__ ((interrupt))
void null_interrupt(struct interrupt_frame *frame) {
    asm volatile ("cli");
    print("Unhandled\n");
    asm volatile ("sti");
}


idt_gate set_gate(uint32_t handler_ptr) {
    idt_gate kbdg;
    uint32_t base = handler_ptr;
    kbdg.offset_15_0 = base & 0xFFFF;
    kbdg.offset_31_16 = (base >> 16) & 0xFFFF;
    // flags is: 1000 1110
    // the low part sets it as an interrupt gate.
    // the high part is the "present" bit, which must be 1 
    // for all valid selectors. 
    //
    kbdg.flags = 0x8e; 

    kbdg.selector = 0x08; // see section 5.1.3 of [2]

    return kbdg;
}

extern void timer_handler(struct interrupt_frame *frame);

// see [2], Ch. 9 - "Exceptions and Interrupts"
// the IDT has 256 total entries. Our PIC-defined 
// entries start at entry 32 (0-indexed)
void setup_interrupt_descriptor_table() {
    // Operation Command Byte 1 [2]
    // 0xfc == 1111 1100 == disable all but keyboard (IRQ1)
    // and timer (IRQ0)
    port_byte_out(PIC1B, 0xfc);
    port_byte_out(PIC2B, 0xff);

    timer_setup();

    // We set gates like this, because we can't easily find functions by name
    // without a language that supports reflection!
    //
    // TODO: have these generated as part of build process.
    idt[1] = set_gate((uint32_t) &interrupt_1);
    idt[2] = set_gate((uint32_t) &interrupt_2);
    idt[3] = set_gate((uint32_t) &interrupt_3);
    idt[4] = set_gate((uint32_t) &interrupt_4);
    idt[5] = set_gate((uint32_t) &interrupt_5);
    idt[6] = set_gate((uint32_t) &interrupt_6);
    idt[7] = set_gate((uint32_t) &interrupt_7);
    idt[8] = set_gate((uint32_t) &interrupt_8);
    idt[9] = set_gate((uint32_t) &interrupt_9);
    idt[10] = set_gate((uint32_t) &interrupt_10);
    idt[11] = set_gate((uint32_t) &interrupt_11);
    idt[12] = set_gate((uint32_t) &interrupt_12);
    idt[13] = set_gate((uint32_t) &interrupt_13);
    idt[14] = set_gate((uint32_t) &interrupt_14);
    idt[15] = set_gate((uint32_t) &interrupt_15);
    idt[16] = set_gate((uint32_t) &interrupt_16);
    idt[17] = set_gate((uint32_t) &interrupt_17);
    idt[18] = set_gate((uint32_t) &interrupt_18);
    idt[19] = set_gate((uint32_t) &interrupt_19);
    idt[20] = set_gate((uint32_t) &interrupt_20);
    idt[21] = set_gate((uint32_t) &interrupt_21);
    idt[22] = set_gate((uint32_t) &interrupt_22);
    idt[23] = set_gate((uint32_t) &interrupt_23);
    idt[24] = set_gate((uint32_t) &interrupt_24);
    idt[25] = set_gate((uint32_t) &interrupt_25);
    idt[26] = set_gate((uint32_t) &interrupt_26);
    idt[27] = set_gate((uint32_t) &interrupt_27);
    idt[28] = set_gate((uint32_t) &interrupt_28);
    idt[29] = set_gate((uint32_t) &interrupt_29);
    idt[30] = set_gate((uint32_t) &interrupt_30);
    idt[31] = set_gate((uint32_t) &interrupt_31);
    idt[32] = set_gate((uint32_t) &timer_handler); // timer!!
    idt[33] = set_gate((uint32_t) &kbd_handler); // keyboard!!
    idt[34] = set_gate((uint32_t) &interrupt_34);
    idt[35] = set_gate((uint32_t) &interrupt_35);
    idt[36] = set_gate((uint32_t) &interrupt_36);
    idt[37] = set_gate((uint32_t) &interrupt_37);
    idt[38] = set_gate((uint32_t) &interrupt_38);
    idt[39] = set_gate((uint32_t) &interrupt_39);
    idt[40] = set_gate((uint32_t) &interrupt_40);
    idt[41] = set_gate((uint32_t) &interrupt_41);
    idt[42] = set_gate((uint32_t) &interrupt_42);
    idt[43] = set_gate((uint32_t) &interrupt_43); // internet, supposedly
    idt[44] = set_gate((uint32_t) &interrupt_44);
    idt[45] = set_gate((uint32_t) &interrupt_45);
    idt[46] = set_gate((uint32_t) &interrupt_46);
    idt[47] = set_gate((uint32_t) &interrupt_47);

    for (int i = 48; i < N_IDT_ENTRIES; i++) {
        idt[i] = set_gate((uint32_t) &null_interrupt);
    }
    idt_gate zero = { 0, 0, 0, 0 };
    idt[0] = zero;

    // load IDT register
    idtr.idt_base = (uint32_t) idt;
    idtr.idt_limit = sizeof (idt) - 1;

    asm volatile ("lidt (%0)" : : "r" (&idtr));

    asm volatile ("sti");
}

