/* memory.c deals with "raw" RAM, not with kernel memory allocators.
 * for those allocators, look at kalloc.*/

#include "memory.h"
#include "hardware.h"
#include "screen.h"
#include <stdint.h>
#include <stddef.h>

// 128 Mb.
#define MEMORY_LIMIT            0x8000000
#define MEMORY_LIMIT_IN_MB      128

// TODO: we need space for the stack to grow downwards.
// so maybe we should end a little lower?
//
// don't write too high to avoid PCI devices
// again, wastes some mem, but it's ok. 
// qemu 128ish Mb doesn't get this high.
#define OK_MEM_END      0x80000000  // PCI addresses
                                    // bar0 for e.g. eth
                                    // is 0xfebc0000

// set in ../boot/bootloader.c, where the page_table_stack 
// is mapped to (+ the range)
#define FREE_START 0x01c00000 


// free pages sit in this region (for now). 
#define N_FREE_PAGES    (MEMORY_LIMIT - FREE_START) / PAGE_SIZE

// for 128Mb, the above value is 0x4C00, or 19456

// this takes up sizeof(physical_page) * 19456 bytes -- about 100Kb. 
static physical_page ppages[N_FREE_PAGES];


// the initial locations of these objects in memory!
#define PAGE_DIRECTORY_START    0xc0400000
#define PAGE_TABLE_START        (PAGE_DIRECTORY_START + 0x1000)

uint32_t *pd = (uint32_t *) PAGE_DIRECTORY_START;

// start at the 7th page table. (after the stack, 
// which is entry #6 in the page directory)
// this corresponds to about 0x0240.0000 in RAM, which is about 28Mb.
//

#define START_PAGE_TABLE_N    7

#define MB_PER_PAGE_TABLE   4

// for 128Mb main memory, comes out to 32 tables.
#define N_PAGE_TABLES   (MEMORY_LIMIT_IN_MB / MB_PER_PAGE_TABLE)

uint32_t *pt_at_index(uint16_t i) {
    return (uint32_t *) (PAGE_TABLE_START + i * 0x1000);
}

#define PAGE_PRESENT    1


// 4 bytes. each page is 4Kb, 
// so there are 25600 entries. 
// = 4 * 25600 ==  100 kb.
// that's a good amount of space!
// but we have 8Mb allocated for kernel
// and our kernel code is only 27kb right now,
// so we're fine.
// 
static physical_page ppages[N_FREE_PAGES];

void init_physical_pages() {
    uint32_t paddr = FREE_START;
    for (int i = 0; i < N_FREE_PAGES; i++) {
        // defaults low bit to "not in-use"
        ppages[i].start = paddr + i * 0x1000;
    }
}

uint32_t alloc_page() {
    for (int i = 0; i < N_FREE_PAGES; i++) {
        uint32_t start_addr = ppages[i].start;

        // in-use
        if (start_addr & 1) continue;

        // otherwise, set as in-use and return!
        ppages[i].start |= 1;

        // when we cached start_addr, it didn't have the flag set. 
        // otherwise, we'd have to xor off the present flag.
        return start_addr;
    }

    print("no free page :(\n");
    sys_exit();
}

void free_page(uint32_t addr) {
    // TODO: this is wayy more inefficient than it needs to be. 
    // linear search! yuck!
    for (int i = 0; i < N_FREE_PAGES; i++) {
        if (ppages[i].start == addr + 1) {
            ppages[i].start -= 1;
            return;
        }
    }

    print("Fatal error: page not found.\n");
    sys_exit();
}


// we use this bitmap to check if the page table is in use or not. 
// there are 1024 page tables. 
static uint32_t pt_bitmap[32];

void set_pt_bit(uint16_t i) {
    uint8_t val = i / 32;
    uint8_t offset = i % 32;
    pt_bitmap[val] |= 1 << offset;
}

void unset_pt_bit(uint16_t i) {
    uint8_t val = i / 32;
    uint8_t offset = i % 32;
    pt_bitmap[val] ^= 1 << offset;
}


