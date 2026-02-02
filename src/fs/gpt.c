#include "gpt.h"
#include "../lib/printf.h"
#include "../lib/memory.h"
#include "../lib/string.h"

// Standard Microsoft Basic Data Partition GUID
// EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
static const uint8_t PARTITION_TYPE_BASIC_DATA[16] = {
    0xA2, 0xA0, 0xD0, 0xEB,
    0xE5, 0xB9,
    0x33, 0x44,
    0x87, 0xC0,
    0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7
};

void gpt_read_table(block_device_t* dev) {
    uint8_t sector_buffer[512]; // Assuming 512-byte sectors for now
    
    // Read GPT Header (LBA 1)
    if (dev->read(dev, 1, 1, sector_buffer) != 0) {
        kprintf("gpt: Failed to read LBA 1 on device '%s'\n", dev->name);
        return;
    }

    gpt_header_t* header = (gpt_header_t*)sector_buffer;

    // Verify Signature "EFI PART" (0x5452415020494645)
    if (header->signature != 0x5452415020494645ULL) {
        kprintf("gpt: Invalid signature on device '%s' (Not GPT)\n", dev->name);
        return;
    }

    kprintf("gpt: Found valid GPT header on '%s'\n", dev->name);
    kprintf("gpt: Disk GUID: %x-%x-...\n", header->disk_guid[3], header->disk_guid[2]);

    // Read Partition Entries
    // Calculate number of sectors needed for partition table
    uint32_t table_size = header->num_partition_entries * header->partition_entry_size;
    uint32_t sectors_needed = (table_size + dev->sector_size - 1) / dev->sector_size;
    
    // Allocate buffer for partition table
    uint8_t* table_buffer = kmalloc(sectors_needed * dev->sector_size);
    if (!table_buffer) {
        kprintf("gpt: Failed to allocate memory for partition table\n");
        return;
    }

    if (dev->read(dev, header->partition_entry_lba, sectors_needed, table_buffer) != 0) {
        kprintf("gpt: Failed to read partition table\n");
        kfree(table_buffer);
        return;
    }

    // Iterate Entries
    for (uint32_t i = 0; i < header->num_partition_entries; i++) {
        gpt_entry_t* entry = (gpt_entry_t*)(table_buffer + (i * header->partition_entry_size));
        
        // Check if entry is unused (Type GUID is all zeros)
        int unused = 1;
        for (int j = 0; j < 16; j++) {
            if (entry->type_guid[j] != 0) {
                unused = 0;
                break;
            }
        }
        if (unused) continue;

        kprintf("gpt: Partition %d: Start=%ld End=%ld (Size=%ld MB)\n", 
            i, entry->start_lba, entry->end_lba, 
            ((entry->end_lba - entry->start_lba) * dev->sector_size) / (1024*1024));

        // Check if it's a Basic Data Partition (FAT32/NTFS/exFAT)
        if (memcmp(entry->type_guid, PARTITION_TYPE_BASIC_DATA, 16) == 0) {
            kprintf("gpt: Found Basic Data Partition (likely FAT32/exFAT)\n");
            
            // Try exFAT first (modern)
            extern int exfat_init_volume(block_device_t* dev, uint64_t partition_lba);
            if (exfat_init_volume(dev, entry->start_lba) != 0) {
                // Determine if FAT32
                extern int fat32_init_volume(block_device_t* dev, uint64_t partition_lba);
                fat32_init_volume(dev, entry->start_lba);
            }
        }
    }

    kfree(table_buffer);
}
