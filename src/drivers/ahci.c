#include "ahci.h"
#include "pci.h"
#include "printf.h"
#include "memory.h"
#include "io.h"
#include "../core/paging.h"
#include "../fs/blockdev.h"
#include "../lib/string.h" // for sprintf/strcpy

static int ahci_bd_read(block_device_t* dev, uint64_t lba, uint32_t count, void* buffer) {
    HBA_PORT* port = (HBA_PORT*)dev->private_data;
    return ahci_read(port, (uint32_t)lba, (uint32_t)(lba >> 32), count, (uint16_t*)buffer);
}

static int ahci_bd_write(block_device_t* dev, uint64_t lba, uint32_t count, const void* buffer) {
    (void)dev; (void)lba; (void)count; (void)buffer;
    return -1; // Write not implemented yet
}

static HBA_MEM* hba_mem = NULL;
static HBA_PORT* sata_ports[32];
static int sata_port_count = 0;

static int check_type(HBA_PORT* port) {
    uint32_t ssts = port->ssts;

    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT) return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE) return AHCI_DEV_NULL;

    switch (port->sig) {
        case SATA_SIG_ATAPI: return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:  return AHCI_DEV_SEMB;
        case SATA_SIG_PM:    return AHCI_DEV_PM;
        default:             return AHCI_DEV_SATA;
    }
}

static void start_cmd(HBA_PORT* port) {
    // Wait until CR is cleared
    int spin = 0;
    while ((port->cmd & HBA_PxCMD_CR) && spin < 1000000) spin++;
    if (spin == 1000000) {
        serial_printf("ahci: start_cmd timeout (CR not cleared)\n");
    }

    // Set FRE and ST
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

static void stop_cmd(HBA_PORT* port) {
    // Clear pending interrupts and errors
    port->is = (uint32_t)-1;
    port->serr = (uint32_t)-1;

    // 1. Clear ST (Start)
    port->cmd &= ~HBA_PxCMD_ST;
    __asm__ __volatile__("" : : : "memory");
    
    // 2. Wait for CR (Command List Running) to be cleared
    int spin = 0;
    while ((port->cmd & HBA_PxCMD_CR) && spin < 1000000) {
        spin++;
        __asm__ __volatile__("pause" : : : "memory");
    }

    // 3. Clear FRE (FIS Receive Enable)
    port->cmd &= ~HBA_PxCMD_FRE;
    __asm__ __volatile__("" : : : "memory");

    // 4. Wait for FR (FIS Receive Running) to be cleared
    spin = 0;
    while ((port->cmd & HBA_PxCMD_FR) && spin < 1000000) {
        spin++;
        __asm__ __volatile__("pause" : : : "memory");
    }

    if (spin == 1000000 || (port->cmd & HBA_PxCMD_CR)) {
        serial_printf("ahci: stop_cmd failed to idle. Performing port reset...\n");
        // Port Reset (COMRESET)
        port->sctl = (port->sctl & ~0x0F) | 0x01; // DET = 1 (Perform interface reset)
        for (int d = 0; d < 1000000; d++) __asm__ __volatile__("pause");
        port->sctl &= ~0x0F; // DET = 0 (Return to normal operation)
        while ((port->ssts & 0x0F) != 0x03); // Wait for device present and communication established
        port->serr = (uint32_t)-1; // Clear errors again
    }
}

static int find_cmd_slot(HBA_PORT* port) {
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0) return i;
        slots >>= 1;
    }
    return -1;
}

