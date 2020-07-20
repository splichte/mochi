#include <stdint.h>

#define PAGE_SIZE   4096

// a physical page in memory. 4KB.
// since it's 4kb-aligned, we can use the low bit as an "in_use" flag.
//
// we don't actually need to do this -- we 
// could use a bitmap for presence! 
// and just infer physical address using offsets.
//
// but I think we'll want to 
// add more stuff later, so I made it a struct.

typedef struct {
   uint32_t start; 
} physical_page;

void setup_memory();

uint8_t map_free_page();

/* memory management functions */
//void *kmalloc(uint32_t size);
//void kfree(void *p);

