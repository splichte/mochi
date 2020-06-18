#include <stdint.h>
#include "../drivers/disk.h"
#include "../kernel/hardware.h"
#include "../drivers/screen.h"

static uint32_t page_directory[1024] __attribute__((aligned (4096)));
static uint32_t page_table[1024] __attribute__((aligned (4096)));
static uint32_t page_table_low[1024] __attribute__((aligned (4096)));

void make_table(uint32_t *pt, uint32_t start_addr) {
    for (int i = 0; i < 1024; i++) {
        uint32_t i_addr = (uint32_t) (start_addr + (i * 0x1000));
        pt[i] =  i_addr | 1;
    }
}

void set_pd_entry(uint16_t i, uint32_t *pt) {
    page_directory[i] = ((uint32_t) pt) | 1;
}

/* we have to fix all the pointers before we set up the MMU, 
 * since our addresses are all messed up pre-MMU. 
 * make_table_pre_mmu and set_pd_entry_pre_mmu do this. */
void make_table_pre_mmu(uint32_t *pt, uint32_t start_addr) {
    uint32_t real_pt = ((uint32_t) pt);
    make_table((uint32_t *) real_pt, start_addr);
}

void set_pd_entry_pre_mmu(uint16_t i, uint32_t *pt) {
    uint32_t real_pt_addr = ((uint32_t) pt);
    uint32_t real_pd = ((uint32_t) page_directory);
    uint32_t *rpd = (uint32_t *) real_pd;
    rpd[i] = real_pt_addr | 1;
}


void set_initial_page_tables() {
    // we have bits 22-31. (10 bits) for the dir
    // bits 12-21 (10 bits) for the page
    // and bits 0-11 (12 bits) for the offset.

    make_table_pre_mmu(page_table_low, 0x0);
    make_table_pre_mmu(page_table, 0x01000000);

    set_pd_entry_pre_mmu(0, page_table_low);

    set_pd_entry_pre_mmu(4, page_table);
    // 0xc1000000, where we set text section of kernel
    set_pd_entry_pre_mmu(772, page_table);

    // 1100 0001 00
}

void setup_vmem() {
    set_initial_page_tables();

    uint32_t real_page_dir_addr = ((uint32_t) page_directory);

    asm volatile ("mov %0, %%eax" : : "r" (real_page_dir_addr));

    asm volatile ("mov %%eax, %%cr3\n\t"
                  "mov %%cr0, %%eax\n\t" 
                  "or $0x80000000, %%eax\n\t"
                  "mov %%eax, %%cr0" ::);

    // why do we need to translate? shouldn't it be translating
    // these higher addresses correctly?
    // or maybe the MMU doesn't translate page table/page directory
    // addresses (since it needs those to even start to translate...)

    // we've loaded appropriately! now we can 
    // add the other entries we need (for MMIO)
//    make_table_pre_mmu(page_table_bar, 0xfebc0000);
    make_table_pre_mmu(page_table_low, 0x0); 
    // first 10 bits of 0xfebc: 1111111010 = 1018
//    set_pd_entry_pre_mmu(1018, page_table_bar);
    set_pd_entry_pre_mmu(768, page_table_low);

    // refresh cr3
    asm volatile ("mov %%cr3, %%eax\n\t"
                "mov %%eax, %%cr3" : :);
}


// 1024 sectors gives 512 kb. that's 
// pretty good. 
// kernel.bin is 17kb right now. 
// 1024 or so is reasonable. 
// 16384 starts to take a while...
// like, longer than I think it should.
//
#define SECTORS_TO_READ 1024

#define KERNEL_START_SECTOR 32

// give 16Mb of room for ISA devices, etc.
// see Linux Kernel Development 3rd ed.
#define KERNEL_ENTRY        0x1000000

int main() {
    clear_screen(); 
    print("Running bootloader...\n");

    uint64_t i = KERNEL_START_SECTOR; /* start at sector 8 -- aka 4K */

    uint8_t *buf;

    // "sectors to read" can be at max 2^8 - 1 = 255
    uint8_t chunk_size = 1;

    // okay, so. changing the chunk size is doing something
    // interesting affecting the behavior...

    for (; i < SECTORS_TO_READ; i += chunk_size) {
        buf  = (uint8_t *) (KERNEL_ENTRY + (i - KERNEL_START_SECTOR) * DISK_SECTOR_SIZE);
        disk_read_bootloader(i, buf, chunk_size);
    }
    if (i != SECTORS_TO_READ) {
        uint8_t diff = (SECTORS_TO_READ - i);
        buf = (uint8_t *) (KERNEL_ENTRY + (i - KERNEL_START_SECTOR) * DISK_SECTOR_SIZE);
        disk_read_bootloader(i, buf, diff);
    }

    setup_vmem();

    // jump to start of kernel and execute from there. 
    uint32_t start_addr = KERNEL_ENTRY + KERNEL_OFFSET;


    print("about to jump.\n");
    print_word(start_addr);

    asm volatile ("jmp %0" : : "r" (start_addr));

    return 0;
}
