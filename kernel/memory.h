#include <stdint.h>

#define PAGE_SIZE   (4 * 1024)      // 4 KB

/* page flags */
#define PAGE_FULL       (1 << 0)
#define PAGE_IN_USE     (1 << 1)

// a physical page in memory. 4KB. 
typedef struct {
    uint8_t flags;  // lock, dirty, etc.
    uint64_t loc;  // location in physical memory
    uint16_t use_count; // processes using this page
    uint16_t free_mem_start;
} page;

void setup_memory();
void print_mem_blocks();

// we need a list of pages
// but we don't know how big it's going to be...
// allocate continuous memory region
page *page_alloc();
void page_free(page *p);

int kmalloc();
int kfree();

