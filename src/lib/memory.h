#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void memory_init(uintptr_t base, size_t total_kb);
size_t memory_get_total_kb(void);
size_t memory_get_used_kb(void);

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);

/* Memory allocation */
void* kmalloc(size_t size);
void* kmalloc_z(size_t size); /* Allocate and zero */
void* kmalloc_a(size_t size); /* Allocate page-aligned */
void* kmalloc_raw_aligned(size_t size); 
void  kfree(void* ptr);

#endif
