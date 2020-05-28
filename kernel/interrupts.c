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
    
    [170] = SHIFT_OUT, [182] = SHIFT_OUT,
};


/* start network stuff
 */

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

#define ETHERNET_MAX_PACKET_SIZE    1518    // in bytes

/* packing these structs isn't necessary since they're multiples 
 * of 32 bits, but it lets you know that it's important that 
 * they are precise. */
struct __attribute__((packed)) {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} rx_desc; 

/* hw/net/e1000_regs.h */
typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} tx_desc;


// got this from wikipedia.
// NOTE: this is network byte order (big endian)
typedef struct __attribute__((packed)) {
    // network link protocol type (ethernet is 1)
    uint16_t htype;

    // protocol type (0x0800 for IP)
    uint16_t ptype;

    // length (in bytes) of hardware address. 6 for ethernet.
    uint8_t hlen;

    // length (in bytes) of internet address. 4 for IPv4.
    uint8_t plen;

    // operation: 1 for request, 2 for reply
    uint16_t op;

    // sender hardware address (SHA)
    uint16_t sha_1;
    uint16_t sha_2;
    uint16_t sha_3;

    // sender protocol address (SPA)
    uint32_t spa;

    // target hardware address (THA)
    // ignored in an ARP request.
    uint16_t tha_1;
    uint16_t tha_2;
    uint16_t tha_3;

    // target protocol address (TPA)
    uint32_t tpa;
} arp_packet;


#define E1000_CTRL_REGISTER             0x00000
#define E1000_STATUS_REGISTER           0x00008
#define E1000_EEPROM_REGISTER           0x00014
#define E1000_CTRL_EXT_REGISTER         0x00018

/* these are the names from the Intel docs. */
#define E1000_RCTL                      0x00100
#define E1000_RDBAL                     0x02800
#define E1000_RDLEN                     0x02808
#define E1000_RDH                       0x02810
#define E1000_RDT                       0x02818

#define E1000_TCTL                      0x00400
#define E1000_TIPG                      0x00410 /* e1000 in qEMU thinks this is unknown*/
#define E1000_TDBAL                     0x03800
#define E1000_TDLEN                     0x03808
#define E1000_TDH                       0x03810
#define E1000_TDT                       0x03818

#define E1000_RX_ADDR_REGISTER          0x05400

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

/* for programming the transmit initialization */
#define TCTL_EN                         (1 << 1)
#define TCTL_PSP                        (1 << 3)
#define TCTL_COLD                       0x40        // full duplex
#define TIPGT                           10          // according to Intel manual

#define CMD_EOP                         (1 << 0)    /* End of Packet */
#define CMD_IFCS                        (1 << 1)    /* Insert FCS */
#define CMD_IC                          (1 << 2)    /* Insert Checksum */
#define CMD_RS                          (1 << 3)    /* Report Status */
#define CMD_RPS                         (1 << 4)    /* Report Packet Sent */
#define CMD_VLE                         (1 << 6)    /* VLAN Packet Enable */
#define CMD_IDE                         (1 << 7)    /* Interrupt Delay Enable */

#define STATUS_DD                       (1 << 0)


#define PCI_ADDR_PORT           0xcf8
#define PCI_VALUE_PORT          0xcfc

// offsets into the PCI header structure. 
// reminder that the PCI header is little-endian. 
#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02

#define PCI_SUBCLASS            0x0a
#define PCI_CLASS_CODE          0x0b

// we only need BAR0
#define PCI_BAR0                0x10

#define PCI_INTERRUPT_LINE      0x3c


typedef struct {
    uint16_t bus;
    uint8_t device;
    uint8_t function;
} pci_bdf;


typedef struct {
    pci_bdf bdf;
    uint8_t interrupt;
    uint32_t bar0;
} pci_device;

// do these addresses just exist in memory, or do I have to 
// explicitly call port byte out/etc? 

// address of the header for this BDF addr in pci configuration space 
//
// 0xfc because bits 1-0 must be zero. (see https://wiki.osdev.org/PCI#PCI_Device_Structure)
//
uint32_t pci_get_addr(pci_bdf bdf, uint8_t offset) {
    return 0x80000000 | (bdf.bus << 16) | (bdf.device << 11) | (bdf.function << 8) | (offset & 0xfc);
}

uint32_t pci_word_at(pci_bdf bdf, uint8_t offset) {
    uint32_t addr = pci_get_addr(bdf, offset);
    port_dword_out(PCI_ADDR_PORT, addr);
    return port_dword_in(PCI_VALUE_PORT);
}

uint16_t pci_get_vendor_id(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x00);
    return word & 0xffff;
}

uint16_t pci_get_device_id(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x00);
    return (word >> 16) & 0xffff;
}

uint8_t pci_get_class_code(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x08);
    return (word >> 24) & 0xff;
}

uint8_t pci_get_subclass(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x08);
    return (word >> 16) & 0xff;
}

uint8_t pci_get_interrupt_line(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x3c);
    return ((word & 0xff) + 0x20); // 0x20 is the pic offset.
}

uint32_t pci_get_bar0(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, PCI_BAR0);
    return word;
}

