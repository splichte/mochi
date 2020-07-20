#include <stddef.h>
#include <stdint.h>
#include "kalloc.h"

// matches just above where the base pointer is set in kernel_entry.asm
#define AVAIL_MEM_START 0xf0400000
#define AVAIL_MEM_END 0xffffffff

// doubly-linked makes freeing easier
typedef struct __header {
    struct __header *prev;
    struct __header *next;
    uint32_t size;
} header_t;

static header_t *base = NULL;

// insert a new node at the head of the linked list. 
// return the pointer to the user's requested memory region.
void *insert_head(uint32_t req_size) {
    header_t *old_base = base;
    base = (header_t *) AVAIL_MEM_START;
    base->size = req_size;
    base->next = old_base;
    base->prev = NULL;
    return (void *) (base + 1);
}

// insert a new node into the linked list. 
// return the pointer to the user's requested memory region.
void *insert_after(header_t *p, uint32_t req_size) {
    uint32_t curr_end = ((uint32_t) p) + p->size;
    header_t *new = (header_t *) curr_end;
    new->size = req_size;
    new->next = p->next;
    new->prev = p;
    p->next = new;
    new->next->prev = new;

    return (void *) (new + 1);
}

void *kmalloc(uint32_t req_size) {
    uint32_t right = AVAIL_MEM_END;
    uint32_t left = AVAIL_MEM_START;
    if (right - left < req_size) {
        // we can't fit this size.
        return NULL;
    }

    if (base == NULL) {
        return insert_head(req_size);
    }

    uint32_t needed_size = req_size + sizeof(header_t);
   
    // base pointer might not refer to AVAIL_MEM_START. 
    // but it will always be the first assigned region.
    // if base does refer to AVAIL_MEM_START, 
    // right - left = 0, so we'll skip this block. 
    right = (uint32_t) base;
    left = AVAIL_MEM_START;
    if (right - left > needed_size) {
        return insert_head(req_size);
    }

    // loop through the linked list
    header_t *curr = base;
    for (; curr->next != NULL; curr = curr->next) {
        // get the difference between the end of this chunk and the beginning
        // of the next
        right = (uint32_t) curr->next;
        left = ((uint32_t) curr) + curr->size;
        if (right - left >= needed_size) {
            return insert_after(curr, req_size);
        }
    }

    // curr->next is null. see if we can fit it in the available region.
    // if not, return a null pointer. 
    right = AVAIL_MEM_END;
    left = ((uint32_t) curr) + curr->size;
    if (right - left > needed_size) {
        return insert_after(curr, req_size);
    }

    return NULL;
}

void kfree(void *p) {
    header_t *hdr = ((header_t *) p) - 1;
    // update pointers of hdr's neighbors to skip over hdr
    hdr->prev->next = hdr->next;
    hdr->next->prev = hdr->prev;
}

void *kcalloc(uint32_t nitems, uint32_t size) {
    uint64_t mult = nitems * size;
    if (mult > UINT32_MAX) return NULL; // we can't fit this.

    void *ret = kmalloc((uint32_t) mult);
    if (ret == NULL) return NULL;

    for (uint32_t i = 0; i < mult; i++) {
        *((uint8_t *)ret) = 0;
    }
    return ret;
}

// NOTE: expects size to be greater than existing size. 
void *krealloc(void *ptr, uint32_t size) {
    // allocate new region
    void *new = kmalloc(size);
    if (new == NULL) return NULL;

    // copy old data to new region
    uint8_t *bnew = (uint8_t *) new;
    uint8_t *bptr = (uint8_t *) ptr;

    header_t *old_hdr = ((header_t *) ptr) - 1;
    uint32_t old_size = old_hdr->size;

    for (uint32_t i = 0; i < old_size; i++) {
        *bnew = *bptr;
    }

    // free old region
    kfree(ptr);
    return new;
}


