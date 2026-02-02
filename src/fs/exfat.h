#ifndef EXFAT_H
#define EXFAT_H

#include "../core/common.h"
#include "blockdev.h"

// ExFAT Boot Sector (Main Boot Region)
typedef struct {
    uint8_t  jump_boot[3];       // 0xEB 0x76 0x90
    uint8_t  fs_name[8];         // "EXFAT   "
    uint8_t  zero[53];
    uint64_t partition_offset;
    uint64_t vol_length;
    uint32_t fat_offset;
    uint32_t fat_length;
    uint32_t cluster_heap_offset;
    uint32_t cluster_count;
    uint32_t root_dir_cluster;
    uint32_t vol_serial;
    uint16_t fs_revision;
    uint16_t vol_flags;
    uint8_t  bytes_per_sec_shift;
    uint8_t  sec_per_cluster_shift;
    uint8_t  num_fats;
    uint8_t  drive_select;
    uint8_t  percent_in_use;
    uint8_t  reserved[7];
    uint8_t  boot_code[390];
    uint16_t signature;          // 0xAA55
} __attribute__((packed)) exfat_boot_sector_t;

// Generic Directory Entry (32 bytes)
typedef struct {
    uint8_t entry_type;
    uint8_t custom_defined[31];
} __attribute__((packed)) exfat_entry_t;

// Entry Types
#define EXFAT_ENTRY_BITMAP      0x81
#define EXFAT_ENTRY_UPCASE      0x82
#define EXFAT_ENTRY_LABEL       0x83
#define EXFAT_ENTRY_FILE        0x85
#define EXFAT_ENTRY_FILE_INFO   0xC0
#define EXFAT_ENTRY_FILE_NAME   0xC1

typedef struct {
    block_device_t* dev;
    uint64_t partition_start_lba;
    uint32_t bytes_per_sec;
    uint32_t sec_per_cluster;
    uint32_t root_dir_cluster;
    uint32_t fat_offset;
    uint32_t cluster_heap_offset;
} exfat_volume_t;

int exfat_init_volume(block_device_t* dev, uint64_t partition_lba);

#endif
