/* Important resources:
 * [1] - IBM PS/2 Hardware Interface Technical Reference (May 1988)
 * [2] - Intel 80386 Programmer's Manual (1986)
 */
#include "../drivers/screen.h"
#include "hardware.h"
#include <stdint.h>

// definitions for PIC controllers. see [1]
#define PIC1A   0x20
#define PIC1B   (PIC1A + 1)

#define PIC2A   0xA0
#define PIC2B   (PIC2A + 1)

#define PIC_EOI 0x20

// print 0 to 256, in decimal
// TODO: I'm sure there's a better way to do this. 
void print_byte(uint8_t i) {
    // max value of 256
    char s[5];
    s[4] = '\0';
    s[3] = '\n';

    int hundreds_place = (i / 100);
    s[0] = hundreds_place + '0';

    int tens_place = (i - (hundreds_place * 100)) / 10;
    s[1] = tens_place + '0';

    int ones_place = i - (hundreds_place * 100) - (tens_place * 10);
    s[2] = ones_place + '0';

    print(s);
}

/* print word as hex */
void print_word(uint32_t word) {
    // 4 bytes = 8 hex digits + 0x
    char s[12];
    s[11] = '\0';
    s[10] = '\n';
    s[1] = 'x';
    s[0] = '0';

    for (int i = 0; i < 8; i++) {
        int ws = (word >> (i * 8)) & 0xF;
        if (ws >= 0 && ws < 10) {
            s[9 - i] = '0' + ws;
        } else {
            s[9 - i] = 'a' + (ws - 10);
        }
    }
    print(s);
}

// from GCC docs:
// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html
struct interrupt_frame {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};

// we need to print the instruction pointer, ip. 
// to figure out what tf is going on lol.  

// i think this interrupt function is causing a general protection fault. 
// so every time it triggers, another int13 is added 
//  
/* preprocessor stuff */  

// this could be returning badly? that seems possible. 
// we should print out the data in the interrupt frame, though. 
// which means we need to (1) finish the hex-writer function
// (2) see what the previous instruction was
// (3) if nothing useful comes up, try defining the interrupts in asm
//
// ripped out as test

// how on earth do I find what instruction is at an address? 
// maybe use the kernel offset + os image? 
// alternatively just call assembly and see what's there. 

// we shouldn't use an error code. ah. I messed up...should have read better...
//
// exceptions push different things on the stack than do interrupts.
#define INT(i) \
     __attribute__((interrupt)) \
    void interrupt_##i(struct interrupt_frame *frame) { \
        asm volatile ("cli"); \
        print_byte(i);\
        if (i > 32 && i < 48) { \
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
INT(32);
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


uint8_t scancodes[255] = {
    0, '`', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '-', '=', 0, 0, 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\', 
};

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
    port_byte_out(PIC1A, 0x10 | 0x01);
    port_byte_out(PIC2A, 0x10 | 0x01);

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
void kbd_handler(struct interrupt_frame *frame) {
    asm volatile ("cli");

    uint8_t status = port_byte_in(0x64);

    // output buffer full? 
    if (status & 0x01) {
        uint8_t key = port_byte_in(0x60);
        uint8_t buf[2];
        buf[0] = scancodes[key];
        buf[1] = '\0';
        print(buf);
    }

    port_byte_out(PIC1A, PIC_EOI);

    asm volatile ("sti");
}

__attribute__ ((interrupt))
void null_interrupt_hi(struct interrupt_frame *frame) {
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

// see [2], Ch. 9 - "Exceptions and Interrupts"
// the IDT has 256 total entries. Our PIC-defined 
// entries start at entry 32 (0-indexed)
void setup_interrupt_descriptor_table() {
    // Operation Command Byte 1 [2]
    // 0xfd == 1111 1101 == disable all but keyboard (IRQ1).
    port_byte_out(PIC1B, 0xfd);
    port_byte_out(PIC2B, 0xff);

    // keyboard interrupt triggers 2x. why? 
//    idt_gate kbdg = set_gate((uint32_t) &interrupt_function);
//    idt[33] = kbdg;

    // TODO: I think separate interrupts are triggering. 
    // is it the system timer? 
    //
    // set like this, because we can't easily find functions by name!
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
    idt[32] = set_gate((uint32_t) &interrupt_32);
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
    idt[43] = set_gate((uint32_t) &interrupt_43);
    idt[44] = set_gate((uint32_t) &interrupt_44);
    idt[45] = set_gate((uint32_t) &interrupt_45);
    idt[46] = set_gate((uint32_t) &interrupt_46);
    idt[47] = set_gate((uint32_t) &interrupt_47);

    for (int i = 48; i < N_IDT_ENTRIES; i++) {
        idt[i] = set_gate((uint32_t) &null_interrupt_hi);
    }
    idt_gate zero = { 0, 0, 0, 0 };
    idt[0] = zero;

    // load IDT register
    idtr.idt_base = (uint32_t) idt;
    idtr.idt_limit = sizeof (idt) - 1;

    asm volatile ("lidt (%0)" : : "r" (&idtr));

    asm volatile ("sti");
}

void main() {
    clear_screen(); 
    print("Hello, world!\n"); 

    setup_interrupt_controller();

    print("Set up interrupt controller.\n");

    setup_interrupt_descriptor_table();

    print("Set up descriptor table.\n");

    while (1) {
    }
}
