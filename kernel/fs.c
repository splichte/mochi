/* this file should have no knowledge of mochi's ext2 constants. */
/* it should derive all its information from the superblock. */

#include <stdint.h>
#include "fs.h"
#include "hardware.h"
#include "../drivers/disk.h"
#include "string.h"

#define S_BLOCK_SIZE    (1024 << super.s_log_block_size)

#define SECTORS_PER_BLOCK   (S_BLOCK_SIZE / DISK_SECTOR_SIZE)

#define SUPERBLOCK_OFFSET   SECTORS_PER_BLOCK


// TODO: maybe...move this into the "disk" section?
// in general, how to handle disk partitions.
static uint32_t filesys_start; // used for correct linear base addressing.
static uint8_t fs_start_set = 0;
static uint16_t n_block_groups;

// TODO: initialize this with a calloc
static bgdesc_t bgdt[1024];

static superblock_t super;

static fs_blk_sz;

int disk_read_blk(uint32_t block_num, uint8_t *buf) {
    if (!fs_start_set) return 1;

    uint32_t lba = filesys_start + block_num * SECTORS_PER_BLOCK;

    for (int i = 0; i < SECTORS_PER_BLOCK; i++) {
        disk_read(lba + i, buf + DISK_SECTOR_SIZE * i);
    }
    return 0;
}

int disk_write_blk(uint32_t block_num, uint8_t *buf) {
    if (!fs_start_set) return -1;

    uint32_t lba = filesys_start + block_num * SECTORS_PER_BLOCK;
    for (int i = 0; i < SECTORS_PER_BLOCK; i++) {
        disk_write(lba + i, buf + DISK_SECTOR_SIZE * i, DISK_SECTOR_SIZE);
    }
}

// TODO: replace disk_write_blk with disk_write_bn
int disk_write_bn(uint32_t block_num, uint8_t *buf, uint16_t len) {
    if (!fs_start_set) return -1;
    if (len > S_BLOCK_SIZE) return -1;

    uint32_t lba = filesys_start + block_num * SECTORS_PER_BLOCK;

    disk_write(lba, buf, len);
}

// finds 0-valued entry in bitmap (len S_BLOCK_SIZE)
uint32_t first_free_bm(uint8_t *bitmap, uint8_t allowed_start) {
    uint32_t first_free_block;
    uint8_t first_val = allowed_start / 8;
    for (int j = first_val; j < S_BLOCK_SIZE; j++) {
        uint8_t bmj = bitmap[j];
        if (bmj == 255) continue; // everything filled...
        // something is not filled! figure out which bit is is.
        // TODO: remember that this is little-endian. might be doing this 
        // backwards.
        uint8_t match;
        for (int k = 0; k < 8; k++) {
            if (((bmj >> k) & 1) == 0) {
               match = k; 
               break; 
            }
        }
        uint16_t first_free_entry = j * 8 + match;
        if (first_free_entry < allowed_start) continue;
        return first_free_entry;
    }
}

// assumes there is a free block!
// check that before calling this function.
uint32_t first_free_block(bgdesc_t *d) {
    // there is a free block; read the block bitmap.
    uint8_t bitmap[S_BLOCK_SIZE]; // S_BLOCK_SIZE is 1024
    disk_read_blk(d->bg_block_bitmap, bitmap);

    // find the first free block in the bitmap. 
    return first_free_bm(bitmap, 0);
}

uint32_t first_free_inode(bgdesc_t *d) {
    uint8_t bitmap[S_BLOCK_SIZE];

    disk_read_blk(d->bg_inode_bitmap, bitmap);

    return first_free_bm(bitmap, EXT2_FREE_INO_START);
}

// write the in-memory BGDT out to disk.
void disk_sync_bgdt() {
    // pad with 0s.
    // (probably horribly inefficient)
    uint8_t buf[S_BLOCK_SIZE];
    uint8_t *bgdt_cast = (uint8_t *) bgdt;
    uint32_t lim = n_block_groups * sizeof(bgdesc_t);
    for (int i = 0; i < lim; i++) {
        *(buf + i) = *(bgdt_cast + i);
    }
    for (int i = lim; i < S_BLOCK_SIZE; i++) {
        *buf = 0;
    }

    uint8_t bgdt_blockn = 2;
    // write out to disk.
    disk_write_blk(bgdt_blockn, buf);

    // also write the backup block.
    disk_write_blk(bgdt_blockn + super.s_blocks_per_group, buf);
}

// write the in-memory superblock out to disk.
void disk_sync_super() {
    // superblock is 1024 bytes long, so we don't need to pad.

    uint8_t super_blockn = 1; 
    disk_write_blk(super_blockn, (uint8_t *) &super);
    // write backup
    disk_write_blk(super_blockn + super.s_blocks_per_group, (uint8_t *) &super);
}

