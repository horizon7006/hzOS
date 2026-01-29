#include "ramdisk.h"
#include "printf.h"

static uint8_t* rd_base = 0;
static uint32_t rd_size = 0;

void ramdisk_init(uint8_t* base, uint32_t size) {
    rd_base = base;
    rd_size = size;
    kprintf("ramdisk: base=%p size=%u bytes\n", base, (unsigned)size);
}

uint32_t ramdisk_size(void) {
    return rd_size;
}

int ramdisk_read(uint32_t offset, void* buf, uint32_t len) {
    if (!rd_base || offset + len > rd_size) {
        return -1;
    }
    uint8_t* dst = (uint8_t*)buf;
    for (uint32_t i = 0; i < len; i++) {
        dst[i] = rd_base[offset + i];
    }
    return 0;
}

