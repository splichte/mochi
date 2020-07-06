#include "disk.h"
#include "screen.h"
#include "../kernel/fs.h" // temporary!
#include "../kernel/hardware.h"

// Talk to hard disk using ATA (Advanced Technology Attachment)
// Most of this is from: http://lateblt.tripod.com/atapi.htm
//

// TODO: consider "offset from I/O base" format. 
#define ATA_DATA_REGISTER               0x1f0 // read + write
#define ATA_ERR_REGISTER                0x1f1 // read
#define ATA_FEATURES_REGISTER           0x1f1 // write
#define ATA_SECTOR_COUNT_REGISTER       0x1f2 // read + write
#define ATA_LBA_LOW_REGISTER            0x1f3 // read + write
#define ATA_LBA_MID_REGISTER            0x1f4 // read + write
#define ATA_LBA_HIGH_REGISTER           0x1f5 // read + write
#define ATA_DRIVE_HEAD_REGISTER         0x1f6 // read + write
#define ATA_STATUS_REGISTER             0x1f7 // read
#define ATA_COMMAND_REGISTER            0x1f7 // write
#define ATA_ALT_STATUS_REGISTER         0x3f6 // read
#define ATA_DEVICE_CONTROL_REGISTER     0x3f6 // write

#define ATA_SECTOR_SIZE 512


// T13/1410D revision 3b
#define ATA_STATUS_BUSY                     (1 << 7)
#define ATA_STATUS_READY                    (1 << 6)
#define ATA_STATUS_DEVICE_FAULT             (1 << 5)
#define ATA_STATUS_DATA_TRANSFER_REQUESTED  (1 << 3)
#define ATA_STATUS_ERR                      (1 << 0)


// ATA errors ( "port_byte_in(ATA_ERR_REGISTER" )
#define ATA_ERROR_BAD_BLOCK                 (1 << 7)
#define ATA_ERROR_UNCORRECTABLE_DATA_ERR    (1 << 6)
#define ATA_ERROR_MEDIA_CHANGED             (1 << 5)
#define ATA_ERROR_ID_MARK_NOT_FOUND         (1 << 4)
#define ATA_ERROR_MEDIA_CHANGE_REQUESTED    (1 << 3)
#define ATA_ERROR_COMMAND_ABORTED           (1 << 2)
#define ATA_ERROR_TRACK_0_NOT_FOUND         (1 << 1)
#define ATA_ERROR_ADDR_MARK_NOT_FOUND       (1 << 0)

// ATA commands
#define ATA_READ_WITH_RETRY     0x20 // osdev
#define ATA_WRITE_WITH_RETRY    0x30 // osdev. see page 303 of ATA v6 spec.
#define ATA_CACHE_FLUSH         0xe7 // toaruos

static void ata_wait_until_not_busy() {
    uint8_t status;
    do {
        status = port_byte_in(ATA_STATUS_REGISTER);
    } while (status & ATA_STATUS_BUSY);
}

static void ata_wait_until_status(uint8_t desired_status) {
    uint8_t status;
    do {
        status = port_byte_in(ATA_STATUS_REGISTER);
    } while (!(status & desired_status));
}

static void waste_cycle_time() {
    // page 337
    port_byte_in(ATA_ALT_STATUS_REGISTER);
}

void disk_write_sector(uint32_t lba, uint8_t *buf, uint16_t nchar);

void disk_write(uint32_t lba, uint8_t *buf, uint32_t nchar) {
    uint32_t chars_written = 0;
    uint8_t *c_buf = buf;
    uint32_t c_lba = lba;
    for (; chars_written < nchar; c_buf += ATA_SECTOR_SIZE, c_lba += 1) {
        uint16_t chars_to_write = ATA_SECTOR_SIZE;
        if ((nchar - chars_written) < ATA_SECTOR_SIZE) {
            chars_to_write = nchar - chars_written;
        }

        disk_write_sector(c_lba, c_buf, chars_to_write);

        chars_written += chars_to_write;
    }
}


/* NOTE: if you don't write a full sector 
 * in port_multiword_out, it will somehow leave the disk in 
 * a bad state that causes subsequent commands to silently fail.
 * (e.g. a read returning all 0s)
 */