void ahci_init(void) {
    serial_printf("ahci: initializing...\n");

    // Search for AHCI controller
    int dev_count = pci_get_device_count();
    pci_device_t* ahci_dev = NULL;

    for (int i = 0; i < dev_count; i++) {
        pci_device_t* dev = pci_get_device(i);
        // Mass Storage (01), SATA (06)
        if (dev->class_code == 0x01 && dev->subclass == 0x06) {
            ahci_dev = dev;
            break;
        }
    }

    if (!ahci_dev) {
        serial_printf("ahci: no AHCI controller found\n");
        return;
    }

    serial_printf("ahci: found controller at %x:%x.%x\n", 
            ahci_dev->bus, ahci_dev->device, ahci_dev->function);

    // Enable Memory Space and Bus Mastering
    uint16_t command = pci_config_read_word(ahci_dev->bus, ahci_dev->device, ahci_dev->function, PCI_COMMAND);
    command |= (1 << 1); // Memory Space
    command |= (1 << 2); // Bus Master
    pci_config_write_word(ahci_dev->bus, ahci_dev->device, ahci_dev->function, PCI_COMMAND, command);

    // ABAR is at BAR 5. Mask lower bits (flags)
    uint64_t abar_phys = ahci_dev->bar5 & 0xFFFFFFF0;
    extern uint64_t g_hhdm_offset;
    abar_phys += g_hhdm_offset;
    
    // Map AHCI MMIO region (usually 1KB per port, but 4KB is safe for ABAR)
    paging_map_mmio(abar_phys, 4096);
    
    hba_mem = (HBA_MEM*)abar_phys;
    
    serial_printf("ahci: MMIO at %lx\n", abar_phys);
    
    // AE (AHCI Enable) must be 1 before HR or any other port registers
    hba_mem->ghc |= (1 << 31);
    for (int d = 0; d < 1000000; d++) __asm__ __volatile__("pause");

    // HBA Reset
    serial_printf("ahci: performing HBA reset...\n");
    hba_mem->ghc |= (1 << 0); // HR (HBA Reset)
    int spin = 0;
    while ((hba_mem->ghc & 0x01) && spin < 20000000) spin++;
    if (spin == 20000000) {
        serial_printf("ahci: HBA reset timeout!\n");
    }

    // AE must be re-enabled after reset as it clears
    for (int d = 0; d < 1000000; d++) __asm__ __volatile__("pause");
    serial_printf("ahci: enabling AHCI mode (AE)...\n");
    hba_mem->ghc |= (1 << 31); // AE (AHCI Enable)
    for (int d = 0; d < 1000000; d++) __asm__ __volatile__("pause");
    
    // Global Interrupt Enable
    hba_mem->ghc |= (1 << 1);  // IE (Interrupt Enable)

    serial_printf("ahci: reading version...\n");
    uint32_t vs = hba_mem->vs;
    serial_printf("ahci: version %d.%d\n", (vs >> 16), vs & 0xFFFF);

    // Check implemented ports
    uint32_t pi = hba_mem->pi;
    serial_printf("ahci: ports implemented mask: %x\n", pi);
    
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            int type = check_type(&hba_mem->ports[i]);
            if (type == AHCI_DEV_SATA) {
                kprintf("ahci: port %d: SATA drive found\n", i);
                serial_printf("ahci: port %d: SATA drive found\n", i);
                
                HBA_PORT* port = &hba_mem->ports[i];
                sata_ports[sata_port_count++] = port;
                
                // Set up port memory (rebase)
                serial_printf("ahci: port %d: rebasing...\n", i);
                stop_cmd(port);
                
                // Command list offset: 1K aligned
                void* cl_addr = kmalloc_a(1024);
                memset(cl_addr, 0, 1024);
                port->clb = (uint32_t)VIRT_TO_PHYS(cl_addr);
                port->clbu = (uint32_t)(VIRT_TO_PHYS(cl_addr) >> 32);

                // FIS offset: 256 bytes aligned
                void* fis_addr = kmalloc_a(256);
                memset(fis_addr, 0, 256);
                port->fb = (uint32_t)VIRT_TO_PHYS(fis_addr);
                port->fbu = (uint32_t)(VIRT_TO_PHYS(fis_addr) >> 32);

                // Command table offset: 128 bytes aligned
                HBA_CMD_HEADER* cmdhdr = (HBA_CMD_HEADER*)cl_addr;
                for (int j = 0; j < 32; j++) {
                    cmdhdr[j].prdtl = 8; // 8 PRDT entries per command table
                    void* ct_addr = kmalloc_a(256);
                    memset(ct_addr, 0, 256);
                    cmdhdr[j].ctba = (uint32_t)VIRT_TO_PHYS(ct_addr);
                    cmdhdr[j].ctbau = (uint32_t)(VIRT_TO_PHYS(ct_addr) >> 32);
                }

                start_cmd(port);
                serial_printf("ahci: port %d: ready\n", i);
                
                // Identify size
                void* id_buf = kmalloc_raw_aligned(4096); // Use raw aligned to be safe with phys
                if (ahci_identify(port, id_buf) == 0) {
                    uint16_t* id = (uint16_t*)id_buf;
                    uint64_t sectors = 0;
                    if (id[83] & (1 << 10)) { // LBA48 supported
                        sectors = *(uint64_t*)&id[100];
                    } else {
                        sectors = *(uint32_t*)&id[60];
                    }
                    kprintf("ahci: port %d size: %d MB (%ld sectors)\n", i, (sectors * 512) / (1024*1024), sectors);

                    block_device_t* bd = kmalloc(sizeof(block_device_t));
                    // char name works because we included string.h/printf.h? 
                    // No sprintf is not standard C, usually we have it in our lib or use kprintf/snprintf. 
                    // Let's use simple strcpy and manual digit.
                    strcpy(bd->name, "sata0"); 
                    bd->name[4] = '0' + i; // Simple hack for sata0..sata9
                    
                    bd->sector_count = sectors;
                    bd->sector_size = 512;
                    bd->read = ahci_bd_read;
                    bd->write = ahci_bd_write;
                    bd->private_data = port;
                    
                    blockdev_register(bd);
                    
                    extern void gpt_read_table(block_device_t* dev);
                    gpt_read_table(bd);
                } else {
                     kprintf("ahci: port %d identify failed\n", i);
                }
                kfree(id_buf); 
            }
        }
    }
    serial_printf("ahci: initialization complete\n");
}

