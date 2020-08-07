#include "../drivers/screen.h"
#include "hardware.h"
#include "interrupts.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "devices.h"
#include "string.h"

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

#define ETHERNET_MAX_PACKET_SIZE    1518    // in bytes


// https://en.wikipedia.org/wiki/EtherType
#define ETHERTYPE_IPV4  0x0800
#define ETHERTYPE_ARP   0x0806
#define ETHERTYPE_IPV6  0x86DD

// https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17


// CRC32, from http://home.thep.lu.se/~bjorn/crc/
// I kept the original formatting. 
uint32_t crc32_for_byte(uint32_t r) {
    for(int j = 0; j < 8; ++j)
        r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    if(!*table)
        for(size_t i = 0; i < 0x100; ++i)
            table[i] = crc32_for_byte(i);
    for(size_t i = 0; i < n_bytes; ++i)
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}


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

/* https://tools.ietf.org/html/rfc2131#section-2 */
// https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol#Discovery
typedef struct __attribute__((packed)) {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;

    uint32_t xid;

    uint16_t secs;
    uint16_t flags;

    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;

    char chaddr[16];

    char sname[64];

    char file[128];

    /* options go here */
    // what's the standard way of doing this?
} dhcp_msg;


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
        print("not found\n");
        return e1000;
    }
    // enable bus mastering
    uint16_t cmd_reg = pci_get_command(e1000.bdf);
    cmd_reg |= (1 << 2); // from toaru
    cmd_reg |= (1 << 0); // from toaru
    pci_set_command(e1000.bdf, cmd_reg);

    cmd_reg = pci_get_command(e1000.bdf);

    e1000.interrupt = pci_get_interrupt_line(e1000.bdf);


    // evaluates to 0xfebc0000 on qemu
    e1000.bar0 = pci_get_bar0(e1000.bdf) & 0xfffffff0;

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
    uint32_t rb_loc = ((uint32_t) ring_buf) - KERNEL_OFFSET;
    pci_reg_write(E1000_TDBAL, rb_loc);

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

uint16_t htons(uint16_t i) {
    uint16_t o = 0;
    o |= (i >> 8) & 0xff;
    o |= (i << 8) & 0xff00;
    return o;
}

uint32_t hton(uint32_t i) {
    uint32_t o = 0;
    o |= (i >> 24) & 0xff;
    o |= (i >> 8) & 0xff00;
    o |= (i << 8) & 0xff0000;
    o |= (i << 24) & 0xff000000;
    return o;
}

typedef struct {
    // TODO: fix these? nah.
//    uint8_t preamble[7];
//    uint8_t sfd;
    uint8_t mac_dest[6];
    uint8_t mac_src[6];
    uint16_t ethertype;
    uint8_t buf[1500];
    // maximum payload (1500) + frame check sequence (4), interpacket gap (12)
//    char payload[1500]; // hmm
//    uint32_t crc; // frame check sequence
//    uint8_t ipg[12]; // interpacket gap
} eth_frame;

typedef struct {
    uint8_t version_ihl;
    uint8_t dscp_ecn;
    uint16_t total_len;
    uint16_t ident;
    uint16_t flags_frag_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} ip_hdr;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} udp_hdr;

//static arp_packet ap;
//static dhcp_msg dm;

static eth_frame ef;

/*
void set_preamble(eth_frame *ef) {
    for (int i = 0; i < 7; i++) {
        ef->preamble[i] = 0xaa; 
    }
    ef->sfd = 0xab;
}
*/

// https://en.wikipedia.org/wiki/IPv4_header_checksum
void ipv4_chksum(ip_hdr *ih) {
    // at this point, the checksum field itself should be 0.
    uint32_t chksum = 0;
    uint8_t *ihp = (uint8_t *) ih;
    for (int i = 0; i < sizeof(ip_hdr); i += 2) {
        uint16_t hi = ((uint16_t) ihp[i]) << 8;
        uint16_t lo = ((uint16_t) ihp[i+1]);
        chksum += hi + lo;
    }

    uint8_t carry_count = (chksum / 0x10000);

    uint16_t sum16 = chksum & 0xffff;
    
    uint32_t ochk = sum16 + carry_count;
    carry_count = (ochk / 0x10000);
    ochk = ochk - 0x10000 + carry_count;

    uint16_t f16 = ochk & 0xffff;
    f16 = ~(f16 & f16); // invert all bits
    ih->checksum = htons(f16);
}

