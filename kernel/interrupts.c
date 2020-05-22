/* Important resources:
 * [1] - IBM PS/2 Hardware Interface Technical Reference (May 1988)
 * [2] - Intel 80386 Programmer's Manual (1986)
 */

/* TODO: separate the keyboard and interrupt logic */

#include "../drivers/screen.h"
#include "hardware.h"
#include "interrupts.h"
#include <stdint.h>
#include <stdbool.h>

// definitions for PIC controllers. see [1]
#define PIC1A   0x20
#define PIC1B   (PIC1A + 1)

#define PIC2A   0xA0
#define PIC2B   (PIC2A + 1)

#define PIC_EOI 0x20


// from GCC docs:
// https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html
struct interrupt_frame {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};

// note: exceptions push different things on the stack than do interrupts.
// these handlers are used for un-assigned interrupts, so we can have 
// a generic handler that fires and logs what int # came.
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


/* standard ASCII symbols */
#define BACKSPACE 8
#define TAB 9
#define LINE_FEED 10
#define SHIFT_OUT 14
#define SHIFT_IN 15
#define ESC 27 

// ones I defined
// TODO: make an enum
#define CAPS_LOCK 256
#define CTRL 259
#define ALT 260
#define MISSING 0


/* I figured these out by using print_byte on the incoming keys for the 
 * keyboard. */
/* Most of the keys we need (the "down" presses) start at the beginning of the 
 * scancode range. */
/* I use extended number range to represent things like 
 * caps lock, etc. */
uint16_t scancodes[255] = {
    MISSING, CAPS_LOCK, '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '-', '=', BACKSPACE, TAB, 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', LINE_FEED, CTRL,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 
    '`', SHIFT_IN, MISSING, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', 
    '.', '/', SHIFT_IN, MISSING, ALT, ' ', ESC,
};

// we need to call this somewhere. 
void finish_scancode_init() {
    scancodes[170] = SHIFT_OUT;
    scancodes[182] = SHIFT_OUT;
}


/* start network stuff. from ToaruOS.
 *
 * I don't know what any of this means.
 */

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

struct rx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} __attribute__((packed)); 

struct tx_desc {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} __attribute__((packed));

// I renamed these from ToaruOS just because...I 
// can read them easier?
#define E1000_CTRL_REGISTER             0x0000
#define E1000_STATUS_REGISTER           0x0008
#define E1000_EEPROM_REGISTER           0x0014
#define E1000_CTRL_EXT_REGISTER         0x0018

#define E1000_RCTRL_REGISTER            0x0100
#define E1000_RX_DESC_LOW_REGISTER      0x2800
#define E1000_RX_DESC_HIGH_REGISTER     0x2804
#define E1000_RX_DESC_LEN_REGISTER      0x2808
#define E1000_RX_DESC_HEAD_REGISTER     0x2810
#define E1000_RX_DESC_TAIL_REGISTER     0x2818

#define E1000_TCTRL_REGISTER            0x0400
#define E1000_TX_DESC_LOW_REGISTER      0x3800
#define E1000_TX_DESC_HIGH_REGISTER     0x3804
#define E1000_TX_DESC_LEN_REGISTER      0x3810
#define E1000_TX_DESC_TAIL_REGISTER     0x3818

#define E1000_RX_ADDR_REGISTER          0x5400

/* Straight up copied from ToaruOS. */
#define RCTL_EN                         (1 << 1)    /* Receiver Enable */
#define RCTL_SBP                        (1 << 2)    /* Store Bad Packets */
#define RCTL_UPE                        (1 << 3)    /* Unicast Promiscuous Enabled */
#define RCTL_MPE                        (1 << 4)    /* Multicast Promiscuous Enabled */
#define RCTL_LPE                        (1 << 5)    /* Long Packet Reception Enable */
#define RCTL_LBM_NONE                   (0 << 6)    /* No Loopback */
#define RCTL_LBM_PHY                    (3 << 6)    /* PHY or external SerDesc loopback */
#define RCTL_RDMTS_HALF                 (0 << 8)    /* Free Buffer Threshold is 1/2 of RDLEN */
#define RCTL_RDMTS_QUARTER              (1 << 8)    /* Free Buffer Threshold is 1/4 of RDLEN */
#define RCTL_RDMTS_EIGHTH               (2 << 8)    /* Free Buffer Threshold is 1/8 of RDLEN */
#define RCTL_MO_36                      (0 << 12)   /* Multicast Offset - bits 47:36 */
#define RCTL_MO_35                      (1 << 12)   /* Multicast Offset - bits 46:35 */
#define RCTL_MO_34                      (2 << 12)   /* Multicast Offset - bits 45:34 */
#define RCTL_MO_32                      (3 << 12)   /* Multicast Offset - bits 43:32 */
#define RCTL_BAM                        (1 << 15)   /* Broadcast Accept Mode */
#define RCTL_VFE                        (1 << 18)   /* VLAN Filter Enable */
#define RCTL_CFIEN                      (1 << 19)   /* Canonical Form Indicator Enable */
#define RCTL_CFI                        (1 << 20)   /* Canonical Form Indicator Bit Value */
#define RCTL_DPF                        (1 << 22)   /* Discard Pause Frames */
#define RCTL_PMCF                       (1 << 23)   /* Pass MAC Control Frames */
#define RCTL_SECRC                      (1 << 26)   /* Strip Ethernet CRC */