int ahci_identify(HBA_PORT* port, uint16_t* buf) {
    port->is = (uint32_t)-1;
    int slot = find_cmd_slot(port);
    if (slot == -1) return -1;

    HBA_CMD_HEADER* cmdhdr = (HBA_CMD_HEADER*)(port->clb);
    cmdhdr += slot;
    cmdhdr->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdhdr->w = 0;
    cmdhdr->prdtl = 1;

    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(cmdhdr->ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));

    cmdtbl->prdt_entry[0].dba = (uint32_t)VIRT_TO_PHYS(buf);
    cmdtbl->prdt_entry[0].dbau = (uint32_t)(VIRT_TO_PHYS(buf) >> 32);
    cmdtbl->prdt_entry[0].dbc = 511; // 512 bytes
    cmdtbl->prdt_entry[0].i = 1;

    FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0xEC; // Identify Device

    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) spin++;
    if (spin == 1000000) return -1;

    port->ci = (1 << slot);
    __asm__ __volatile__("" : : : "memory");

    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) return -1;
        __asm__ __volatile__("pause" : : : "memory");
    }

    return 0;
}

int ahci_read(HBA_PORT* port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t* buf) {
    serial_printf("ahci: read request: LBA %x:%x, count %d\n", starth, startl, count);
    
    port->is = (uint32_t)-1;
    int slot = find_cmd_slot(port);
    if (slot == -1) {
        serial_printf("ahci: no free command slots\n");
        return -1;
    }

    HBA_CMD_HEADER* cmdhdr = (HBA_CMD_HEADER*)(port->clb);
    cmdhdr += slot;
    cmdhdr->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmdhdr->w = 0;
    cmdhdr->prdtl = (uint16_t)((count - 1) >> 4) + 1;

    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(cmdhdr->ctba);
    // Be careful with memset if prdt_entry count is large, but for 1 sector it's small
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) + (cmdhdr->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

    uint16_t* temp_buf = buf;
    uint32_t temp_count = count;
    int i;
    for (i = 0; i < cmdhdr->prdtl - 1; i++) {
        cmdtbl->prdt_entry[i].dba = (uint32_t)VIRT_TO_PHYS(temp_buf);
        cmdtbl->prdt_entry[i].dbau = (uint32_t)(VIRT_TO_PHYS(temp_buf) >> 32);
        cmdtbl->prdt_entry[i].rsv0 = 0;
        cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1; // 8K bytes
        cmdtbl->prdt_entry[i].i = 1;
        temp_buf += 4 * 1024;
        temp_count -= 16;
    }
    
    cmdtbl->prdt_entry[i].dba = (uint32_t)VIRT_TO_PHYS(temp_buf);
    cmdtbl->prdt_entry[i].dbau = (uint32_t)(VIRT_TO_PHYS(temp_buf) >> 32);
    cmdtbl->prdt_entry[i].rsv0 = 0;
    cmdtbl->prdt_entry[i].dbc = (temp_count << 9) - 1;
    cmdtbl->prdt_entry[i].i = 1;

    FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;

    // Use LBA28 for small addresses, LBA48 for large addresses
    if (starth == 0 && startl < 0x0FFFFFFF) {
        cmdfis->command = 0xC8; // READ DMA
        cmdfis->device = (1 << 6) | ((startl >> 24) & 0x0F); // LBA mode + head
    } else {
        cmdfis->command = 0x25; // READ DMA EX
        cmdfis->device = (1 << 6); // LBA mode
    }

    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl >> 8);
    cmdfis->lba2 = (uint8_t)(startl >> 16);
    
    cmdfis->lba3 = (uint8_t)(startl >> 24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth >> 8);

    cmdfis->countl = (uint8_t)count;
    cmdfis->counth = (uint8_t)(count >> 8);

    int spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
        spin++;
        __asm__ __volatile__("pause" : : : "memory");
    }
    if (spin == 1000000) {
        serial_printf("ahci: port is hung before issue. TFD=%x\n", port->tfd);
        return -1;
    }

    port->ci = (1 << slot);
    __asm__ __volatile__("" : : : "memory");

    // Wait for completion
    spin = 0;
    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        
        if (port->is & (1 << 30)) {
            serial_printf("ahci: task file error. TFD=%x, IS=%x, SERR=%x\n", 
                          port->tfd, port->is, port->serr);
            return -1;
        }
        spin++;
        if (spin > 20000000) {
            serial_printf("ahci: timeout. CI=%x, TFD=%x, IS=%x, SERR=%x\n", 
                          port->ci, port->tfd, port->is, port->serr);
            return -1;
        }
        __asm__ __volatile__("pause" : : : "memory");
    }

    serial_printf("ahci: read successful after %d spins\n", spin);
    return 0;
}

int ahci_get_port_count(void) {
    return sata_port_count;
}

HBA_PORT* ahci_get_port(int index) {
    if (index < 0 || index >= sata_port_count) return NULL;
    return sata_ports[index];
}
