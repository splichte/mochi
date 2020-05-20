#include <stdint.h>

#define DISK_SECTOR_SIZE 512 // matches ATA_SECTOR_SIZE in disk.c

// disk commands
int disk_write(uint64_t lba, uint8_t *buf);
int disk_read(uint64_t lba, uint8_t *buf);

