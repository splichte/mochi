#include "memory.h"
#include "hardware.h"
#include "../drivers/screen.h"
#include <stdint.h>

// set by boot code
#define ADDR_RANGE_DESC_START 0xf000

/* this is KERNEL_ENTRY + SECTORS_TO_READ * 512, 
 * from boot/bootloader.c. 
 * which is 0x1000000 + 524288
 * which is 0x1000000 + 0x80000
 */
// we want 16Mb of clearance at the start for ISA devices, 
// which is what linux defines as its DMA zone
#define KERNEL_START    0x1000000
#define KERNEL_END      (KERNEL_START + 0x80000)

// don't write too low, to avoid interrupts, BIOS, etc.
// this wastes some mem but it's OK
#define OK_MEM_START    0x1000000

// don't write too high to avoid PCI devices
// again, wastes some mem, but it's ok. 
// qemu 128ish Mb doesn't get this high.
#define OK_MEM_END      0x80000000  // PCI addresses
                                    // bar0 for e.g. eth
                                    // is 0xfebc0000

#define AVAILABLE_RAM   1

#define MAX_ADDR_BLOCKS 256

// representation provided by the BIOS
typedef struct {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} addr_range_desc;


// linked list of free physical address blocks.
// (2^32 bytes is 4 Gb, btw)
typedef struct {
    uint64_t start;
    uint64_t end;
} phy_addr_block;

// we don't have malloc yet to manage a linked list.
static phy_addr_block mem_block_list[MAX_ADDR_BLOCKS];
static uint32_t n_mem_blocks;

#define MAX_N_PAGES 1024
static page pages[MAX_N_PAGES];
static uint32_t npages;

void page_init(page *p, uint64_t loc) {
    p->flags = 0;
    p->loc = loc;
    p->free_mem_start = 0;
}

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

        if (base_addr + length < OK_MEM_START) continue;

        phy_addr_block b = { base_addr, base_addr + length };
        mem_block_list[j++] = b;
    }
    n_mem_blocks = j;

    // making some assumptions here because the memory 
    // blocks look a certain way on qemu.
    // that might not hold up on other machines.
    // call print_mem_blocks() to see.
    if (n_mem_blocks == 1) {
        if (mem_block_list[0].start < KERNEL_END) {
            mem_block_list[0].start = KERNEL_END;
        }
        if (mem_block_list[0].end > OK_MEM_END) {
            mem_block_list[0].end = OK_MEM_END;
        }
    } else {
        print("ERR: Unhandled number of memory blocks.\n");
        sys_exit();
    }

    // initializing physical pages, assuming we 
    // only have 1 memory block.
    phy_addr_block p = mem_block_list[0];
    npages = (p.end - p.start) / PAGE_SIZE;

    for (int i = 0; i < npages; i++) {
        uint64_t loc = p.start + (PAGE_SIZE * i);
        page_init(pages + i, loc);
    }

    // now, all pages are initialized...
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
