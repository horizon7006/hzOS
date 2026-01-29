#include "memory.h"

extern uint32_t _end;
uintptr_t placement_addr = (uintptr_t)&_end;

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while(num--) *p++ = (unsigned char)value;
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

void* kmalloc(size_t size) {
    /* Simple bump allocation */
    uintptr_t tmp = placement_addr;
    placement_addr += size;
    return (void*)tmp;
}

void* kmalloc_z(size_t size) {
    void* ptr = kmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* kmalloc_a(size_t size) {
    /* Page align (4K) */
    if ((placement_addr & 0xFFFFF000) != placement_addr) {
        placement_addr &= 0xFFFFF000;
        placement_addr += 0x1000;
    }
    uintptr_t tmp = placement_addr;
    placement_addr += size;
    return (void*)tmp;
}
