/* ext2-styled filesystem. 
 * structs based off:
 * https://www.nongnu.org/ext2-doc/ext2.html
 *
 *
 */
#include <stdint.h>
#include "../drivers/disk.h"

#define ROOT_USR    0

#define EXT2_SUPER_MAGIC    0xef53

#define EXT2_VALID_FS   1
#define EXT2_ERROR_FS   2

#define EXT2_OS_MOCHI   0xeeee

#define EXT2_ERRORS_RO  2

#define EXT2_GOOD_OLD_FIRST_INO 11
#define EXT2_GOOD_OLD_INODE_SIZE 128

/* Defined reserved inodes. */
#define EXT2_BAD_INO            1
#define EXT2_ROOT_INO           2
#define EXT2_ACL_IDX_INO        3
#define EXT2_ACL_DATA_INO       4
#define EXT2_BOOT_LOADER_INO    5
#define EXT2_UNDEL_DIR_INO      6

#define EXT2_FREE_INO_START     11
/* i_mode */

/* file format */
#define EXT2_S_IFSOCK   0xc000      // socket
#define EXT2_S_IFLNK    0xa000      // symlink
#define EXT2_S_IFREG    0x8000      // regular file
#define EXT2_S_IFBLK    0x6000      // block device
#define EXT2_S_IFDIR    0x4000      // directory
#define EXT2_S_IFCHR    0x2000      // character device
#define EXT2_S_IFIFO    0x1000      // fifo

/* process execution user/group override */
#define EXT2_S_ISUID    0x0800      // set process user id
#define EXT2_S_ISGID    0x0400      // set process group id
#define EXT2_S_ISVTX    0x0200      // sticky bit

/* access rights */
#define EXT2_S_IRUSR    0x0100      // user read
#define EXT2_S_IWUSR    0x0080      // user write
#define EXT2_S_IXUSR    0x0040      // user execute
#define EXT2_S_IRGRP    0x0020      // group read
#define EXT2_S_IWGRP    0x0010      // group write
#define EXT2_S_IXGRP    0x0008      // group execute
#define EXT2_S_IROTH    0x0004      // others read
#define EXT2_S_IWOTH    0x0002      // others write
#define EXT2_S_IXOTH    0x0001      // others execute


/* Table 4.2: Defined Inode File Type Values */
#define EXT2_FT_UNKNOWN     0
#define EXT2_FT_REG_FILE    1
#define EXT2_FT_DIR         2
#define EXT2_FT_CHRDEV      3
#define EXT2_FT_BLKDEV      4
#define EXT2_FT_FIFO        5
#define EXT2_FT_SOCK        6
#define EXT2_FT_SYMLINK     7


/* located at byte offset 1024
 * and 1024 bytes long. */
typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    /* NOTE: the storage format is little-endian
     * so these uint16_t fields will be "reversed"
     * TODO: check if these is how other 
     * implementations actually store them. */
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    /* EXT2_DYNAMIC_REV specific */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    char s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algo_bitmap;

    /* Performance hints */
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t alignment; 

    /* Journaling support */
    char s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;

    /* Directory Indexing Support */
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t padding_a;
    uint16_t padding_b;

    /* Other options */
    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;
    uint8_t reserved_future[760];
} superblock_t;

/* Block group descriptor. 
 * The block group descriptor table 
 * is an array of these.
 * */
typedef struct {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;

    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;

    uint8_t bg_reserved[12];
} bgdesc_t;

// 128 bytes
typedef struct {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
} inode_t;

// 32, 32, 64 = 128
typedef struct _dentry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[64]; // wasteful, but whatever!
} dentry_t;


// make an ext2 filesystem on the disk.
// args are in Mb
void mkfs(uint32_t offset, uint32_t len);

// once the metadata is in place, creates the root directory.
void finish_fs_init(uint32_t mb_start);


// cast mb to bytes, then divide by disk sector size (512)
#define mb_to_lba(mb) (mb * 1024 * 2)

void read_fs(uint32_t location);

void test_fs();

/* interface to the filesystem! */

typedef uint32_t fd_t;      // file descriptor type

// an open file.
typedef struct {
    inode_t *inode;      // the inode for this file.
    fd_t fd;            // the file descriptor (is what, an index into the file table?)
} file_t;

// given a "path", print the file contents
void cat(const char *path);

// how to "make" a new file"?
// I want to open a file for writing.

// rename a file
//int mkdir(const char *pathname);
//int rmdir(const char *pathname);


