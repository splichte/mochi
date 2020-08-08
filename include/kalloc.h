#pragma once

#include <stdint.h>

void *kmalloc(uint32_t req_size);
void kfree(void *p);
void *kcalloc(uint32_t nitems, uint32_t size);
void *krealloc(void *ptr, uint32_t size);

