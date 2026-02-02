#include "memory.h"

extern uint32_t _end;
static uintptr_t placement_addr = (uintptr_t)&_end;

void memory_init(uintptr_t base) {
    placement_addr = base;
}

uintptr_t get_heap_base(void) {
    return placement_addr;
}

typedef struct malloc_block {
    size_t size;
    int free;
    struct malloc_block* next;
} malloc_block_t;

static malloc_block_t* free_list = NULL;

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while(num--) *p++ = (unsigned char)value;
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    const char* src8 = (const char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

static malloc_block_t* find_free_block(size_t size) {
    malloc_block_t* curr = free_list;
    while (curr) {
        if (curr->free && curr->size >= size) return curr;
        curr = curr->next;
    }
    return NULL;
}

void* kmalloc(size_t size) {
    // Round to 4 bytes
    size = (size + 3) & ~3;
    
    malloc_block_t* block = find_free_block(size);
    if (block) {
        // Can we split this block?
        if (block->size >= size + sizeof(malloc_block_t) + 4) {
             malloc_block_t* new_block = (malloc_block_t*)((uint8_t*)(block + 1) + size);
             new_block->size = block->size - size - sizeof(malloc_block_t);
             new_block->free = 1;
             new_block->next = block->next;
             
             block->size = size;
             block->next = new_block;
        }
        block->free = 0;
        return (void*)(block + 1);
    }

    // Allocate new block
    uintptr_t addr = placement_addr;
    block = (malloc_block_t*)addr;
    block->size = size;
    block->free = 0;
    block->next = free_list;
    free_list = block;

    placement_addr += sizeof(malloc_block_t) + size;
    return (void*)(block + 1);
}

void kfree(void* ptr) {
    if (!ptr) return;
    malloc_block_t* block = (malloc_block_t*)ptr - 1;
    block->free = 1;
}

void* kmalloc_z(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

void* kmalloc_a(size_t size) {
    // Minimal alignment support for now (simplified)
    if ((placement_addr & 0xFFF) != 0) {
        placement_addr = (placement_addr & ~0xFFF) + 0x1000;
    }
    return kmalloc(size);
}

void* kmalloc_raw_aligned(size_t size) {
    if ((placement_addr & 0xFFF) != 0) {
        placement_addr = (placement_addr & ~0xFFF) + 0x1000;
    }
    void* addr = (void*)placement_addr;
    placement_addr += size;
    return addr;
}
