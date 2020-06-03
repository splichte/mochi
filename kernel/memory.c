#include "memory.h"
#include <stdint.h>

#define ADDR_RANGE_DESC_START 0xf000

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
    uint64_t base_addr;
    uint64_t len;
} phy_addr_block;

#define AVAILABLE_RAM   1

// we don't have malloc yet to manage a linked list.
#define MAX_ADDR_BLOCKS 512
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

        phy_addr_block b = { base_addr, length };
        mem_block_list[j++] = b;
    }
    n_mem_blocks = j;
}

void print_mem_blocks() {
    print("Memory blocks (Base Address, Length)\n");
    print("---------------------\n");
    for (int i = 0; i < n_mem_blocks; i++) {
        // casting to 32 for now, since we're not hitting 4Gb...
        print_word((uint32_t) (mem_block_list[i].base_addr));
        print_word((uint32_t) (mem_block_list[i].len));
        print("---------------------\n");
    }
}
