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


// taken from ToaruOS. 
// I think this stops the CPU from out-of-order pipelining?
#define PIC_WAIT() \
	do { \
		/* May be fragile */ \
		asm volatile("jmp 1f\n\t" \
		             "1:\n\t" \
		             "    jmp 2f\n\t" \
		             "2:"); \
	} while (0)


uint8_t scancodes[255] = {
    0, '`', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '-', '=', 0, 0, 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\', 
};

// the format of the Task State Segment (TSS)'s "dynamic set"
// see [2] p.130
// see Figure 7-1, "80386 32-Bit Task State Segment"
typedef struct __attribute__((packed)) {
    // general registers
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;

    // segment registers
    uint16_t es, cs, ss, ds, fs, gs;

    // flags register
    uint32_t eflags;

    // instruction pointer
    uint32_t eip;

    // selector of the TSS of the 
    // previously executing task
    uint32_t sel;
} tss_dynamic_set;

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
    PIC_WAIT();

    port_byte_out(PIC2A, 0x10 | 0x01);
    PIC_WAIT();

    // Initialization Command Byte 2
    // offset by 32 to the start of Intel's "ok" range
    // for interrupts. 
    // 32 == 0010 0000 = 0x20
    port_byte_out(PIC1B, 0x20);
    PIC_WAIT();

    port_byte_out(PIC2B, 0x28);
    PIC_WAIT();

    // Initialization Command Byte 3
    // 0x04 to PIC1 and 0x02 to PIC2 to set their 
    // communication with each other (see PS/2 reference)
    port_byte_out(PIC1B, 0x04);
    PIC_WAIT();

    port_byte_out(PIC2B, 0x02);
    PIC_WAIT();

    // Initialization Command Byte 4 (final)
    // set 80286/80386 mode (0x00). 
    // ToaruOS does 0x01, though? 
    // I'm reading the 8259A manual and not sure 
    // what to set, or how it matters.
    port_byte_out(PIC1B, 0x01);
    PIC_WAIT();

    port_byte_out(PIC2B, 0x01); 
    PIC_WAIT();

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

__attribute__ ((interrupt))
void interrupt_function(struct interrupt_frame *frame, uint32_t error_code) {
    asm volatile ("cli");

    // do something! like print to the screen!
    print("Hello from interrupt handler!");

    while (1) {
        // halt, so we can see the print!
    }

    asm volatile ("sti");
}

// see [2], Ch. 9 - "Exceptions and Interrupts"
// the IDT has 256 total entries. Our PIC-defined 
// entries start at entry 32 (0-indexed)
void setup_interrupt_descriptor_table() {
    // right now, we want to ignore every interrupt
    // except the keyboard. 
    // apparently this disables some handlers? from osdev.
    port_byte_out(PIC1B, 0xfd);
    port_byte_out(PIC2B, 0xff);

    idt_gate kbdg;
    uint32_t base = (uint32_t) &interrupt_function;
    kbdg.offset_15_0 = base & 0xFFFF;
    kbdg.offset_31_16 = (base >> 16) & 0xFFFF;
    // flags is: 1000 1110
    // the low part sets it as an interrupt gate.
    // the high part is the "present" bit, which must be 1 
    // for all valid selectors. 
    //
    kbdg.flags = 0x8e; 

    kbdg.selector = 0x08; // see section 5.1.3 of [2]

    // setup the IDT
    for (int i = 1; i < N_IDT_ENTRIES; i++) {
        idt[i] = kbdg;
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

    // TODO: disable the mouse. 
    // TODO: use interrupt-driven, not polling! 
    while (1) {
        // controller status register
//        unsigned uint8_t status = port_byte_in(0x64);
//
//        // output buffer full? 
//        if (status & 0x01) {
//            unsigned uint8_t key = port_byte_in(0x60);
//            uint8_t buf[2];
//            buf[0] = scancodes[key];
//            buf[1] = '\0';
//            print(buf);
//        }
    }
}