void disk_write_sector(uint32_t lba, uint8_t *in_buf, uint16_t nchar) {
    if (nchar > ATA_SECTOR_SIZE) {
        print("Bad write\n");
        return;
    }

    uint8_t buf[ATA_SECTOR_SIZE];
    // if given buf size is less than ATA_SECTOR_SIZE, 
    // pad out to 512. 
    // TODO: inefficient
    int i = 0;
    for (; i < nchar; i++) {
        buf[i] = in_buf[i]; 
    }

    for (; i < ATA_SECTOR_SIZE; i++) {
        buf[i] = 0;
    }

    // stop interrupts
    asm volatile ("cli");

    // busy wait until disk is ready.
    ata_wait_until_status(ATA_STATUS_READY);

    // https://wiki.osdev.org/ATA_read/write_sectors
    port_byte_out(ATA_DRIVE_HEAD_REGISTER, 0xe0); // LBA mode

    // send # of sectors to write (1, in this case)
    port_byte_out(ATA_SECTOR_COUNT_REGISTER, 0x01);

    // byte 1 (bit 0-7) of LBA
    port_byte_out(ATA_LBA_LOW_REGISTER, (uint8_t) lba & 0xff);

    // byte 2 (bit 8-15) of LBA
    port_byte_out(ATA_LBA_MID_REGISTER, (uint8_t) (lba >> 8) & 0xff);

    // byte 3 (bit 16-23) of LBA
    port_byte_out(ATA_LBA_HIGH_REGISTER, (uint8_t) (lba >> 16) & 0xff);

    // command: write with retry
    port_byte_out(ATA_COMMAND_REGISTER, ATA_WRITE_WITH_RETRY);

    // read alternate status register and ignore result
    waste_cycle_time();

    ata_wait_until_status(ATA_STATUS_DATA_TRANSFER_REQUESTED);

    port_multiword_out(ATA_DATA_REGISTER, buf, ATA_SECTOR_SIZE / 2);

    // see osdev "cache flush"
    port_byte_out(ATA_COMMAND_REGISTER, ATA_CACHE_FLUSH);

    ata_wait_until_not_busy();

    // check if an error was set:
    uint8_t status = port_byte_in(ATA_STATUS_REGISTER);
    if (status & ATA_STATUS_ERR) {
        // uh oh!
        print("Error writing disk...");
    }

    // re-enable interrupts
    asm volatile ("sti");
}

void disk_read_internal(uint32_t lba, uint8_t *buf, uint8_t nsectors) {
    // busy wait until disk is ready.
    ata_wait_until_status(ATA_STATUS_READY);
    ata_wait_until_not_busy();

    // https://wiki.osdev.org/ATA_read/write_sectors
    // we want bits 5 and 7 set. 1010 0000. 
    uint8_t lba_highest = ((lba >> 24) & 0xff);
    // https://wiki.osdev.org/ATA_PIO_Mode 
    // Drive / Head registers
    port_byte_out(ATA_DRIVE_HEAD_REGISTER, 0xe0 | lba_highest);

    // send # of sectors to read
    port_byte_out(ATA_SECTOR_COUNT_REGISTER, nsectors);

    // byte 1 (bit 0-7) of LBA
    port_byte_out(ATA_LBA_LOW_REGISTER, (uint8_t) (lba & 0xff));

    // byte 2 (bit 8-15) of LBA
    port_byte_out(ATA_LBA_MID_REGISTER, (uint8_t) ((lba >> 8) & 0xff));

    // byte 3 (bit 16-23) of LBA
    port_byte_out(ATA_LBA_HIGH_REGISTER, (uint8_t) ((lba >> 16) & 0xff));

    // command: read with retry
    port_byte_out(ATA_COMMAND_REGISTER, ATA_READ_WITH_RETRY);

    // wait until not busy
    ata_wait_until_not_busy();

    for (int i = 0; i < nsectors; i++) {
        ata_wait_until_status(ATA_STATUS_DATA_TRANSFER_REQUESTED);
        port_multiword_in(ATA_DATA_REGISTER, (uint8_t *)(buf + i*ATA_SECTOR_SIZE), ATA_SECTOR_SIZE / 2);
    }
    ata_wait_until_not_busy();

    // check if an error was set:
    uint8_t status = port_byte_in(ATA_STATUS_REGISTER);
    if (status & ATA_STATUS_ERR) {
        // uh oh!
        print("Error reading disk... (2)");
    }

}

/* disk read function for use before interrupts are ready.
 * (e.g. in bootloader). */

/* TODO: this doesn't need to be bootloader-specific. 
 * reading big chunks from disk is generally useful...*/
void disk_read_bootloader(uint32_t lba, uint8_t *buf, uint8_t chunk) {
    disk_read_internal(lba, buf, chunk);
}

/* WARNING: disk_read assumes buf has enough space for the read! */
void disk_read(uint32_t lba, uint8_t *buf) {
    // stop interrupts
    asm volatile ("cli");

    disk_read_internal(lba, buf, 0x01);

    // re-enable interrupts
    asm volatile ("sti");
}


