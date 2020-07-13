#include <stdint.h>
#include "fs.h"
#include "hardware.h"
#include "../drivers/disk.h"
#include "string.h"

// TODO: dynamically calculate.
// right now, it's based on our test 24Mb size disk. 
#define N_BLOCK_GROUPS 3

#define SUPERBLOCK_OFFSET   SECTORS_PER_BLOCK

#define S_BLOCK_SIZE    (1024 << super.s_log_block_size)

// TODO: maybe...move this into the "disk" section?
// in general, how to handle disk partitions.
static uint32_t filesys_start; // used for correct linear base addressing.
static uint8_t fs_start_set = 0;

// TODO: N_BLOCK_GROUPS should be dynamic. 
static bg_desc bgdt[N_BLOCK_GROUPS];
static superblock super;

static fs_blk_sz;

superblock make_super(uint16_t block_group_nr) {
    superblock b = { 0 };

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
void write_superblock(uint32_t location, superblock b) {
    disk_write(location, (uint8_t *) &b, sizeof(b));
}

void make_bgdt(bg_desc *table) {
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
void write_bgdt(uint32_t addr, bg_desc *table) {
    // sizeof bg_desc is 32.
    if ((N_BLOCK_GROUPS * sizeof(bg_desc)) > EXT2_BLK_SIZE) {
        print("Error: cannot fit bgdt on one ext2 block. \n");
        sys_exit();
    }

    // should we pad to a full block?
    disk_write(addr, (uint8_t *) table, N_BLOCK_GROUPS * sizeof(bg_desc));
}

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
    if (len > EXT2_BLK_SIZE) return -1;

    uint32_t lba = filesys_start + block_num * SECTORS_PER_BLOCK;

    disk_write(lba, buf, len);
}

// finds 0-valued entry in bitmap (len EXT2_BLK_SIZE)
uint32_t first_free_bm(uint8_t *bitmap, uint8_t allowed_start) {
    uint32_t first_free_block;
    uint8_t first_val = allowed_start / 8;
    for (int j = first_val; j < EXT2_BLK_SIZE; j++) {
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
uint32_t first_free_block(bg_desc *d) {
    // there is a free block; read the block bitmap.
    uint8_t bitmap[EXT2_BLK_SIZE]; // EXT2_BLK_SIZE is 1024
    disk_read_blk(d->bg_block_bitmap, bitmap);

    // find the first free block in the bitmap. 
    return first_free_bm(bitmap, 0);
}

uint32_t first_free_inode(bg_desc *d) {
    uint8_t bitmap[EXT2_BLK_SIZE];

    disk_read_blk(d->bg_inode_bitmap, bitmap);

    return first_free_bm(bitmap, EXT2_FREE_INO_START);
}

// write the in-memory BGDT out to disk.
void disk_sync_bgdt() {
    // pad with 0s.
    // (probably horribly inefficient)
    uint8_t buf[S_BLOCK_SIZE];
    uint8_t *bgdt_cast = (uint8_t *) bgdt;
    uint32_t lim = N_BLOCK_GROUPS * sizeof(bg_desc);
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
    for (int i = 0; i < N_BLOCK_GROUPS; i++) {
        bg_desc d = bgdt[i];
        uint16_t n_free_blocks = d.bg_free_blocks_count;
        if (n_free_blocks == 0) continue;

        *res = first_free_block(&d);
        return 0;
    }

    // TODO: error here. disk full.
    return -1;
}

int first_free_inode_num(uint32_t *res) {
    for (int i = 0; i < N_BLOCK_GROUPS; i++) {
        bg_desc d = bgdt[i];
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
    bg_desc d = bgdt[block_grp_n];
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
    bg_desc d = bgdt[block_grp_n];
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

inode new_dir_inode(uint32_t free_block_n) {
    inode new_inode;
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

void write_inode_table(uint32_t inode_n, inode new_inode) {
    // inode numbers start from 1
    uint32_t index = inode_n - 1;

    uint16_t block_group_n = index / super.s_inodes_per_group;
    uint16_t bg_offset = index % super.s_inodes_per_group;
    uint16_t block_offset = (bg_offset * sizeof(inode)) / S_BLOCK_SIZE;
    uint16_t in_block_offset = (bg_offset * sizeof(inode)) % S_BLOCK_SIZE;

    bg_desc d = bgdt[block_group_n];

    // read the table at a particular position.
    uint32_t block_to_update = d.bg_inode_table + block_offset;

    uint8_t buf[S_BLOCK_SIZE];
    disk_read_blk(block_to_update, buf);

    inode *to_update = (inode *) (buf + in_block_offset);
    *to_update = new_inode;

    // write the updated block back!
    disk_write_blk(block_to_update, buf);
}

void update_inode_bg_desc(uint32_t free_inode_n) {
    uint16_t inode_bg_n = (free_inode_n - 1) / super.s_blocks_per_group;
    bg_desc inode_bg_desc = bgdt[inode_bg_n];
    inode_bg_desc.bg_free_inodes_count -= 1;
    inode_bg_desc.bg_used_dirs_count += 1;
}

void update_block_bg_desc(uint32_t free_block_n) {
    uint16_t block_bg_n = free_block_n / super.s_blocks_per_group;
    bg_desc block_bg_desc = bgdt[block_bg_n];
    block_bg_desc.bg_free_blocks_count -= 1;
}

// ah. it would be great if we had malloc...hint hint...
int create_root_directory() {
    // nothing is in this directory yet, so we 
    // only need one 512-byte block to store it.

    uint32_t free_block_n, free_inode_n;
    if (first_free_block_num(&free_block_n)) {
        print("No free blocks available.\n");
        return -1;
    }

    // the root directory is always at the same location
    // in the inode table.
    free_inode_n = EXT2_ROOT_INO;

    // Create a new inode, and update the inode table with it.
    inode new_inode = new_dir_inode(free_block_n);

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

uint32_t mb_to_lba(uint32_t mb) {
    // cast mb to bytes, then divide by disk sector size (512)
    return mb * 1024 * 2;
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
    superblock b;
    if (disk_read_blk(1, (uint8_t *) &b)) {
        print("Failed to read superblock.\n");
        sys_exit();
    }

    super = b;
}

void set_bgdt() {
    uint32_t bgdt_start_blkno = 2;
    uint8_t buf[EXT2_BLK_SIZE];

    // the BGDT fits in 1 disk block 
    // if we have < (1024 / 32) block groups
    // (which we do)
    disk_read_blk(bgdt_start_blkno, buf);

    // copy into bgdt
    uint8_t *bgdt_buf = (uint8_t *) bgdt;
    for (int i = 0; i < N_BLOCK_GROUPS * sizeof(bg_desc); i++) {
        bgdt_buf[i] = buf[i];
    }
}

// TODO: it's super confusing to have the things
// that make the filesystem and read from disk in the same file.
// this writes to file-global variables! sb, block groups
void set_as_fs(uint32_t fs_start_in_mb) {
    // file-global
    filesys_start = mb_to_lba(fs_start_in_mb);
    fs_start_set = 1;

    // once filesys is set, we can use disk_read_blk.
    set_superblock();

    // now super is set, and we can use that.    
    // next, we want to set the bgdt entries.
    set_bgdt();
}

inode get_inode(uint32_t inode_n) {
    // inode numbers start from 1
    uint32_t index = inode_n - 1;

    uint16_t bgdt_n = index / super.s_inodes_per_group;
    uint32_t inode_offset = index % super.s_inodes_per_group;

    uint32_t inode_table_blkn = bgdt[bgdt_n].bg_inode_table;

    // each 1024-byte block evenly fits 8 inodes (128 bytes each)
    uint16_t inodes_per_blk = EXT2_BLK_SIZE / sizeof(inode);

    uint16_t inode_blk_n = inode_table_blkn + inode_offset / inodes_per_blk;
    uint16_t inode_blk_offset = inode_offset % inodes_per_blk;

    inode blk_nodes[EXT2_BLK_SIZE / sizeof(inode)];

    disk_read_blk(inode_blk_n, (uint8_t *) blk_nodes);

    return blk_nodes[inode_blk_offset];
}

// TODO: just cache this, like we do with superblock.
inode get_root_dir_inode() {
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
    inode n = { 0 };
    n.i_mode = EXT2_S_IFREG;
    n.i_size = 4;
    n.i_blocks = 1; // ~wasteful~
    n.i_block[0] = free_block_n;

    write_inode_table(free_inode_n, n);

    set_inode_bitmap(free_inode_n);

    set_block_bitmap(free_block_n);

    // add a "directory entry" in the root directory. 
    dentry d = {
        .inode = free_inode_n,
        .rec_len = sizeof(dentry),
        .name_len = flen,
        .file_type = EXT2_FT_DIR };

    for (int i = 0; i < flen; i++) {
        d.name[i] = fn[i];
    }

    // put the dentry d in the directory file!
    inode root = get_root_dir_inode();
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
    inode root = get_root_dir_inode();
    // read from directory
    uint32_t data_block_n = root.i_block[0];

    uint8_t buf[EXT2_BLK_SIZE];
    disk_read_blk(data_block_n, (uint8_t *) buf);

    dentry *d = (dentry *) buf;

    if(!strcmp((const char *) d->name, fn)) {
        // this is the entry we want!

        // get the inode
        uint32_t inode_n = d->inode;
        inode ind = get_inode(inode_n);

        data_block_n = ind.i_block[0];
        uint8_t bufb[EXT2_BLK_SIZE];

        // should be...
        disk_read_blk(data_block_n, (uint8_t *) bufb);
    
        uint8_t fdata[ind.i_size];
        for (int i = 0; i < ind.i_size; i++) {
            fdata[i] = bufb[i];
        }
        print((const char *) fdata);
    }
}

void set_initial_used_blocks(superblock s, bg_desc *init_bgdt, uint32_t mb_start) {
    // how many blocks for inode table, in each block group?
    uint16_t inodes_per_block = EXT2_BLK_SIZE / sizeof(inode);
    uint32_t inode_table_blocks = MOCHI_EXT2_INODES_PER_GROUP / inodes_per_block;

    uint32_t bpg = MOCHI_EXT2_BLOCKS_PER_GROUP;

    // set for block group 1 + 2
    uint16_t boot_offset = 1;
    uint16_t super_bgdt_offset = 2; // superblock, bgdt
    uint16_t bitmap_offset = 2;

    // we need the superblock to be set for 
    // "set_block_bitmap" to work.
    super = s;

    // ditto for bgdt.
    bgdt[0] = init_bgdt[0];
    bgdt[1] = init_bgdt[1];
    bgdt[2] = init_bgdt[2];

    // yup...
    filesys_start = mb_to_lba(mb_start);
    fs_start_set = 1;

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


void mkfs(uint32_t mb_start, uint32_t len) {
    uint32_t write_addr = fs_start_to_lba_superblk(mb_start);

    uint32_t nblks = len * 1024 * 2; // disk blocks

    // create a superblock.
    superblock b = make_super(0);

    write_superblock(write_addr, b);

    // make + write the backup superblock
    b = make_super(1);
    write_superblock(write_addr + SECTORS_PER_BLOCK_GROUP, b);
    write_addr += SECTORS_PER_BLOCK;

    uint32_t test;
    // create block group descriptor table
    bg_desc to_write_bgdt[N_BLOCK_GROUPS];
 
    // this writes the global static thing.
    // we need to read the state from disk to properly initialize
    // otherwise you will always need to run "mkfs" before doing any
    // filesystem operation...
    make_bgdt(to_write_bgdt);

    write_bgdt(write_addr, to_write_bgdt);

    // write backup bgdt
    write_bgdt(write_addr + SECTORS_PER_BLOCK_GROUP, to_write_bgdt);

    set_initial_used_blocks(b, to_write_bgdt, mb_start);

    // start of where we assume we have these things!
    set_as_fs(mb_start);

    create_root_directory();

    create_test_file();

    // test!
    print_file("test.txt");
}


