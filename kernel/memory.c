#include "memory.h"
#include "hardware.h"
#include "../drivers/screen.h"
#include <stdint.h>
#include <stddef.h>

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

// TODO: we need space for the stack to grow downwards.
// so maybe we should end a little lower?
//
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



// each page directory entry manages
// a 4Mb chunk. page size is 4Kb, 
// so that means it manages 1000 pages. 
// also, it must be 4kb-aligned. I 
// don't know what the easiest way to 
// ensure that is...

/* bits 12-31 are page table address.
 * bits 9-11 are unused.
 * bits 0-8 are flags.
 * NOTE: must be 4kb-aligned.
 */
typedef struct __attribute__((packed)) {
    uint16_t addr;
    uint16_t addr_free_flags; 
} page_directory_entry;

//typedef struct __attribute__((packed)) {
//
//} page_table_entry;

// we don't have malloc yet to manage a linked list.
static phy_addr_block mem_block_list[MAX_ADDR_BLOCKS];
static uint32_t n_mem_blocks;

#define MAX_N_PAGES 1024
static page pages[MAX_N_PAGES];
static uint32_t npages;

void page_init(page *p, uint64_t loc) {
    p->flags = 0;
    p->loc = loc;
    p->following = 0;
}

typedef struct {
    uint32_t ptr_addr;
    uint32_t sz;
    page *first_page; // allocation may span many pages.
    uint8_t is_active; // could use a bitfield, but premature
} allocation_record;

static page *ap;
static allocation_record *ar;
static uint32_t nals;   // number of allocations

// a dumb simple way would be to have the allocation table
// a fixed size. and just bound the max # allocations
// so, that's what we'll do! (for now)
//
#define ALLOC_TABLE_N_PAGES     64
#define MAX_N_ALLOCATIONS       (ALLOC_TABLE_N_PAGES * PAGE_SIZE / sizeof(allocation_record))

#define ALLOC_ACTIVE    1
#define ALLOC_FREE      0


// on failure, return NULL
allocation_record *add_allocation_record(uint32_t ptr, uint32_t len, page *p) {
    allocation_record r = { ptr, len, p, ALLOC_ACTIVE };

    // find the first non_active allocation record
    for (int i = 0; i < nals; i++) {
        if (ar[i].is_active == ALLOC_FREE) {
            ar[i] = r;
            return (ar + i);
        } 
    }
    // if all records are active, try adding to the end.
    if (nals == MAX_N_ALLOCATIONS) {
        // no space: allocation failed.
        return NULL;
    }

    ar[nals++] = r;
    allocation_record *ret = (ar + nals);
    nals++;
    return ret;
}

