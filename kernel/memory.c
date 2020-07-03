#include "memory.h"
#include "hardware.h"
#include "../drivers/screen.h"
#include <stdint.h>
#include <stddef.h>

// 128 Mb.
#define MEMORY_LIMIT    0x8000000

/* this is KERNEL_ENTRY + SECTORS_TO_READ * 512, 
 * from boot/bootloader.c. 
 * which is 0x1000000 + 524288
 * which is 0x1000000 + 0x80000
 */
// we want 16Mb of clearance at the start for ISA devices, 
// which is what linux defines as its DMA zone
// (we reserve a 1Mb region, starting at 16Mb....)
//
#define KERNEL_START    0x1000000
#define KERNEL_END      (KERNEL_START + 0x100000)

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

// set in ../boot/bootloader.c, where the page_table_stack 
// is mapped to (+ the range)
#define STACK_START 0x03400000 

// free pages sit in this region (for now). 
#define N_FREE_PAGES    (MEMORY_LIMIT - STACK_START) / PAGE_SIZE

// for 128Mb, the above value is 0x4C00, or 19456

// ...wait. this is DEFINITELY going to require more virtual pages
// to be mapped :/

// if we're using 100kb rofl
// so annoy


// turning ppages on causes some BSS issue with objdump...
// probably our script is breaking it hehe

// this takes up sizeof(physical_page) * 19456 bytes -- about 100Kb. 
static physical_page ppages[N_FREE_PAGES];

// should use a pqueue or something to find free pages more quickly...
// but for now, we do this. 
//uint32_t next_free() {
//    uint8_t page_found = 0;
//    for (int i = 0; i < N_FREE_PAGES; i++) {
//        if (!ppages[i].in_use) {
//            ppages[i].in_use = 1;
//            return ppages[i].start;
//        }
//    }
//    if (!page_found) {
//        print("out of physical pages\n");
//        sys_exit();
//    }
//}

void setup_memory() {

}
