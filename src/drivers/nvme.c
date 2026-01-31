#include "nvme.h"
#include "../lib/memory.h"
#include "../lib/printf.h"
#include "../core/io.h"

static nvme_controller_t g_nvme;

static void nvme_write_reg32(nvme_controller_t* nvme, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(nvme->bar0 + reg) = val;
}

static uint32_t nvme_read_reg32(nvme_controller_t* nvme, uint32_t reg) {
    return *(volatile uint32_t*)(nvme->bar0 + reg);
}

static uint64_t nvme_read_reg64(nvme_controller_t* nvme, uint32_t reg) {
    return *(volatile uint64_t*)(nvme->bar0 + reg);
}

static void nvme_write_reg64(nvme_controller_t* nvme, uint32_t reg, uint64_t val) {
    *(volatile uint64_t*)(nvme->bar0 + reg) = val;
}

static void nvme_write_doorbell(nvme_controller_t* nvme, uint32_t index, int is_cq, uint16_t val) {
    uint32_t offset = 0x1000 + (index * 2 + (is_cq ? 1 : 0)) * (4 << nvme->db_stride);
    nvme_write_reg32(nvme, offset, val);
}

static int nvme_controller_init(nvme_controller_t* nvme, pci_device_t* pci) {
    uint64_t full_bar = pci->bar0 & ~0xF;
    if ((pci->bar0 & 0x6) == 0x4) { // 64-bit BAR
        full_bar |= ((uint64_t)pci->bar1 << 32);
    }
    nvme->bar0 = full_bar;
    
    // 1. Get Capabilities
    uint64_t cap = nvme_read_reg64(nvme, NVME_REG_CAP);
    nvme->db_stride = (cap >> 32) & 0xF;
    nvme->max_entries = (cap & 0xFFFF) + 1;
    
    kprintf("NVMe: BAR=%lx CAP=%lx Stride=%d MaxEntries=%d\n", nvme->bar0, cap, nvme->db_stride, nvme->max_entries);
    
    // 2. Reset Controller
    uint32_t cc = nvme_read_reg32(nvme, NVME_REG_CC);
    if (cc & 1) { // If already enabled, disable first
        nvme_write_reg32(nvme, NVME_REG_CC, cc & ~1);
        // Wait for Ready=0 in CSTS
        int timeout = 1000000;
        while (nvme_read_reg32(nvme, NVME_REG_CSTS) & 1) {
            if (--timeout == 0) {
                kprintf("NVMe: Timeout waiting for Ready=0\n");
                return -1;
            }
        }
    }
    
    // 3. Setup Admin Queues
    // For simplicity, use 64 entries for Admin Queues
    uint16_t admin_q_size = 64;
    nvme->admin_sq = (nvme_sq_entry_t*)kmalloc_raw_aligned(4096);
    nvme->admin_cq = (nvme_cq_entry_t*)kmalloc_raw_aligned(4096);
    memset(nvme->admin_sq, 0, 4096);
    memset(nvme->admin_cq, 0, 4096);
    
    // CC.CSS (Command Set Selected) = 0 for NVM
    // CC.MPS (Memory Page Size) = 0 for 4KB (2^(12+0))
    // CC.AMS (Arbitration Mechanism Selected) = 0 (Round Robin)
    // CC.IOCQES = 4 (Log2(16)), CC.IOSQES = 6 (Log2(64))
    cc = (0 << 11) | (0 << 7) | (4 << 20) | (6 << 16); 
    nvme_write_reg32(nvme, NVME_REG_CC, cc);
    
    // AQA: ACQS (0-11), ASQS (16-27)
    // Value is 0-based
    uint32_t aqa = ((admin_q_size - 1) << 16) | (admin_q_size - 1);
    nvme_write_reg32(nvme, NVME_REG_AQA, aqa);
    
    nvme_write_reg64(nvme, NVME_REG_ASQ, (uintptr_t)nvme->admin_sq);
    nvme_write_reg64(nvme, NVME_REG_ACQ, (uintptr_t)nvme->admin_cq);
    
    // 4. Enable Controller
    nvme_write_reg32(nvme, NVME_REG_CC, cc | 1);
    
    // Wait for Ready=1 in CSTS
    int timeout = 1000000;
    while (!(nvme_read_reg32(nvme, NVME_REG_CSTS) & 1)) {
        if (--timeout == 0) {
            kprintf("NVMe: Timeout waiting for Ready=1\n");
            return -1;
        }
    }
    
    nvme->admin_sq_tail = 0;
    nvme->admin_cq_head = 0;
    nvme->admin_phase = 1;
    
    kprintf("NVMe: Controller Enabled\n");
    return 0;
}

