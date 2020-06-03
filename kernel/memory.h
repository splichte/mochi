// hmmm...we need a "page directory" and multiple "page tables"

/* pages are all sized */
//typedef struct _page_info {
//    uint32_t start_addr;
//    uint8_t flags;
//} page_info;

//page_info *page_alloc();
//void page_free(page_info *p);
void setup_memory();
void print_mem_blocks();
