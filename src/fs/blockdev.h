#ifndef BLOCKDEV_H
#define BLOCKDEV_H

#include "../core/common.h"

typedef struct block_device {
    char name[32];
    uint64_t sector_count;
    uint32_t sector_size;
    
    // Read/Write sectors
    // Returns 0 on success, non-zero on error
    int (*read)(struct block_device* dev, uint64_t lba, uint32_t count, void* buffer);
    int (*write)(struct block_device* dev, uint64_t lba, uint32_t count, const void* buffer);
    
    void* private_data;
    struct block_device* next;
} block_device_t;

void blockdev_register(block_device_t* dev);
block_device_t* blockdev_get_first(void);
block_device_t* blockdev_find(const char* name);

#endif