static int nvme_submit_admin_cmd(nvme_controller_t* nvme, nvme_sq_entry_t* cmd, nvme_cq_entry_t* res) {
    // 1. Copy command to SQ
    memcpy(&nvme->admin_sq[nvme->admin_sq_tail], cmd, sizeof(nvme_sq_entry_t));
    
    // 2. Increment Tail
    nvme->admin_sq_tail = (nvme->admin_sq_tail + 1) % 64; // Hardcoded 64 for now
    
    // 3. Ring Doorbell
    nvme_write_doorbell(nvme, 0, 0, nvme->admin_sq_tail);
    
    // 4. Wait for Completion
    volatile nvme_cq_entry_t* cq = &nvme->admin_cq[nvme->admin_cq_head];
    int timeout = 10000000;
    while ((cq->status & 1) != nvme->admin_phase) {
        if (--timeout == 0) {
             kprintf("NVMe: Admin Cmd Timeout\n");
             return -1;
        }
    }
    
    // 5. Copy result
    if (res) memcpy(res, (void*)cq, sizeof(nvme_cq_entry_t));
    
    // 6. Increment Head
    nvme->admin_cq_head = (nvme->admin_cq_head + 1) % 64;
    if (nvme->admin_cq_head == 0) nvme->admin_phase ^= 1;
    
    // 7. Ring Doorbell (Completion Head)
    nvme_write_doorbell(nvme, 0, 1, nvme->admin_cq_head);
    
    return cq->status >> 1;
}

static int nvme_identify(nvme_controller_t* nvme) {
    nvme_sq_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    
    void* buffer = kmalloc_raw_aligned(4096);
    memset(buffer, 0, 4096);
    
    cmd.cdw0 = NVME_OP_ADMIN_IDENTIFY;
    cmd.prp1 = (uintptr_t)buffer;
    cmd.cdw10 = 1; // Identify Controller
    
    nvme_cq_entry_t res;
    int status = nvme_submit_admin_cmd(nvme, &cmd, &res);
    if (status != 0) {
        kprintf("NVMe: Identify Controller failed status=%x\n", status);
        return -1;
    }
    
    // Parse Identify Controller structure
    // [24:255] Model Number (ASCII)
    char model[41];
    memcpy(model, (uint8_t*)buffer + 24, 40);
    model[40] = 0;
    kprintf("NVMe: Model: %s\n", model);
    
    // [516:519] Number of Namespaces
    uint32_t nn = *(uint32_t*)((uint8_t*)buffer + 516);
    kprintf("NVMe: Namespaces: %d\n", nn);
    
    if (nn > 0) {
        // Identify Namespace 1
        memset(&cmd, 0, sizeof(cmd));
        cmd.cdw0 = NVME_OP_ADMIN_IDENTIFY;
        cmd.prp1 = (uintptr_t)buffer;
        cmd.nsid = 1;
        cmd.cdw10 = 0; // Identify Namespace
        
        status = nvme_submit_admin_cmd(nvme, &cmd, &res);
        if (status == 0) {
            uint64_t nsze = *(uint64_t*)((uint8_t*)buffer + 0);
            uint8_t flbas = *(uint8_t*)((uint8_t*)buffer + 26);
            uint8_t lbads = *(uint8_t*)((uint8_t*)buffer + 128 + (flbas & 0xF) * 4 + 2);
            
            nvme->nsid = 1;
            nvme->sector_count = nsze;
            nvme->sector_size = 1 << lbads;
            
            kprintf("NVMe: NS1 Size: %ld sectors, BlockSize: %d bytes\n", nsze, nvme->sector_size);
        }
    }
    
    return 0;
}

static int nvme_create_io_queues(nvme_controller_t* nvme) {
    // 1. Create I/O Completion Queue
    nvme_sq_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    
    nvme->io_cq = (nvme_cq_entry_t*)kmalloc_raw_aligned(4096);
    memset(nvme->io_cq, 0, 4096);
    
    // Create I/O CQ (ID 1)
    /* CDW10: QID=1, QSIZE=511 (512 entries) */
    cmd.cdw0 = NVME_OP_ADMIN_CREATE_IO_CQ;
    cmd.prp1 = (uintptr_t)nvme->io_cq;
    cmd.cdw10 = (511 << 16) | 1; 
    cmd.cdw11 = 1; // PC (Physically Contiguous) = 1
    
    int status = nvme_submit_admin_cmd(nvme, &cmd, NULL);
    if (status != 0) {
        kprintf("NVMe: Create I/O CQ failed status=%x\n", status);
        return -1;
    }
    
    // 2. Create I/O Submission Queue
    memset(&cmd, 0, sizeof(cmd));
    nvme->io_sq = (nvme_sq_entry_t*)kmalloc_raw_aligned(4096);
    memset(nvme->io_sq, 0, 4096);
    
    /* CDW10: QID=1, QSIZE=511 */
    cmd.cdw0 = NVME_OP_ADMIN_CREATE_IO_SQ;
    cmd.prp1 = (uintptr_t)nvme->io_sq;
    cmd.cdw10 = (511 << 16) | 1;
    cmd.cdw11 = (1 << 16) | 1; // CQID=1, PC=1
    
    status = nvme_submit_admin_cmd(nvme, &cmd, NULL);
    if (status != 0) {
        kprintf("NVMe: Create I/O SQ failed status=%x\n", status);
        return -1;
    }
    
    nvme->io_sq_tail = 0;
    nvme->io_cq_head = 0;
    nvme->io_phase = 1;
    
    kprintf("NVMe: I/O Queues Created\n");
    return 0;
}

