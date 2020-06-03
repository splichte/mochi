// hmmm...we need a "page directory" and multiple "page tables"

/* this is KERNEL_ENTRY + SECTORS_TO_READ * 512, 
 * from boot/bootloader.c. 
 * which is 0x10000 + 524288
 * which is 0x10000 + 0x80000
 */
#define KERNEL_END  0x90000

/* pages are all sized */
//typedef struct _page_info {
//    uint32_t start_addr;
//    uint8_t flags;
//} page_info;

//page_info *page_alloc();
//void page_free(page_info *p);
void setup_memory();
void print_mem_blocks();
