#ifndef PCI_H
#define PCI_H

#include "../core/common.h"

/* PCI Configuration Space Registers */
#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_PROG_IF              0x09
#define PCI_SUBCLASS             0x0A
#define PCI_CLASS                0x0B
#define PCI_HEADER_TYPE          0x0E
#define PCI_BAR0                 0x10
#define PCI_BAR1                 0x14
#define PCI_BAR2                 0x18
#define PCI_BAR3                 0x1C
#define PCI_BAR4                 0x20
#define PCI_BAR5                 0x24

/* PCI I/O Ports */
#define PCI_CONFIG_ADDRESS       0xCF8
#define PCI_CONFIG_DATA          0xCFC

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t header_type;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
} pci_device_t;

/* Initialize PCI subsystem and enumerate devices */
void pci_init(void);

/* Get number of detected PCI devices */
int pci_get_device_count(void);

/* Get device info by index */
pci_device_t* pci_get_device(int index);

/* Read from PCI configuration space */
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Write to PCI configuration space */
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t value);
void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);
uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Find device by vendor and device ID */
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);

#endif