uint16_t pci_get_command(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x04);
    return word & 0xffff;
}

uint16_t pci_get_status(pci_bdf bdf) {
    uint32_t word = pci_word_at(bdf, 0x04);
    return (word >> 16) & 0xffff;
}

uint16_t pci_set_command(pci_bdf bdf, uint16_t cmd_value) {
    uint32_t addr = pci_get_addr(bdf, 0x04);
    uint32_t value = ((uint32_t) pci_get_status(bdf) << 16) | cmd_value;
    port_dword_out(PCI_ADDR_PORT, addr);
    port_dword_out(PCI_VALUE_PORT, value);
}

bool is_ethernet(pci_bdf target) {
    uint16_t vendor_id = pci_get_vendor_id(target);

    if (vendor_id == 0xffff) { // device doesn't exist
        return false;
    }

    if (vendor_id != 0x8086) { // device is not intel
        return false;
    }

    uint8_t class_code = pci_get_class_code(target);
    uint8_t subclass = pci_get_subclass(target);
    uint16_t device_id = pci_get_device_id(target);

    if (device_id != 4110) {
        return false;
    }

    // 0x02 = network; 0x00 = ethernet
    // this isn't needed, but I like having it.
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

pci_device get_e1000() {
    pci_device e1000;
    e1000.bdf = get_ethernet_bdf();
    if (e1000.bdf.bus == -1) {
        // TODO: better error
        return e1000;
    }

    // enable bus mastering
    uint16_t cmd_reg = pci_get_command(e1000.bdf);
    cmd_reg |= (1 << 2); // from toaru
    cmd_reg |= (1 << 0); // from toaru
    pci_set_command(e1000.bdf, cmd_reg);

    cmd_reg = pci_get_command(e1000.bdf);

    e1000.interrupt = pci_get_interrupt_line(e1000.bdf);

    // enable bus mastering? do we need to do that?

    // determine if it's mem space or io space
    // (it's mem space)
    e1000.bar0 = pci_get_bar0(e1000.bdf) & 0xfffffff0;

    // what's at that register?
    uint32_t w = port_dword_in(e1000.bar0 + 0x0008);

    return e1000;
}


// section 14.5: Transmit Initialization

static tx_desc ring_buf[E1000_NUM_TX_DESC];

static pci_device eth;

void pci_reg_write(uint16_t port, uint32_t data) {
    *((uint32_t*)(eth.bar0 + port)) = data;
}

uint32_t pci_reg_read(uint16_t port) {
    return *((uint32_t*)(eth.bar0 + port));
}

// we can dump this
void transmit_initialization() {
    eth = get_e1000();

    // 1. allocate a region of memory for transmit descriptor list. 
    // (done above)
    //
    // 2. point TDBAL to that region. 
    pci_reg_write(E1000_TDBAL, (uint32_t) ring_buf);

    // 3. set TDLEN to size of descriptor ring (in bytes)
    pci_reg_write(E1000_TDLEN, E1000_NUM_TX_DESC * sizeof(tx_desc));

    // 4. set TDH and TDT to 0
    pci_reg_write(E1000_TDH, 0);
    pci_reg_write(E1000_TDT, 0);

    // 5. set TCTL for:
    //  normal operation, pad short packets, full duplex mode
    pci_reg_write(E1000_TCTL, TCTL_EN | TCTL_PSP | TCTL_COLD);

    // 6. set Transmit IPG
    pci_reg_write(E1000_TIPG, TIPGT);

}

void test_transmit() {
    // TODO: hton function
    //
    // ok. doing "arp_packet" at ALL causes some memory thing? what? lol.
    // is it a kernel size issue again...?
    // RAM issue?
    // no, the byte counts are exactly the same...
    // what? lol
    arp_packet ap;
    ap.htype = 1;
    ap.ptype = 0x08; // reversing bytes of 0x0800
    ap.hlen = 6;
    ap.plen = 4;
    ap.op = 0x0100; // reversed, hton

    // sender is 10.0.2.15. router is at 10.0.2.2. 
    // these are QEMU defaults. 
    ap.spa = (10 << 24) | (0 << 16) | (2 << 8) | 15;
    ap.tpa = (10 << 24) | (0 << 16) | (2 << 8) | 2;

    // get mac address...
    // we need to do this properly, 
    // using base addr registers, 
    // but ignore it for now. 

    tx_desc pkt = {0}; // "memset" to 0

    pkt.addr = (uint32_t) &ap;
    pkt.length = sizeof(arp_packet);
    pkt.cmd = CMD_RS | CMD_RPS | CMD_EOP;

    // send packet
    ring_buf[0] = pkt;

    pci_reg_write(E1000_TDT, 1);

    while (true) {
        if (ring_buf[0].status & STATUS_DD) {
            print("Packet sent!\n");
            break;
        }
    }
}

// so, right now we have the interrupt. we need the device too, though, 
// right? to do useful things with it. 

// from osdev:
// The easiest test you can do is to send an ARP broadcast on the network
// You can use Wireshark both to capture an example of a valid ARP request and to verify your own request has been received by the target host. As far as receiving data, your network card should capture data sent across the local network, even if it is not addressed to your machine.

// ok, so. great. 

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