void setup_memory() {
    uint32_t i = ADDR_RANGE_DESC_START;
    uint32_t j = 0; // mem_block_list

    while (1) {
        addr_range_desc d = *((addr_range_desc *) i);
        i += sizeof(addr_range_desc);

        uint64_t a = (uint64_t) d.base_addr_high;
        uint64_t f = (uint64_t) d.base_addr_low;
        uint64_t c = (uint64_t) d.length_low;
        uint64_t e = (uint64_t) d.length_high;

        uint64_t base_addr = (a << 32) | f;
        uint64_t length = (e << 32) | c;

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
        print_mem_blocks();
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

    // allocate pages for the allocation records.
    // because the pages were just initialized,
    // we can grab the first ALLOC_TABLE_N_PAGES
    // without any problems -- they will be free 
    // and contiguous
    for (int i = 0; i < ALLOC_TABLE_N_PAGES; i++) {
        page_alloc(pages+i);
    }

    // set the memory from these pages
    // as where we will store the allocation records.
    ar = (allocation_record *) pages[0].loc;
    nals = 0;
}

// weird stuff. why are we "allocating" a page? 
// its struct is already in memory...
// 
void page_alloc(page *p) {
    p->flags |= PAGE_IN_USE;
}

// TODO allocates N pages
page *pages_alloc(uint16_t n) {
    if (n >= (1 << 17)) {
        return NULL;
    }

    // find first free block of N contiguous pages
    for (int i = 0; i < npages; i++) {
    }
}

void page_free(page *p) {
    p->flags &= ~PAGE_IN_USE;
}

typedef struct {
    uint32_t sz;
    page *p;
} alloc_info;


// can page fit sz? if so, return first pos where;
// if not, return -1.
int page_fit_sz(page *p, uint16_t sz) {
    // 0 is unused, 1 is allocated
    uint8_t page_mask[PAGE_SIZE];

    for (int i = 0; i < PAGE_SIZE; i++) {
        page_mask[i] = 0;
    }

    // loop through allocation table to figure out
    // what the used sections of this page are.
    // (TODO: can optimize. allocation table is big;
    // we shouldn't search it all every time)
    for (int i = 0; i < nals; i++) {
        allocation_record r = ar[i];
        if (r.is_active != ALLOC_ACTIVE) continue;

        // allocation is assigned to this page.
        if (r.ptr_addr >= p->loc && r.ptr_addr < p->loc + PAGE_SIZE) {
            uint32_t start = r.ptr_addr - p->loc;
            uint32_t end = start + r.sz;
            if (end > PAGE_SIZE) end = PAGE_SIZE;
            
            for (int j = start; j < end; j++) {
                page_mask[j] = 1;
            }
            continue;
        }
        // else, check if allocation is assigned 
        // to an earlier page that overflows into
        // this one
        uint32_t pst = r.ptr_addr;
        uint32_t pend = r.ptr_addr + r.sz;
        if (pst < p->loc && pend >= p->loc) {
            // the buffer starts before this 
            // page's start, and ends 
            // after its end.
            // so the entire page is filled;
            // we can't fit anything
            if (pend > p->loc + PAGE_SIZE) {
                return -1; 
            }
            // else, the buffer starts before this 
            // page's start, and ends 
            // somewhere inside it.
            uint32_t start = p->loc;
            uint32_t end = pend;

            for (int j = start; j < end; j++) {
                page_mask[j] = 1;
            }
        }
        // else, this allocation record 
        // doesn't intersect this page at all.
        // continue to the next... 
    }

    // basically do string processing on this
    // looking for a run of 0s
    for (int i = 0; i < PAGE_SIZE - sz; i++) {
        int j = i;
        for (; j < PAGE_SIZE; j++) {
            if (page_mask[j] == 1) break;
        }
        // we found a buffer of length sz 
        // that starts at i!
        if ((j - i) == sz) {
            return i;
        }
    }
    return -1;
}

void *kmalloc(uint32_t sz) {
    // can the size fit on one page?
    if (sz <= PAGE_SIZE) {
        // find first page with that much space.
        // NOTE: page iters are REALLY slow. :(

        // start after the dedicated blocks 
        // for the allocation tables.
        for (int i = ALLOC_TABLE_N_PAGES; i < npages; i++) {
            if (pages[i].flags & PAGE_FULL) continue;
            if (pages[i].flags & PAGE_IN_USE) {
                // does page have a free region big enough?
                int pos = page_fit_sz(pages+i, sz);
                if (pos >= 0) {
                    // start pt
                    uint32_t start_pt = pages[i].loc + pos;
    
                    // try to add an allocation record
                    if (add_allocation_record(start_pt, sz, pages+i) == NULL) {
                        return NULL;
                    }
                    return (void *) start_pt;
                }
            } else {
                // this page isn't used; allocate it
                page_alloc(pages+i);            
                uint32_t start_pt = pages[i].loc;
                // try to add an allocation record
                if (add_allocation_record(start_pt, sz, pages+i) == NULL) {
                    return NULL;
                }
                return (void *) start_pt;

            }
        }
        print("done searching pages\n");
        // out of memory.
        return NULL;
    }
    print("branch 2\n");

    // we need multiple contiguous pages, so
    // try to find ceil(sz/PAGE_SIZE) free pages
    // ignore the optimization that we don't need the 
    // first/last pages to be totally free -- just 
    // at their end/start, respectively.
    uint32_t wanted_npages = (sz / PAGE_SIZE) + 1;
    for (int i = 0; i < npages - wanted_npages; i++) {
        int j = 0;
        for (; j < wanted_npages; j++) {
            if (pages[i + j].flags & PAGE_FULL) break;
        }
        if (j == wanted_npages) {
            // this is same code as above, basically.
            uint32_t start_pt = pages[i].loc;

            if (add_allocation_record(start_pt, sz, pages+i) == NULL) {
                return NULL;
            }

            return (void *) start_pt;
        }
    }
}

void kfree(void *p) {
    // find matching allocation record and mark it
    for (int i = 0; i < nals; i++) {
        if (ar[i].ptr_addr = (uint32_t) p) {
            ar[i].is_active = ALLOC_FREE;
            return;
        }
    }

    print("Fatal error: attempt to free pointer not in allocation record list.\n");
    sys_exit();
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
