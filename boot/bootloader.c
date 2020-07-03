#include <stdint.h>
#include "../drivers/disk.h"
#include "../kernel/hardware.h"
#include "../drivers/screen.h"

// The page directory is located at the 
// start at the 2nd page table, address 0x40.0000
// we will use 2 page tables' worth of entries
// to store the page directory + page table data.
//
// the page tables are 4096-byte (0x1000) chunks 
// in a linear sequence from the base addr, 
// which starts at 0x40.1000
// each matches with the physical 4Mb chunk
// (not the virtual chunk) it handles.
// so for the Nth 4Mb chunk of physical RAM, 
// the page table can be found at 0x40.1000 
// + N * 0x1000. 
//
//
uint32_t page_directory_addr = 0x400000;
uint32_t *page_directory = (uint32_t *) 0x400000;

void set_pd_entry(uint16_t i, uint32_t pt_addr) {
    page_directory[i] = pt_addr | 1;
}

void init_pt(uint32_t pt_addr, uint32_t start_addr) {
    uint32_t *pt = (uint32_t *) pt_addr;
    for (int i = 0; i < 1024; i++) {
        uint32_t i_addr = start_addr + (i * 0x1000);
        pt[i] =  i_addr | 1;
    }
}

void set_initial_page_tables() {
    // ==== Intel MMU Notes
    //
    // We have bits 22-31. (10 bits) for the dir
    // bits 12-21 (10 bits) for the page
    // and bits 0-11 (12 bits) for the offset.
    //
    // So when you see a line below like:
    //      `uint16_t pd_vlow_i = 768; // 0xc000.0000`
    // 768, the index into the 1024-entry page directory
    // (1024 entries since 2^10 == 1024),
    // is obtained by taking the most significant 10 bits
    // from the desired virtual address 0xc000.0000. 


    // 0x1000 == 4096 (the size of the page directory and page tables
    // in bytes)
    uint32_t pt_base_addr = page_directory_addr + 0x1000;

    // ==== Low Pages (Screen)
    //
    // physical address range: 0x0000.0000 to 0x0040.0000
    // virtual address range: 0xc000.0000 to 0xc040.0000
    //
    // identity-mapped.
    //
    // this covers the video address 0xb8000 (see drivers/screen.c)

    uint16_t pd_plow_i = 0; // 0x0000.0000
    uint16_t pd_vlow_i = 768; // 0xc000.0000

    uint32_t pt_low_addr = pt_base_addr;
    init_pt(pt_low_addr, 0x0);

    set_pd_entry(pd_plow_i, pt_low_addr);
    set_pd_entry(pd_vlow_i, pt_low_addr);


    // ==== Kernel Pages
    //
    // physical address range: 0x0100.0000 to 0x0140.0000
    // virtual address range: 0xc100.0000 to 0xc140.0000
    //
    // identity-mapped.

    uint16_t pd_pkernel_i = 4; // 0x0100.0000
    uint16_t pd_vkernel_i = 772; // 0xc100.0000

    uint32_t pt_kernel_addr = pt_base_addr + pd_pkernel_i * 0x1000;
    init_pt(pt_kernel_addr, 0x01000000);

    set_pd_entry(pd_pkernel_i, pt_kernel_addr);
    set_pd_entry(pd_vkernel_i, pt_kernel_addr);


    // ==== Page Directory + Page Table Pages
    //
    // physical address range: 0x40.0000 to 0x80.0000
    // virtual address range: 0xc040.0000 to 0xc0c0.0000
    //
    // NOT identity-mapped. 
    // we don't need to identity-map the physical address range.
    //

    // first page we need (0x40.0000 to 0x80.0000)
    uint16_t pd_pt_i = 1;
    uint16_t pd_vpt_i = 769; // 0xc040.0000
    uint32_t pt_page_addr = pt_base_addr + pd_pt_i * 0x1000;
    init_pt(pt_page_addr, 0x400000);
    set_pd_entry(pd_vpt_i, pt_page_addr);

    // second page we need (0x80.0000 to 0xc0.0000)
    pd_vpt_i++;
    pt_page_addr += 0x1000;
    init_pt(pt_page_addr, 0x800000);
    set_pd_entry(pd_vpt_i, pt_page_addr);



    // ==== Stack Pages (ebp + esp) (in virtual memory)
    //
    // physical address range: 0x0180.0000 to 0x01c0.0000 
    // virtual address range: 0xf000.0000 to 0xf040.0000
    //
    // NOT identity-mapped.
    //
    // 0x0180.0000 is at roughly 24Mb in RAM.
    // it gives 0x0100.0000 to 0x0100.0000, or 8Mb, for 
    // the kernel.
    //
    // if the virtual address is changed, make sure to change
    // kernel_entry.asm (which sets ebp + esp)

    uint16_t pd_pstack_i = 6; // 0x0180.0000
    uint16_t pd_vstack_i = 960; // 0xf000.0000
    uint32_t pt_stack_addr = pt_base_addr + pd_pstack_i * 0x1000;
    init_pt(pt_stack_addr, 0x01800000); 
    set_pd_entry(pd_vstack_i, pt_stack_addr);
}

void setup_vmem() {
    set_initial_page_tables();

    asm volatile ("mov %0, %%eax" : : "r" (page_directory_addr));

    asm volatile ("mov %%eax, %%cr3\n\t"
                  "mov %%cr0, %%eax\n\t" 
                  "or $0x80000000, %%eax\n\t"
                  "mov %%eax, %%cr0" ::);

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
