#include <stdint.h>
#include "fs.h"
#include "hardware.h"
#include "disk.h"
#include "screen.h"
#include "string.h"

// TODO: dynamically calculate.
// right now, it's based on our test 24Mb size disk. 
#define N_BLOCK_GROUPS 3

#define MOCHI_EXT2_BLK_SIZE   1024

// this is based off the 214-block, 1kb block-size 
// that's in the sample 20kb ext2 filesystem
// with our current values, it yields 1712 inodes per group.
#define MOCHI_EXT2_INODES_PER_GROUP     ((214 * MOCHI_EXT2_BLK_SIZE) / sizeof(inode_t))

// see "Sample 20mb Partition Layout"
// subtracting off the blocks for block bitmap (1)
// inode bitmap (1), inode table (214)
#define MOCHI_EXT2_FREE_BLOCKS_PER_GROUP    7976

// correct this for what the inode table uses
#define MOCHI_EXT2_BLOCKS_PER_GROUP 8192

#define SECTORS_PER_BLOCK   (MOCHI_EXT2_BLK_SIZE / DISK_SECTOR_SIZE)

#define SECTORS_PER_BLOCK_GROUP (SECTORS_PER_BLOCK * MOCHI_EXT2_BLOCKS_PER_GROUP)


superblock_t make_super(uint16_t block_group_nr) {
    superblock_t b = { 0 };

    // TODO: dynamically calculate.
    uint8_t n_block_groups = N_BLOCK_GROUPS;

    b.s_inodes_per_group = MOCHI_EXT2_INODES_PER_GROUP;
    b.s_blocks_per_group = MOCHI_EXT2_BLOCKS_PER_GROUP;
    b.s_frags_per_group = MOCHI_EXT2_BLOCKS_PER_GROUP;

    b.s_inodes_count = b.s_inodes_per_group * n_block_groups;
    b.s_blocks_count = b.s_blocks_per_group * n_block_groups;

    // all blocks are free after init.
    // -4 for the superblock and block group descriptor, and their backups.
    // each is 1 block in size.
    b.s_free_blocks_count = MOCHI_EXT2_FREE_BLOCKS_PER_GROUP * n_block_groups - 4;

    // all inodes are free after init.
    b.s_free_inodes_count = b.s_inodes_count;

    b.s_first_data_block = 1;
    b.s_log_block_size = 0;
    // we don't do anything with fragments. 
    // just set it equal to block size.
    b.s_log_frag_size = 1;

    // We ignore all times and counts for now. 
    // we don't have clock set up yet, and they aren't really needed.

    b.s_magic = EXT2_SUPER_MAGIC;
    b.s_state = EXT2_ERROR_FS;
    b.s_errors = EXT2_ERRORS_RO;
    b.s_creator_os = EXT2_OS_MOCHI;
    b.s_checkinterval = (1 << 24); // something big
    b.s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
    b.s_inode_size = EXT2_GOOD_OLD_INODE_SIZE; 
    b.s_block_group_nr = block_group_nr;

    return b;
}

/* write superblock using default Mochi values. */
void write_superblock(uint32_t location, superblock_t b) {
    disk_write(location, (uint8_t *) &b, sizeof(b));
}

void make_bgdt(bgdesc_t *table) {
    for (int i = 0; i < N_BLOCK_GROUPS; i++) {
        // first 2 block groups have 2 initial blocks before the 
        // bitmaps: superblock and block group descriptor.
        if (i < 2) {
            uint8_t extra_blocks = 2;
            table[i].bg_block_bitmap = 1 + extra_blocks + i * MOCHI_EXT2_BLOCKS_PER_GROUP;
            table[i].bg_inode_bitmap = 2 + extra_blocks + i * MOCHI_EXT2_BLOCKS_PER_GROUP;
            table[i].bg_inode_table = 3 + extra_blocks + i * MOCHI_EXT2_BLOCKS_PER_GROUP;

            table[i].bg_free_blocks_count = MOCHI_EXT2_FREE_BLOCKS_PER_GROUP - extra_blocks;
        } else {
            table[i].bg_block_bitmap = 1 + i * MOCHI_EXT2_BLOCKS_PER_GROUP;
            table[i].bg_inode_bitmap = 2 + i * MOCHI_EXT2_BLOCKS_PER_GROUP;
            table[i].bg_inode_table = 3 + i * MOCHI_EXT2_BLOCKS_PER_GROUP;
            table[i].bg_free_blocks_count = MOCHI_EXT2_FREE_BLOCKS_PER_GROUP;
        }

        table[i].bg_used_dirs_count = 0;
        table[i].bg_free_inodes_count = MOCHI_EXT2_INODES_PER_GROUP;
    }
}

// write block group descriptor table
void write_bgdt(uint32_t addr, bgdesc_t *table) {
    // sizeof bgdesc_t is 32.
    if ((N_BLOCK_GROUPS * sizeof(bgdesc_t)) > MOCHI_EXT2_BLK_SIZE) {
        print("Error: cannot fit bgdt on one ext2 block. \n");
        sys_exit();
    }

    // should we pad to a full block?
    disk_write(addr, (uint8_t *) table, N_BLOCK_GROUPS * sizeof(bgdesc_t));
}



void mkfs(uint32_t mb_start, uint32_t len) {
    uint32_t write_addr = fs_start_to_lba_superblk(mb_start);

    uint32_t nblks = len * 1024 * 2; // disk blocks

    // create a superblock.
    superblock_t b = make_super(0);

    write_superblock(write_addr, b);

    // make + write the backup superblock
    b = make_super(1);
    write_superblock(write_addr + SECTORS_PER_BLOCK_GROUP, b);
    write_addr += SECTORS_PER_BLOCK;

    uint32_t test;
    // create block group descriptor table
    bgdesc_t to_write_bgdt[N_BLOCK_GROUPS];
 
    // this writes the global static thing.
    // we need to read the state from disk to properly initialize
    // otherwise you will always need to run "mkfs" before doing any
    // filesystem operation...
    make_bgdt(to_write_bgdt);

    write_bgdt(write_addr, to_write_bgdt);

    // write backup bgdt
    write_bgdt(write_addr + SECTORS_PER_BLOCK_GROUP, to_write_bgdt);

    // this should be in finish_fs_init..
    finish_fs_init(mb_start);
}