// TODO: should be given a superblock or some argument
int first_free_block_num(uint32_t *res) {
    for (int i = 0; i < n_block_groups; i++) {
        bgdesc_t d = bgdt[i];
        uint16_t n_free_blocks = d.bg_free_blocks_count;
        if (n_free_blocks == 0) continue;

        *res = first_free_block(&d);
        return 0;
    }

    // TODO: error here. disk full.
    return -1;
}

int first_free_inode_num(uint32_t *res) {
    for (int i = 0; i < n_block_groups; i++) {
        bgdesc_t d = bgdt[i];
        uint16_t n_free_inodes = d.bg_free_inodes_count;
        if (n_free_inodes == 0) continue;

        *res = first_free_inode(&d);
        return 0;
    }

    return -1;
}

void set_bit(uint8_t *bitmap, uint16_t i) {
    uint16_t byte = i / 8;
    uint8_t bit = i % 8;

    bitmap[byte] |= (1 << bit);
}

void unset_bit(uint8_t *bitmap, uint16_t i) {
    uint16_t byte = i / 8;
    uint8_t bit = i % 8;

    bitmap[byte] ^= (1 << bit);
}

void mark_block(uint32_t block_num, uint8_t val) {
    // figure out which block group descriptor we need
    // from the block
    uint16_t block_grp_n = block_num / super.s_blocks_per_group;

    // using appropriate block descriptor, access the block bitmap
    bgdesc_t d = bgdt[block_grp_n];
    uint8_t bitmap[S_BLOCK_SIZE];
    disk_read_blk(d.bg_block_bitmap, bitmap);

    uint16_t bit_to_set = block_num % super.s_inodes_per_group;

    if (val == 1) {
        set_bit(bitmap, bit_to_set);
    } else {
        unset_bit(bitmap, bit_to_set);
    }

    // write back out to disk
    disk_write_blk(d.bg_block_bitmap, bitmap);
}

void set_block_bitmap(uint32_t block_num) {
    mark_block(block_num, 1);
}

void unset_block_bitmap(uint32_t block_num) {
    mark_block(block_num, 0);
}

void mark_inode(uint32_t inode_num, uint8_t val) {
    // figure out which block group descriptor we need
    // from the inode number
    uint16_t block_grp_n = (inode_num - 1) / super.s_inodes_per_group;

    // using appropriate block descriptor, access the inode bitmap
    bgdesc_t d = bgdt[block_grp_n];
    uint8_t bitmap[S_BLOCK_SIZE];
    disk_read_blk(d.bg_inode_bitmap, bitmap);

    uint16_t bit_to_set = (inode_num - 1) % super.s_inodes_per_group;

    if (val == 1) {
        set_bit(bitmap, bit_to_set);
    } else {
        unset_bit(bitmap, bit_to_set);
    }

    // write back out to disk
    disk_write_blk(d.bg_inode_bitmap, bitmap);
}

void set_inode_bitmap(uint32_t inode_num) {
    mark_inode(inode_num, 1);
}

void unset_inode_bitmap(uint32_t inode_num) {
    mark_inode(inode_num, 0);
}

void write_inode_table(uint32_t inode_n, inode_t new_inode) {
    // inode numbers start from 1
    uint32_t index = inode_n - 1;

    uint16_t block_group_n = index / super.s_inodes_per_group;
    uint16_t bg_offset = index % super.s_inodes_per_group;
    uint16_t block_offset = (bg_offset * sizeof(inode_t)) / S_BLOCK_SIZE;
    uint16_t in_block_offset = (bg_offset * sizeof(inode_t)) % S_BLOCK_SIZE;

    bgdesc_t d = bgdt[block_group_n];

    // read the table at a particular position.
    uint32_t block_to_update = d.bg_inode_table + block_offset;

    uint8_t buf[S_BLOCK_SIZE];
    disk_read_blk(block_to_update, buf);

    inode_t *to_update = (inode_t *) (buf + in_block_offset);
    *to_update = new_inode;

    // write the updated block back!
    disk_write_blk(block_to_update, buf);
}

void update_inode_bg_desc(uint32_t free_inode_n) {
    uint16_t inode_bg_n = (free_inode_n - 1) / super.s_blocks_per_group;
    bgdesc_t inode_bg_desc = bgdt[inode_bg_n];
    inode_bg_desc.bg_free_inodes_count -= 1;
    inode_bg_desc.bg_used_dirs_count += 1;
}

