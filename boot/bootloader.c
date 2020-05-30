#include <stdint.h>
#include "../drivers/disk.h"
#include "../kernel/hardware.h"
#include "../drivers/screen.h"

/* each sector is 512 bytes. we'll assume the 
 * OS fits in 8Mb. and copy over that many sectors. 
 * Then jump to 0x10000
 */

#define OS_LIMIT_IN_MB  8
#define SECTORS_TO_READ (2 * OS_LIMIT_IN_MB * 1000)

#define KERNEL_START_SECTOR 8
// the bootloader loads at 0x1000. 
// this gives it maybe 32kb of space 
// before hitting this entry point.
#define KERNEL_ENTRY        0x10000

int main() {
    clear_screen(); 
    print("Running bootloader...\n");

    uint64_t i = KERNEL_START_SECTOR; /* start at sector 8 -- aka 4K */

    uint8_t *buf;
    // "sectors to read" can be at max 2^8 - 1 = 255
    uint8_t chunk_size = 255;

    for (; i < SECTORS_TO_READ; i += chunk_size) {
        buf  = (uint8_t *) KERNEL_ENTRY + i * DISK_SECTOR_SIZE;
        disk_read_bootloader(i, buf, chunk_size);
    }
    if (i != SECTORS_TO_READ) {
        uint8_t diff = (SECTORS_TO_READ - i);
        buf = (uint8_t *) KERNEL_ENTRY + i * DISK_SECTOR_SIZE;
        disk_read_bootloader(i, buf, chunk_size);
    } 
     
    // jump to start of kernel and execute from there. 
    uint32_t start_addr = KERNEL_ENTRY;

    print("about to jump.\n");

    asm volatile ("jmp %0" : : "r" (start_addr));

    return 0;
}
