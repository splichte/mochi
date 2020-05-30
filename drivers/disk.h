#include <stdint.h>

#define SECTOR_CHUNK        0xff
#define DISK_SECTOR_SIZE    512 // matches ATA_SECTOR_SIZE in disk.c

// disk commands
void disk_write(uint64_t lba, uint8_t *buf);
void disk_read(uint64_t lba, uint8_t *buf);
void disk_read_bootloader(uint64_t lba, uint8_t *buf, uint8_t chunk);

