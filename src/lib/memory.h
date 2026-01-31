#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t count);

/* Memory allocation */
void* kmalloc(size_t size);
void* kmalloc_z(size_t size); /* Allocate and zero */
void* kmalloc_a(size_t size); /* Allocate page-aligned */
void* kmalloc_raw_aligned(size_t size); 
void  kfree(void* ptr);

#endif
