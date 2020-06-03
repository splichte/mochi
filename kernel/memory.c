#include "memory.h"
#include <stdint.h>

// set by boot code
#define ADDR_RANGE_DESC_START 0xf000

/* this is KERNEL_ENTRY + SECTORS_TO_READ * 512, 
 * from boot/bootloader.c. 
 * which is 0x10000 + 524288
 * which is 0x10000 + 0x80000
 */
#define KERNEL_START    0x10000
#define KERNEL_END      (KERNEL_START + 0x80000)

// don't write too low, to avoid interrupts, BIOS, etc.
// this wastes some mem but it's OK
#define OK_MEM_START    0x1000


// representation provided by the BIOS
typedef struct _addr_range_desc {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} addr_range_desc;


// linked list of free physical address blocks.
// (2^32 bytes is 4 Gb, btw)
typedef struct _phy_addr_block {
    uint64_t start;
    uint64_t end;
} phy_addr_block;

#define AVAILABLE_RAM   1

// we don't have malloc yet to manage a linked list.
#define MAX_ADDR_BLOCKS 256
static phy_addr_block mem_block_list[MAX_ADDR_BLOCKS];
static n_mem_blocks;

void setup_memory() {
    uint32_t i = ADDR_RANGE_DESC_START;
    uint32_t j = 0; // mem_block_list

    while (1) {
        addr_range_desc d = *((addr_range_desc *) i);
        i += sizeof(addr_range_desc);

        uint64_t base_addr = (d.base_addr_high << 32) | d.base_addr_low;
        uint64_t length = (d.length_low << 32) | d.length_high;

        // set to 0 as the "done" indicator 
        // in `boot/switch_to_pm.asm`
        if (length == 0) break;

        if (d.type != AVAILABLE_RAM) continue;

        phy_addr_block b = { base_addr, base_addr + length };
        mem_block_list[j++] = b;
    }
    n_mem_blocks = j;

    // remove bad regions from the available memory blocks.
    //
    // TODO: handle PCI/ISA memory-mapped devices
    // (not handled now since those addresses are much higher
    // than our available memory will go)
    //
    // also making some assumptions here because the memory 
    // blocks look a certain way on qemu (for instance, that no 
    // returned memory block is entirely in a bad region, only 
    // partially). 
    // that might not hold up on other machines.
    // call print_mem_blocks() to see.
    if (n_mem_blocks == 2) {
        // block 1: 0x00000 to 0x9fc00
        mem_block_list[0].start = OK_MEM_START;

        // block 2: 0x00100000 to 0x07ee0000
        mem_block_list[1].start = KERNEL_END;
    } else {
        print("Unhandled number of memory blocks.\n");
    }
}

void print_mem_blocks() {
    print("Memory blocks (Start, End)\n");
    print("---------------------\n");
    for (int i = 0; i < n_mem_blocks; i++) {
        // casting to 32 for now, since we're not hitting 4Gb...
        print_word((uint32_t) (mem_block_list[i].start));
        print_word((uint32_t) (mem_block_list[i].end));
        print("---------------------\n");
    }
}