uint8_t pt_bit_is_set(uint16_t i) {
    uint8_t val = i / 32;
    uint8_t offset = i % 32;
    uint32_t present = pt_bitmap[val] & (1 << offset);
    return (present > 0);
}

// get the address of a free page table.
uint32_t *get_free_page_table() {
    uint8_t found = 0;
    for (int i = 0; i < 1024; i++) {
        if (!pt_bit_is_set(i)) {
            set_pt_bit(i);

            // do we have to correct this? 
            return pt_at_index(i);
        }
    }

    // no page tables free. need to do eviction~~
    if (!found) {
        print("no page tables free. aborting...\n");
        sys_exit();
    }
}

// we have a virtual address we need a page for. 
uint8_t map_free_page(uint32_t virtual_addr) {
    uint16_t pd_i = virtual_addr >> 22;
    uint16_t pt_i = (virtual_addr >> 12) & 0x3ff; // 0011 1111 1111
//    print("looking for page directory index: ");
//    print_word(pd_i);
//    print("page table index: ");
//    print_word(pt_i);

    // get a free physical page. 
    // TODO: this should be able to fail.
    // when it does...time to evict!

    uint32_t phy_page_addr = alloc_page();


    // CASE 1: this 4Mb section is totally unassigned! 
    // (not marked "present" in the page directory)
    // we need to assign both a page table
    // and then assign the page into the page table.

    uint32_t pd_entry = pd[pd_i];
    // entry not present. 
    if (!(pd_entry & 1)) {
//        print("pd index: "); print_word(pd_i);
//        print("pd entry: "); print_word(pd_entry);
        // get a free page table (using bitmap)
        uint32_t *pt = get_free_page_table();

        // update page directory with this new table!
        // NOTE: we have to correct for the mapping...
        pd[pd_i] = ((uint32_t) pt - KERNEL_OFFSET) | 1;

        // set the physical page 
        pt[pt_i] = phy_page_addr | 1;

        return 0;
    }

    // CASE 2: this 4Mb section is "present". 
    // we only need to map a new page. 

    // chop off the low 12 bits to get the page table address
    // of this entry
    uint32_t *pt = (uint32_t *) ((pd_entry & 0xfffff000) + KERNEL_OFFSET);
//    print("pt addr: "); print_word((uint32_t) pt);
//    print("page allocated: "); print_word(phy_page_addr);
    pt[pt_i] = phy_page_addr | 1;


    // we'll handle this later...
//    if (!page_found) {
//        print("out of physical pages\n");
//        sys_exit();
//    }
}

void zero_page_directory() {
    // from bootloader. the page tables
    // that had "init pt" called on them.
    set_pt_bit(0);
    set_pt_bit(1);
    set_pt_bit(2);
    set_pt_bit(4);
    set_pt_bit(6);

    for (int i = 0; i < N_PAGE_TABLES; i++) {
        // we've set these entries:
        // 0, 1, 4, 6, 768, 769, 772, 960, 1018
        // (see boot/bootloader.c)
        // don't change those, but zero everything else.
        switch(i) {
            case 0:
            case 1:
            case 2:
            case 4:
            case 6:
            case 768:
            case 769:
            case 772:
            case 960:
            case 1018:
                break;
            default:
                pd[i] = 0;
                
        }
    }
}

void zero_pages() {
    for (int i = START_PAGE_TABLE_N; i < N_PAGE_TABLES; i++) {
        uint32_t *page_table = pt_at_index(i);

        for (int j = 0; j < 1024; j++) {
            page_table[j] = 0;
        }
    }
}

void test_pd_presence() {
    for (int i = 0; i < 10; i++) {
        uint32_t t = pd[i];
        if (t & 0x1) {
           print("present!\n");
        } else {
            print("nope!\n");
        }
    }

}

void setup_memory() {
    // TODO: this should be done in the bootloader, not here...
    zero_page_directory();
    // zero pages, so we can can check if the other ones are "present"!
    zero_pages();

    init_physical_pages();
}

