#ifndef NVME_H
#define NVME_H

#include "../core/common.h"
#include "pci.h"

/* NVMe Controller Registers (Offsets from BAR0) */
#define NVME_REG_CAP      0x00  /* Controller Capabilities (8 bytes) */
#define NVME_REG_VS       0x08  /* Version (4 bytes) */
#define NVME_REG_INTMS    0x0C  /* Interrupt Mask Set (4 bytes) */
#define NVME_REG_INTMC    0x10  /* Interrupt Mask Clear (4 bytes) */
#define NVME_REG_CC       0x14  /* Controller Configuration (4 bytes) */
#define NVME_REG_CSTS     0x1C  /* Controller Status (4 bytes) */
#define NVME_REG_NSSR     0x20  /* NVM Subsystem Reset (4 bytes) */
#define NVME_REG_AQA      0x24  /* Admin Queue Attributes (4 bytes) */
#define NVME_REG_ASQ      0x28  /* Admin Submission Queue Base Address (8 bytes) */
#define NVME_REG_ACQ      0x30  /* Admin Completion Queue Base Address (8 bytes) */

/* NVMe Submission Queue Entry (64 bytes) */
typedef struct {
    uint32_t cdw0;      /* Command Identifier, Opcode, Fuse, PSR */
    uint32_t nsid;      /* Namespace Identifier */
    uint64_t reserved0;
    uint64_t mpointer;  /* Metadata Pointer */
    uint64_t prp1;      /* PRP Entry 1 */
    uint64_t prp2;      /* PRP Entry 2 */
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} nvme_sq_entry_t;

/* NVMe Completion Queue Entry (16 bytes) */
typedef struct {
    uint32_t cdw0;      /* Command Specific */
    uint32_t reserved;
    uint16_t sq_head;   /* SQ Head Pointer */
    uint16_t sq_id;     /* SQ Identifier */
    uint16_t cid;       /* Command Identifier */
    uint16_t status;    /* Status Field (includes Phase bit) */
} nvme_cq_entry_t;

/* NVMe Commands */
#define NVME_OP_ADMIN_IDENTIFY 0x06
#define NVME_OP_ADMIN_CREATE_IO_CQ 0x05
#define NVME_OP_ADMIN_CREATE_IO_SQ 0x01

#define NVME_OP_IO_WRITE 0x01
#define NVME_OP_IO_READ  0x02

typedef struct {
    uintptr_t bar0;
    uint32_t db_stride;
    uint16_t max_entries;
    
    nvme_sq_entry_t* admin_sq;
    nvme_cq_entry_t* admin_cq;
    uint16_t admin_sq_tail;
    uint16_t admin_cq_head;
    uint8_t admin_phase;

    nvme_sq_entry_t* io_sq;
    nvme_cq_entry_t* io_cq;
    uint16_t io_sq_tail;
    uint16_t io_cq_head;
    uint8_t io_phase;

    uint32_t nsid;
    uint64_t sector_count;
    uint32_t sector_size;
} nvme_controller_t;

void nvme_init(void);
int nvme_read(uint64_t lba, uint32_t count, void* buffer);
int nvme_write(uint64_t lba, uint32_t count, const void* buffer);

#endif