void update_block_bg_desc(uint32_t free_block_n) {
    uint16_t block_bg_n = free_block_n / super.s_blocks_per_group;
    bgdesc_t block_bg_desc = bgdt[block_bg_n];
    block_bg_desc.bg_free_blocks_count -= 1;
}

inode_t new_dir_inode(uint32_t free_block_n) {
    inode_t new_inode;
    new_inode.i_mode = EXT2_S_IFDIR;
    new_inode.i_links_count = 1; // 1 or 0?
    new_inode.i_blocks = 2; 
    new_inode.i_links_count = 1;
    new_inode.i_block[0] = free_block_n;
    new_inode.i_file_acl = 0;
    new_inode.i_dir_acl = 0;
    new_inode.i_faddr = 0;
    return new_inode;
}


// used for getting the filesystem superblock location
// from a given filesystem start point (in Mb)
uint32_t fs_start_to_lba_superblk(uint32_t mb_start) {
    // adding a block's worth of sectors (SECTORS_PER_BLOCK)
    // to the initial write address, since the first block 
    // is reserved for boot records on ext2 (which we don't use).
    //
    // the write_addr is lba, so it's measured in disk sectors
    // (512 byte units)
    return mb_to_lba(mb_start) + SUPERBLOCK_OFFSET;
}


void set_superblock() {
    // set superblock 
    superblock_t b;
    if (disk_read_blk(1, (uint8_t *) &b)) {
        print("Failed to read superblock.\n");
        sys_exit();
    }

    super = b;
}

void set_bgdt() {
    uint32_t bc = super.s_blocks_count;
    uint32_t bpg = super.s_blocks_per_group;
    
    n_block_groups = bc / bpg;
    if (bc % bpg != 0) n_block_groups++;


    uint32_t bgdt_start_blkno = 2;
    uint8_t buf[S_BLOCK_SIZE];

    // the BGDT fits in 1 disk block 
    // if we have < (1024 / 32) block groups
    // (which we do)
    disk_read_blk(bgdt_start_blkno, buf);

    // TODO: fill this out
//    bgdt = (uint8_t *) kcalloc(n_block_groups, sizeof(bgdesc_t));

    // copy into bgdt
    uint8_t *bgdt_buf = (uint8_t *) bgdt;
    for (int i = 0; i < n_block_groups * sizeof(bgdesc_t); i++) {
        bgdt_buf[i] = buf[i];
    }
}

void read_fs(uint32_t fs_start_in_mb) {
    // file-global
    filesys_start = mb_to_lba(fs_start_in_mb);
    fs_start_set = 1;

    // once filesys is set, we can use disk_read_blk.
    set_superblock();

    // now super is set, and we can use that.    
    // next, we want to set the bgdt entries.
    set_bgdt();
}

inode_t get_inode(uint32_t inode_n) {
    // inode numbers start from 1
    uint32_t index = inode_n - 1;

    uint16_t bgdt_n = index / super.s_inodes_per_group;
    uint32_t inode_offset = index % super.s_inodes_per_group;

    uint32_t inode_table_blkn = bgdt[bgdt_n].bg_inode_table;

    // each 1024-byte block evenly fits 8 inodes (128 bytes each)
    uint16_t inodes_per_blk = S_BLOCK_SIZE / sizeof(inode_t);

    uint16_t inode_blk_n = inode_table_blkn + inode_offset / inodes_per_blk;
    uint16_t inode_blk_offset = inode_offset % inodes_per_blk;

    inode_t blk_nodes[S_BLOCK_SIZE / sizeof(inode_t)];

    disk_read_blk(inode_blk_n, (uint8_t *) blk_nodes);

    return blk_nodes[inode_blk_offset];
}

// TODO: just cache this, like we do with superblock.
inode_t get_root_dir_inode() {
    return get_inode(EXT2_ROOT_INO);
}

