#include <stdint.h>

#define PAGE_SIZE   (4 * 1024)      // 4 KB

/* page flags */
#define PAGE_FULL       (1 << 0)
#define PAGE_IN_USE     (1 << 1)

// a physical page in memory. 4KB.
typedef struct {
    uint8_t flags;  // lock, dirty, etc.
    uint32_t loc;  // location in physical memory
    uint16_t following; // for freeing pages allocated in chunks
} page;

void setup_memory();
void print_mem_blocks();

void page_alloc(page *p);
void page_free(page *p);

void *kmalloc(uint32_t sz);
void kfree();

