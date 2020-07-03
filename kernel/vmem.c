#include "memory.h"
#include "hardware.h"
#include "devices.h"
#include "../drivers/screen.h"


// we need to handle the mappings properly. 
// right now we have some memory that's somewhere. 
// how on earth do we fix up the page tables 
// once we're already inside vmem?

__attribute__ ((interrupt))
void page_fault_handler(struct interrupt_frame *frame, uint32_t error_code) {
    asm volatile ("cli");
//    print("exception #14!\n");
    uint32_t mem_addr;
    asm volatile ("mov %%cr2, %%eax\n\t"
            "mov %%eax, %0" : "=r" (mem_addr) :); 

//    print("attempted to access: "); print_word(mem_addr); 

    map_free_page(mem_addr);

    // TODO: if all pages used, evict some page


//    HALT();
    asm volatile ("sti");

}


void finish_vmem_setup() {
    // by this point, we're already in the higher kernel.
}

//// virtual memory start!
//// in the future, there will be multiple of these...
//uint32_t page_directory[1024] __attribute__((aligned (4096)));
//
//// hmmm
//uint32_t page_table[1024] __attribute__((aligned (4096)));
//uint32_t page_table_low[1024] __attribute__((aligned (4096)));
//uint32_t page_table_bar[1024] __attribute__((aligned (4096)));
//
//
//void make_table(uint32_t *pt, uint32_t start_addr) {
//    for (int i = 0; i < 1024; i++) {
//        uint32_t i_addr = (uint32_t) (start_addr + (i * 0x1000));
//        pt[i] =  i_addr | 1;
//    }
//}
//
//void set_pd_entry(uint16_t i, uint32_t *pt) {
//    page_directory[i] = ((uint32_t) pt) | 1;
//}
//
///* we have to fix all the pointers before we set up the MMU, 
// * since our addresses are all messed up pre-MMU. 
// * make_table_pre_mmu and set_pd_entry_pre_mmu do this. */
//void make_table_pre_mmu(uint32_t *pt, uint32_t start_addr) {
//    uint32_t real_pt = ((uint32_t) pt) - KERNEL_OFFSET;
//    make_table((uint32_t *) real_pt, start_addr);
//}
//
//void set_pd_entry_pre_mmu(uint16_t i, uint32_t *pt) {
//    uint32_t real_pt_addr = ((uint32_t) pt) - KERNEL_OFFSET;
//    uint32_t real_pd = ((uint32_t) page_directory) - KERNEL_OFFSET;
//    uint32_t *rpd = (uint32_t *) real_pd;
//    rpd[i] = real_pt_addr | 1;
//}
//
//
//void set_initial_page_tables() {
//    // we have bits 22-31. (10 bits) for the dir
//    // bits 12-21 (10 bits) for the page
//    // and bits 0-11 (12 bits) for the offset.
//
//    make_table_pre_mmu(page_table_low, 0x0);
//    make_table_pre_mmu(page_table, 0x01000000);
//    make_table_pre_mmu(page_table_bar, 0xfebc0000);
//
//    set_pd_entry_pre_mmu(4, page_table);
//    // 0xc1000000, where we set text section of kernel
//    set_pd_entry_pre_mmu(772, page_table);
//
//    // 1100 0001 00
//}
//
//void dump_position_regs() {
//    uint32_t esp, ebp;
//    asm volatile ("mov %%ebp, %0" : "=r" (ebp) :);
//    asm volatile ("mov %%esp, %0" : "=r" (esp) :);
//    print("esp: "); print_word(esp);
//    print("ebp: "); print_word(ebp);
////    PRINT_EIP();
//}
//
//void finish_setup();
//
//void setup_vmem() {
//    set_initial_page_tables();
//
//    uint32_t real_page_dir_addr = ((uint32_t) page_directory) - KERNEL_OFFSET;
//
//    asm volatile ("mov %0, %%eax" : : "r" (real_page_dir_addr));
//
//    asm volatile ("mov %%eax, %%cr3\n\t"
//                  "mov %%cr0, %%eax\n\t" 
//                  "or $0x80000000, %%eax\n\t"
//                  "mov %%eax, %%cr0" ::);
//
//    // why do we need to translate? shouldn't it be translating
//    // these higher addresses correctly?
//    // or maybe the MMU doesn't translate page table/page directory
//    // addresses (since it needs those to even start to translate...)
//
//    // we've loaded appropriately! now we can 
//    // add the other entries we need (for MMIO)
//    make_table_pre_mmu(page_table_bar, 0xfebc0000);
//    make_table_pre_mmu(page_table_low, 0x0); 
//    // first 10 bits of 0xfebc: 1111111010 = 1018
//    set_pd_entry_pre_mmu(1018, page_table_bar);
//    set_pd_entry_pre_mmu(768, page_table_low);
//    asm volatile ("mov %%cr3, %%eax\n\t"
//                "mov %%eax, %%cr3" : :);
//
//    notify_screen_mmu_on();
//
//    uint32_t floc = (uint32_t) finish_setup;
//    asm volatile ("jmp %0" :: "r" (floc));
//}
//
//void finish_setup() {
//    // we're in the higher kernel now...
//    // but getting a fault? why?
//    // strange.
//
//    // moving to %eax as intermediate causes the 
//    // label to not be "corrected", and to 
//    // still be in the higher part as the linker assigned it
//    // we need to jump past the above point. 
//    // getting the "current" eip won't work. 
//
//    // unset pd entry for initial kernel load.
//    // we don't need it anymore.
//
//    // who is trying to use this page..?
//    // stack? what? ah...yeah I think the stack.
//    // let's shift all our values up by 0xc?
//    // hopefully it maps properly.
//    //
//
//    // need to make sure this will be an OK area!
//    // right now it is...but it could definitely 
//    // overwrite video memory.
//    // should move somewhere else.
//
//    asm volatile ("add %0, %%ebp" :: "r" (KERNEL_OFFSET));
//    asm volatile ("add %0, %%esp" :: "r" (KERNEL_OFFSET));
//    page_directory[4] = 0;
//
//    // how to copy the stack to a different memory area?
//
//    // reload page directory.
//    asm volatile ("mov %%cr3, %%eax\n\t"
//                "mov %%eax, %%cr3" : :);
//
//    clear_screen();
//
//    // jump to kmain, from which we should never return...
//    // (I think we need to do this stuff in the bootloader.
//    // this is kind of messed up).
//    // (it's impossible to get our stack in an OK state)
//}
//
//