// "/test.txt"
int create_test_file() {
    char *fn = "test.txt";
    uint8_t flen = 9; 
    uint8_t dlen = 4;

    char data[dlen];
    data[0] = 'H';
    data[1] = 'i';
    data[2] = '\n';
    data[3] = '\0';

    // reserve a free block and free inode
    uint32_t free_block_n, free_inode_n;
    if (first_free_block_num(&free_block_n)) {
        print("No free blocks available.\n");
        return -1;
    }

    if (first_free_inode_num(&free_inode_n)) {
        print("No free inodes available.\n");
        return -1;
    }

    // create file inode, etc.
    inode_t n = { 0 };
    n.i_mode = EXT2_S_IFREG;
    n.i_size = 4;
    n.i_blocks = 1; // ~wasteful~
    n.i_block[0] = free_block_n;

    write_inode_table(free_inode_n, n);

    set_inode_bitmap(free_inode_n);

    set_block_bitmap(free_block_n);

    // add a "directory entry" in the root directory. 
    dentry_t d = {
        .inode = free_inode_n,
        .rec_len = sizeof(dentry_t),
        .name_len = flen,
        .file_type = EXT2_FT_DIR };

    for (int i = 0; i < flen; i++) {
        d.name[i] = fn[i];
    }

    // put the dentry d in the directory file!
    inode_t root = get_root_dir_inode();
    // for our test file, we can write to the first block~

    // write to the directory file.
    uint32_t data_block_n = root.i_block[0];

    // TODO: fix disk_write_blk
    // so we don't have to read the whole dumb thing
    disk_write_bn(data_block_n, (uint8_t *) &d, sizeof(d));

    // write the data (finally) to the actual file!
    disk_write_bn(free_block_n, (uint8_t *) data, dlen);

    // add to inode table in free place
    update_inode_bg_desc(free_inode_n);
    update_block_bg_desc(free_block_n);
    disk_sync_bgdt();

    // update the block group descriptor that the block belongs to.
    // update the superblock. 
    super.s_free_blocks_count -= 1;
    super.s_free_inodes_count -= 1;
    disk_sync_super();
}

// print file in root directory. 
// this is just a test.
void print_file(const char *fn) {
    inode_t root = get_root_dir_inode();
    // read from directory
    uint32_t data_block_n = root.i_block[0];

    uint8_t buf[S_BLOCK_SIZE];
    disk_read_blk(data_block_n, (uint8_t *) buf);

    dentry_t *d = (dentry_t *) buf;

    if(!strcmp((const char *) d->name, fn)) {
        // this is the entry we want!

        // get the inode
        uint32_t inode_n = d->inode;
        inode_t ind = get_inode(inode_n);

        data_block_n = ind.i_block[0];
        uint8_t bufb[S_BLOCK_SIZE];

        // should be...
        disk_read_blk(data_block_n, (uint8_t *) bufb);
    
        uint8_t fdata[ind.i_size];
        for (int i = 0; i < ind.i_size; i++) {
            fdata[i] = bufb[i];
        }
        print((const char *) fdata);
    }
}

int create_root_directory() {
    uint32_t free_block_n, free_inode_n;
    if (first_free_block_num(&free_block_n)) {
        print("No free blocks available.\n");
        return -1;
    }

    // the root directory is always at the same location
    // in the inode table.
    free_inode_n = EXT2_ROOT_INO;

    // Create a new inode, and update the inode table with it.
    inode_t new_inode = new_dir_inode(free_block_n);

    // write to inode table
    write_inode_table(free_inode_n, new_inode);

    // set inode bitmap
    set_inode_bitmap(free_inode_n);

    // set block bitmap
    set_block_bitmap(free_block_n);

    // TODO: an optimization is to not actually sync
    // the superblock/bgdt to disk so often. that saves time.

    // update the block group descriptor that the inode belongs to.
    update_inode_bg_desc(free_inode_n);
    update_block_bg_desc(free_block_n);
    disk_sync_bgdt();

    // update the block group descriptor that the block belongs to.
    // update the superblock. 
    super.s_free_blocks_count -= 1;
    super.s_free_inodes_count -= 1;
    disk_sync_super();
}


void set_initial_used_blocks() {
    

    // how many blocks for inode table, in each block group?
    uint16_t inodes_per_block = S_BLOCK_SIZE / sizeof(inode_t);
    uint32_t inode_table_blocks = super.s_inodes_per_group / inodes_per_block;

    uint32_t bpg = super.s_blocks_per_group;

    // set for block group 1 + 2
    uint16_t boot_offset = 1;
    uint16_t super_bgdt_offset = 2; // superblock, bgdt
    uint16_t bitmap_offset = 2;

    // boot
    set_block_bitmap(0);

    // superblock + backup
    set_block_bitmap(1);
    set_block_bitmap(1 + bpg);

    // bgdt + backup
    set_block_bitmap(2);
    set_block_bitmap(2 + bpg);
   
    // mark the bitmaps + inode table blocks, for each block group. 
    for (int i = 0; i < bitmap_offset + inode_table_blocks; i++) {
        // block group 1
        set_block_bitmap(boot_offset + super_bgdt_offset + i);
        // block group 2
        set_block_bitmap(boot_offset + super_bgdt_offset + bpg + i);
        // block group 3
        set_block_bitmap(boot_offset + 2*bpg + i);
    }
}



void finish_fs_init(uint32_t mb_start) {
    // read the metadata into our "local" variables.
    read_fs(mb_start);

    set_initial_used_blocks();

    create_root_directory();

}

void test_fs() {
    create_test_file();

    // test!
    print_file("test.txt");
}

