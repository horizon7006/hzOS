#ifndef FAT32_H
#define FAT32_H

#include "../core/common.h"
#include "blockdev.h"

typedef struct {
    uint8_t  bs_jmp_boot[3];
    uint8_t  bs_oem_name[8];
    uint16_t bpb_bytes_per_sec;
    uint8_t  bpb_sec_per_clus;
    uint16_t bpb_rsvd_sec_cnt;
    uint8_t  bpb_num_fats;
    uint16_t bpb_root_ent_cnt;
    uint16_t bpb_tot_sec16;
    uint8_t  bpb_media;
    uint16_t bpb_fat_sz16;
    uint16_t bpb_sec_per_trk;
    uint16_t bpb_num_heads;
    uint32_t bpb_hidd_sec;
    uint32_t bpb_tot_sec32;
    uint32_t bpb_fat_sz32;
    uint16_t bpb_ext_flags;
    uint16_t bpb_fs_ver;
    uint32_t bpb_root_clus;
    uint16_t bpb_fs_info;
    uint16_t bpb_bk_boot_sec;
    uint8_t  bpb_reserved[12];
    uint8_t  bs_drv_num;
    uint8_t  bs_reserved1;
    uint8_t  bs_boot_sig;
    uint32_t bs_vol_id;
    uint8_t  bs_vol_lab[11];
    uint8_t  bs_fil_sys_type[8];
} __attribute__((packed)) fat32_bpb_t;

typedef struct {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

typedef struct {
    block_device_t* dev;
    uint64_t partition_start_lba;
    uint32_t bytes_per_sec;
    uint32_t sec_per_clus;
    uint32_t rsvd_sec_cnt;
    uint32_t num_fats;
    uint32_t fat_sz32;
    uint32_t root_clus;
    uint32_t data_start_lba;
} fat32_volume_t;

int fat32_init_volume(block_device_t* dev, uint64_t partition_lba);
int fat32_read_root_dir(fat32_volume_t* vol);

#endif
