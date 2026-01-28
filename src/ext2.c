#include "ext2.h"
#include "ramdisk.h"
#include "printf.h"

/* Minimal, read-only ext2 parser skeleton backed by ramdisk.
   This step just mounts and dumps basic info; directory walking
   will follow once a real image is wired in. */

static ext2_super_block_t super;
static uint32_t block_size = 0;

static int read_block(uint32_t block, void* buf) {
    if (block_size == 0) return -1;
    return ramdisk_read(block * block_size, buf, block_size);
}

int ext2_mount_from_ramdisk(void) {
    if (ramdisk_size() == 0) {
        kprintf("ext2: no ramdisk present, cannot mount\n");
        return -1;
    }

    /* Superblock is at offset 1024 bytes from start. */
    if (ramdisk_read(1024, &super, sizeof(super)) != 0) {
        kprintf("ext2: failed to read superblock\n");
        return -1;
    }

    if (super.s_magic != 0xEF53) {
        kprintf("ext2: bad magic 0x%x (not ext2)\n", super.s_magic);
        return -1;
    }

    block_size = 1024U << super.s_log_block_size;
    kprintf("ext2: mounted ramdisk: block_size=%u, inodes=%u, blocks=%u\n",
            (unsigned)block_size,
            (unsigned)super.s_inodes_count,
            (unsigned)super.s_blocks_count);
    return 0;
}

void ext2_print_super(void) {
    if (block_size == 0) {
        kprintf("ext2: not mounted\n");
        return;
    }

    kprintf("ext2 super: inodes=%u blocks=%u free_blocks=%u free_inodes=%u\n",
            (unsigned)super.s_inodes_count,
            (unsigned)super.s_blocks_count,
            (unsigned)super.s_free_blocks_count,
            (unsigned)super.s_free_inodes_count);
}

void ext2_list_root(void) {
    if (block_size == 0) {
        kprintf("ext2: not mounted\n");
        return;
    }

    /* For simplicity, assume 128-byte inodes (classic ext2)
       and single group, and walk inode 2 (root directory). */

    ext2_group_desc_t gd;
    uint32_t gd_block;

    if (block_size == 1024) {
        gd_block = 2; /* super at block 1, GDT at block 2 */
    } else {
        gd_block = 1; /* super and GDT share first block */
    }

    if (read_block(gd_block, &gd) != 0) {
        kprintf("ext2: failed to read group desc\n");
        return;
    }

    uint32_t inode_size = 128; /* assumption; adjust for s_inode_size if needed */
    uint32_t inode_table_block = gd.bg_inode_table;

    /* inode 2 is root directory */
    uint32_t inode_index = 2;
    uint32_t inode_offset = (inode_index - 1) * inode_size;
    uint32_t inode_block = inode_table_block + (inode_offset / block_size);
    uint32_t inode_block_offset = inode_offset % block_size;

    uint8_t buf[4096]; /* enough for one block up to 4K */
    if (block_size > sizeof(buf)) {
        kprintf("ext2: block_size too large\n");
        return;
    }

    if (read_block(inode_block, buf) != 0) {
        kprintf("ext2: failed to read inode table block\n");
        return;
    }

    ext2_inode_t* root_inode = (ext2_inode_t*)(buf + inode_block_offset);
    if (!(root_inode->i_mode & 0x4000)) { /* not a directory */
        kprintf("ext2: inode 2 is not a directory\n");
        return;
    }

    uint32_t dir_block = root_inode->i_block[0];
    if (dir_block == 0) {
        kprintf("ext2: root dir has no data block\n");
        return;
    }

    if (read_block(dir_block, buf) != 0) {
        kprintf("ext2: failed to read root dir block\n");
        return;
    }

    kprintf("ext2: root directory entries:\n");

    uint32_t offset = 0;
    while (offset < block_size) {
        ext2_dir_entry_t* de = (ext2_dir_entry_t*)(buf + offset);
        if (de->inode == 0 || de->rec_len == 0) {
            break;
        }

        char name_buf[256];
        uint32_t nlen = de->name_len;
        if (nlen >= sizeof(name_buf)) nlen = sizeof(name_buf) - 1;
        for (uint32_t i = 0; i < nlen; i++) {
            name_buf[i] = de->name[i];
        }
        name_buf[nlen] = '\0';

        kprintf("  inode=%u name=%s\n", (unsigned)de->inode, name_buf);

        offset += de->rec_len;
    }
}

