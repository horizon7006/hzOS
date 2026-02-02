#ifndef GPT_H
#define GPT_H

#include "../core/common.h"
#include "blockdev.h"

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t my_lba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t partition_entry_crc32;
} __attribute__((packed)) gpt_header_t;

typedef struct {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t start_lba;
    uint64_t end_lba;
    uint64_t attributes;
    uint16_t partition_name[36]; // UTF-16LE
} __attribute__((packed)) gpt_entry_t;

// Partition structure to represent a logical volume
typedef struct partition {
    block_device_t* parent_dev;
    uint64_t start_lba;
    uint64_t sector_count;
    uint8_t  type_guid[16];
    char     name[32]; // ASCII converted name
} partition_t;

void gpt_read_table(block_device_t* dev);

#endif