void ip_cksum_test() {
    // example from Wikipedia
    uint8_t arr[] = {
        0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 
        0x40, 0x11, 0x00, 0x00, 0xc0, 0xa8, 0x00, 0x01, 
        0xc0, 0xa8, 0x00, 0xc7,
    };

    print("ip cksum test: \n");
    ipv4_chksum((ip_hdr *) arr); // expected: 0xb861
}

void test_transmit() {
    // zero out eth frame
    eth_frame e = { 0 };
    ef = e;

    // for DHCP discover
    uint8_t dhcp_options[] = {
        0x63, 0x82, 0x53, 0x63,
        0x35, 0x01, 0x01,
        0x32, 0x04, 0xc0, 0xa8, 0x01, 0x64,
        0x37, 0x04, 0x01, 0x03, 0x0f, 0x06,
        0xff };

    //========================= IP PACKET

    ip_hdr ip = { 0 };
    ip.version_ihl = 4 << 4;
    ip.version_ihl |= 5;

    ip.ttl = 0xff; // just needs to be nonzero.

    ip.total_len = htons(sizeof(ip_hdr) + sizeof(udp_hdr) + sizeof(dhcp_msg) 
            + sizeof(dhcp_options));
 
    ip.protocol = IP_PROTOCOL_UDP;

    ip.src_addr = hton(0);
    ip.dst_addr = hton(0xffffffff); // 255.255.255.255

    // need to be VERY careful about endianness.
    ipv4_chksum(&ip);

    memmove(ef.buf, &ip, sizeof(ip_hdr));



    //====================== UDP PACKET
    udp_hdr udp = { 0 };
    udp.src_port = htons(68); // dhcp
    udp.dst_port = htons(67);
    udp.len = htons(sizeof(udp_hdr) + sizeof(dhcp_msg) + sizeof(dhcp_options));

    memmove(ef.buf + sizeof(ip_hdr), &udp, sizeof(udp_hdr));




    //===================== DHCP PACKET
    //
    // https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol#Operation
    dhcp_msg dm = { 0 };
    dm.hlen = 0x06;
    dm.htype = 0x01;
    dm.op = 0x01;
    dm.xid = hton(0x3903f326);


    // what's my mac address?
    uint32_t mac_addr = hton(82); // from debugging output of qemu

    // TODO: acquire mac addr in the "proper" way
    memmove(dm.chaddr, &mac_addr, 16);

    memmove(ef.buf + sizeof(ip_hdr) + sizeof(udp_hdr), &dm, sizeof(dhcp_msg));

    // now write the DHCP options
    uint32_t bufi = sizeof(ip_hdr) + sizeof(udp_hdr) + sizeof(dhcp_msg);
    uint8_t *write_loc = ef.buf + bufi;
    memmove(write_loc, dhcp_options, sizeof(dhcp_options));


    //=================== ETHERNET PACKET
 
    // we have to wrap the DHCP packet in an ethernet frame.
    // where do we get one of those? 
    // TODO: unclear if we should actually be setting this
    // oh, f. I don't know if I actually need to attach 
    // the CRC. shoot. 

    //    set_preamble(&ef);
    memset(ef.mac_dest, 0xff, 6);
    memmove(ef.mac_src, &mac_addr, 6);
    // how big do we want to send? 
    // should be size of IP/UDP message. but this works.
    ef.ethertype = htons(ETHERTYPE_IPV4); 

//    uint32_t crc = 0;
//    uint32_t content_len = sizeof(eth_frame) - 16;
//    crc32(ef.buf, content_len, &crc);

//    memmove(ef.buf+i, &crc, 4);
    // interpacket gap
//    memset(ef.buf+i+4, 0x00, 12); 


    //====================================== 

    // e1000 TX descriptor

    tx_desc pkt = {0}; // "memset" to 0

    pkt.addr = ((uint32_t) &ef) - KERNEL_OFFSET;

    pkt.length = sizeof(eth_frame);
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

void test_transmit_arp() {
    // TODO: hton function
    //
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

    pkt.addr = ((uint32_t) &ap) - KERNEL_OFFSET;


    print("pkt addr\n");
    print_word(pkt.addr);
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

