#include <stdint.h>
#include "../drivers/disk.h"
#include "../kernel/hardware.h"
#include "../drivers/screen.h"

// 1024 sectors gives 512 kb. that's 
// pretty good. 
// kernel.bin is 17kb right now. 
// 1024 or so is reasonable. 
// 16384 starts to take a while...
// like, longer than I think it should.
//
#define SECTORS_TO_READ 1024

#define KERNEL_START_SECTOR 16
// the bootloader loads at 0x1400. 
// this gives it maybe 32kb of space 
// before hitting this entry point.
#define KERNEL_ENTRY        0x10000

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

    // jump to start of kernel and execute from there. 
    uint32_t start_addr = KERNEL_ENTRY;

    print("about to jump.\n");

    asm volatile ("jmp %0" : : "r" (start_addr));

    return 0;
}
