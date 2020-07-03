#include <stdint.h>

#define PAGE_SIZE   4096

// a physical page in memory. 4KB.
typedef struct {
    uint32_t start;
    uint8_t in_use; // is page mapped to by vmem? 
} physical_page;

void setup_memory();