#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

#define TCTL_EN                         (1 << 1)    /* Transmit Enable */
#define TCTL_PSP                        (1 << 3)    /* Pad Short Packets */
#define TCTL_CT_SHIFT                   4           /* Collision Threshold */
#define TCTL_COLD_SHIFT                 12          /* Collision Distance */
#define TCTL_SWXOFF                     (1 << 22)   /* Software XOFF Transmission */
#define TCTL_RTLC                       (1 << 24)   /* Re-transmit on Late Collision */

#define CMD_EOP                         (1 << 0)    /* End of Packet */
#define CMD_IFCS                        (1 << 1)    /* Insert FCS */
#define CMD_IC                          (1 << 2)    /* Insert Checksum */
#define CMD_RS                          (1 << 3)    /* Report Status */
#define CMD_RPS                         (1 << 4)    /* Report Packet Sent */
#define CMD_VLE                         (1 << 6)    /* VLAN Packet Enable */
#define CMD_IDE                         (1 << 7)    /* Interrupt Delay Enable */


#define PCI_ADDR_PORT           0xcf8
#define PCI_VALUE_PORT          0xcfc

// offsets into the PCI header structure. 
// reminder that the PCI header is little-endian. 
#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02

#define PCI_SUBCLASS            0x0a
#define PCI_CLASS_CODE          0x0b

#define PCI_INTERRUPT_LINE      0x3c


typedef struct {
    uint16_t bus;
    uint8_t device;
    uint8_t function;
} pci_bdf;

// do these addresses just exist in memory, or do I have to 
// explicitly call port byte out/etc?

// address of the header for this BDF addr in pci configuration space
//
// 0xfc because bits 1-0 must be zero. (see https://wiki.osdev.org/PCI#PCI_Device_Structure)
//
uint32_t pci_get_addr(pci_bdf bdf, uint8_t offset) {
    return 0x80000000 | (bdf.bus << 16) | (bdf.device << 11) | (bdf.function << 8) | (offset & 0xfc);
}

uint16_t pci_get_vendor_id(pci_bdf bdf) {
    uint32_t addr = pci_get_addr(bdf, 0x00);
    port_dword_out(PCI_ADDR_PORT, addr);

    uint32_t word = port_dword_in(PCI_VALUE_PORT);

    return word & 0xffff;
//    return ((word & 0xff) << 8) | ((word >> 8) & 0xff);
}

uint8_t pci_get_class_code(pci_bdf bdf) {
    uint32_t addr = pci_get_addr(bdf, 0x08);
    port_dword_out(PCI_ADDR_PORT, addr);

    uint32_t word = port_dword_in(PCI_VALUE_PORT);

    return (word >> 24) & 0xff;
}

uint8_t pci_get_subclass(pci_bdf bdf) {
    uint32_t addr = pci_get_addr(bdf, 0x08);
    port_dword_out(PCI_ADDR_PORT, addr);

    uint32_t word = port_dword_in(PCI_VALUE_PORT);

    return (word >> 16) & 0xff;
}

uint8_t pci_get_interrupt_line(pci_bdf bdf) {
    uint32_t addr = pci_get_addr(bdf, 0x3c);
    port_dword_out(PCI_ADDR_PORT, addr);

    uint32_t word = port_dword_in(PCI_VALUE_PORT);

    return ((word & 0xff) + 0x20); // 0x20 is the pic offset.
}


// stdbool?
bool is_ethernet(pci_bdf target) {
    uint16_t vendor_id = pci_get_vendor_id(target);

    if (vendor_id == 0xffff) { // device doesn't exist
        return false;
    }

    if (vendor_id != 0x8086) { // intel
        return false;
    }

    uint8_t class_code = pci_get_class_code(target);
    uint8_t subclass = pci_get_subclass(target);

    // 0x02 = network; 0x00 = ethernet
    if (class_code != 0x02 | subclass != 0x00) {
        return false;
    }

    return true;
}

pci_bdf get_ethernet_bdf() {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_bdf target = { bus, device, 0 };
            if (is_ethernet(target)) {
                return target;
            }
        }
    }

    pci_bdf bad = { -1, -1, -1 };
    return bad;
}

int get_e1000_interrupt() {
    pci_bdf ethernet = get_ethernet_bdf();
    if (ethernet.bus == -1) {
        return -1;
    }

    return pci_get_interrupt_line(ethernet);
}


 /* end network stuff
 */


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
void kbd_handler(struct interrupt_frame *frame) {
    asm volatile ("cli");

    uint8_t status = port_byte_in(0x64);

    // output buffer full? 
    if (status & 0x01) {
        uint8_t key = port_byte_in(0x60);
        uint8_t buf[2];
        buf[0] = scancodes[key];
        buf[1] = '\0';
        if (buf[0] != MISSING) {
            print(buf);
        }
    }

    port_byte_out(PIC1A, PIC_EOI);

    asm volatile ("sti");
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

// see [2], Ch. 9 - "Exceptions and Interrupts"
// the IDT has 256 total entries. Our PIC-defined 
// entries start at entry 32 (0-indexed)
void setup_interrupt_descriptor_table() {
    // Operation Command Byte 1 [2]
    // 0xfd == 1111 1101 == disable all but keyboard (IRQ1).
    port_byte_out(PIC1B, 0xfd);
    port_byte_out(PIC2B, 0xff);

    finish_scancode_init();

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

