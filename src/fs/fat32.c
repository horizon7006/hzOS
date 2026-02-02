#include "fat32.h"
#include "../lib/printf.h"
#include "../lib/memory.h"
#include "../lib/string.h"

// Basic FAT32 Driver

int fat32_init_volume(block_device_t* dev, uint64_t partition_lba) {
    uint8_t sector[512];
    if (dev->read(dev, partition_lba, 1, sector) != 0) {
        kprintf("FAT32: Read error on partition LBA %ld\n", partition_lba);
        return -1;
    }

    fat32_bpb_t* bpb = (fat32_bpb_t*)sector;

    // Check signature 0xAA55 at offset 510
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        // Not a bootable sector, but might still be FAT
    }

    // Heuristic: Bytes per sector usually 512
    if (bpb->bpb_bytes_per_sec != 512) {
        // kprintf("FAT32: Non-512 bytes per sector (%d) not supported yet\n", bpb->bpb_bytes_per_sec);
        return -1;
    }

    // Determine FAT type
    uint32_t total_sectors = (bpb->bpb_tot_sec16 != 0) ? bpb->bpb_tot_sec16 : bpb->bpb_tot_sec32;
    uint32_t fat_sz = (bpb->bpb_fat_sz16 != 0) ? bpb->bpb_fat_sz16 : bpb->bpb_fat_sz32;
    uint32_t root_dir_sectors = ((bpb->bpb_root_ent_cnt * 32) + (bpb->bpb_bytes_per_sec - 1)) / bpb->bpb_bytes_per_sec;
    uint32_t data_sectors = total_sectors - (bpb->bpb_rsvd_sec_cnt + (bpb->bpb_num_fats * fat_sz) + root_dir_sectors);
    uint32_t count_of_clusters = data_sectors / bpb->bpb_sec_per_clus;

    if (count_of_clusters < 65525) {
        // Not FAT32
        return -1;
    }

    kprintf("FAT32: Detected volume at LBA %ld\n", partition_lba);
    kprintf("FAT32: OEM: %.8s\n", bpb->bs_oem_name);
    kprintf("FAT32: Vol Label: %.11s\n", bpb->bs_vol_lab);
    kprintf("FAT32: Cluster Count: %d\n", count_of_clusters);

    
    return 0;
}