void nvme_init(void) {
    pci_device_t* pci = 0;
    for (int i = 0; i < pci_get_device_count(); i++) {
        pci_device_t* dev = pci_get_device(i);
        if (dev->class_code == 0x01 && dev->subclass == 0x08 && dev->prog_if == 0x02) {
            pci = dev;
            break;
        }
    }
    
    if (!pci) {
        kprintf("NVMe: No controller found.\n");
        return;
    }
    
    kprintf("NVMe: Found controller at %d:%d:%d\n", pci->bus, pci->device, pci->function);
    
    if (nvme_controller_init(&g_nvme, pci) == 0) {
        if (nvme_identify(&g_nvme) == 0) {
            if (nvme_create_io_queues(&g_nvme) == 0) {
                kprintf("NVMe: Ready for I/O\n");
            }
        }
    }
}

static uint16_t g_cid = 0;

int nvme_read(uint64_t lba, uint32_t count, void* buffer) {
    if (!g_nvme.io_sq) return -1;

    nvme_sq_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.cdw0 = NVME_OP_IO_READ | (g_cid << 16);
    cmd.nsid = g_nvme.nsid;
    cmd.prp1 = (uintptr_t)buffer;
    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (count - 1) & 0xFFFF; // 0-based

    // 1. Submit to I/O SQ
    memcpy(&g_nvme.io_sq[g_nvme.io_sq_tail], &cmd, sizeof(nvme_sq_entry_t));
    g_nvme.io_sq_tail = (g_nvme.io_sq_tail + 1) % 512;
    nvme_write_doorbell(&g_nvme, 1, 0, g_nvme.io_sq_tail);

    // 2. Wait for Completion on I/O CQ
    volatile nvme_cq_entry_t* cq = &g_nvme.io_cq[g_nvme.io_cq_head];
    int timeout = 10000000;
    while ((cq->status & 1) != g_nvme.io_phase) {
        if (--timeout == 0) {
            kprintf("NVMe: I/O Read Timeout\n");
            return -1;
        }
    }

    int status = cq->status >> 1;
    
    // 3. Move Head
    g_nvme.io_cq_head = (g_nvme.io_cq_head + 1) % 512;
    if (g_nvme.io_cq_head == 0) g_nvme.io_phase ^= 1;
    nvme_write_doorbell(&g_nvme, 1, 1, g_nvme.io_cq_head);

    g_cid++;
    return status;
}

int nvme_write(uint64_t lba, uint32_t count, const void* buffer) {
    if (!g_nvme.io_sq) return -1;
    
    nvme_sq_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.cdw0 = NVME_OP_IO_WRITE | (g_cid << 16);
    cmd.nsid = g_nvme.nsid;
    cmd.prp1 = (uintptr_t)buffer;
    cmd.cdw10 = (uint32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (count - 1) & 0xFFFF;

    memcpy(&g_nvme.io_sq[g_nvme.io_sq_tail], &cmd, sizeof(nvme_sq_entry_t));
    g_nvme.io_sq_tail = (g_nvme.io_sq_tail + 1) % 512;
    nvme_write_doorbell(&g_nvme, 1, 0, g_nvme.io_sq_tail);

    volatile nvme_cq_entry_t* cq = &g_nvme.io_cq[g_nvme.io_cq_head];
    int timeout = 10000000;
    while ((cq->status & 1) != g_nvme.io_phase) {
        if (--timeout == 0) {
            kprintf("NVMe: I/O Write Timeout\n");
            return -1;
        }
    }

    int status = cq->status >> 1;

    g_nvme.io_cq_head = (g_nvme.io_cq_head + 1) % 512;
    if (g_nvme.io_cq_head == 0) g_nvme.io_phase ^= 1;
    nvme_write_doorbell(&g_nvme, 1, 1, g_nvme.io_cq_head);

    g_cid++;
    return status;
}
