#include "exfat.h"
#include "../lib/printf.h"
#include "../lib/memory.h"
#include "../lib/string.h"

int exfat_init_volume(block_device_t* dev, uint64_t partition_lba) {
    uint8_t sector[512];
    if (dev->read(dev, partition_lba, 1, sector) != 0) {
        kprintf("exFAT: Read error on partition LBA %ld\n", partition_lba);
        return -1;
    }

    exfat_boot_sector_t* boot = (exfat_boot_sector_t*)sector;

    // Verify "EXFAT   " string at offset 3
    if (memcmp(boot->fs_name, "EXFAT   ", 8) != 0) {
        return -1; // Not exFAT
    }

    // Verify 0xAA55 signature
    if (boot->signature != 0xAA55) {
        return -1;
    }

    // Parse Headers
    uint64_t partition_offset = boot->partition_offset;
    uint32_t cluster_heap_offset = boot->cluster_heap_offset;
    uint32_t fat_offset = boot->fat_offset;
    uint32_t root_dir_cluster = boot->root_dir_cluster;
    
    // Bytes Per Sector is 2 ^ N
    uint32_t bytes_per_sec = 1 << boot->bytes_per_sec_shift;
    
    // Sectors Per Cluster is 2 ^ N
    uint32_t sec_per_cluster = 1 << boot->sec_per_cluster_shift;
    
    kprintf("exFAT: Detected volume at LBA %ld\n", partition_lba);
    kprintf("exFAT: Cluster Heap Offset: %d\n", cluster_heap_offset);
    kprintf("exFAT: Root Dir Cluster: %d\n", root_dir_cluster);
    kprintf("exFAT: Block Size: %d bytes\n", bytes_per_sec);
    
    return 0;
}
