#pragma once

#include "memory.h"
#include "hardware.h"
#include "devices.h"
#include "../drivers/screen.h"

__attribute__ ((interrupt))
void page_fault_handler(struct interrupt_frame *frame, uint32_t error_code) {
    asm volatile ("cli");
    uint32_t mem_addr;
    asm volatile ("mov %%cr2, %%eax\n\t"
            "mov %%eax, %0" : "=r" (mem_addr) :); 

    map_free_page(mem_addr);

    // TODO: if all pages used, evict some page

    asm volatile ("sti");

}

typedef struct {
    uint32_t ptr_addr;
    uint32_t sz;
    uint8_t is_active; // could use a bitfield, but premature
} allocation_record;

/* we have a chunk of memory available...starting at. */
void *kmalloc(uint32_t size) {
    // find a free chunk of memory. we have a big block of heap memory.
    
}

void kfree(void *p) {

}
