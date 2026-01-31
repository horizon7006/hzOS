#include "pci.h"
#include "../core/io.h"
#include "serial.h"
#include "../lib/memory.h"

#define MAX_PCI_DEVICES 256

static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t data = inl(PCI_CONFIG_DATA);
    return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}

uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t data = inl(PCI_CONFIG_DATA);
    return data;
}

void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t old_val = inl(PCI_CONFIG_DATA);
    
    // Replace the 16 bits
    uint32_t new_val;
    if (offset & 2) {
        new_val = (old_val & 0xFFFF) | ((uint32_t)value << 16);
    } else {
        new_val = (old_val & 0xFFFF0000) | (uint32_t)value;
    }
    
    outl(PCI_CONFIG_DATA, new_val);
}

void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t data = inl(PCI_CONFIG_DATA);
    return (uint8_t)((data >> ((offset & 3) * 8)) & 0xFF);
}

static void pci_scan_bus(uint8_t bus);

static void pci_check_device(uint8_t bus, uint8_t device) {
    uint16_t vendor_id = pci_config_read_word(bus, device, 0, PCI_VENDOR_ID);
    
    if (vendor_id == 0xFFFF) {
        return; // No device
    }
    
    // Check if multifunction device
    uint8_t header_type = pci_config_read_byte(bus, device, 0, PCI_HEADER_TYPE);
    
    // Scan function 0
    uint16_t device_id = pci_config_read_word(bus, device, 0, PCI_DEVICE_ID);
    uint8_t class_code = pci_config_read_byte(bus, device, 0, PCI_CLASS);
    uint8_t subclass = pci_config_read_byte(bus, device, 0, PCI_SUBCLASS);
    uint8_t prog_if = pci_config_read_byte(bus, device, 0, PCI_PROG_IF);
    
    if (pci_device_count < MAX_PCI_DEVICES) {
        pci_devices[pci_device_count].bus = bus;
        pci_devices[pci_device_count].device = device;
        pci_devices[pci_device_count].function = 0;
        pci_devices[pci_device_count].vendor_id = vendor_id;
        pci_devices[pci_device_count].device_id = device_id;
        pci_devices[pci_device_count].class_code = class_code;
        pci_devices[pci_device_count].subclass = subclass;
        pci_devices[pci_device_count].prog_if = prog_if;
        pci_devices[pci_device_count].header_type = header_type;
        pci_devices[pci_device_count].bar0 = pci_config_read_dword(bus, device, 0, PCI_BAR0);
        pci_devices[pci_device_count].bar1 = pci_config_read_dword(bus, device, 0, PCI_BAR1);
        pci_devices[pci_device_count].bar2 = pci_config_read_dword(bus, device, 0, PCI_BAR2);
        pci_devices[pci_device_count].bar3 = pci_config_read_dword(bus, device, 0, PCI_BAR3);
        pci_devices[pci_device_count].bar4 = pci_config_read_dword(bus, device, 0, PCI_BAR4);
        pci_devices[pci_device_count].bar5 = pci_config_read_dword(bus, device, 0, PCI_BAR5);
        pci_device_count++;
    }
    
    // If multifunction, check other functions
    if ((header_type & 0x80) != 0) {
        for (uint8_t func = 1; func < 8; func++) {
            vendor_id = pci_config_read_word(bus, device, func, PCI_VENDOR_ID);
            if (vendor_id != 0xFFFF) {
                device_id = pci_config_read_word(bus, device, func, PCI_DEVICE_ID);
                class_code = pci_config_read_byte(bus, device, func, PCI_CLASS);
                subclass = pci_config_read_byte(bus, device, func, PCI_SUBCLASS);
                prog_if = pci_config_read_byte(bus, device, func, PCI_PROG_IF);
                header_type = pci_config_read_byte(bus, device, func, PCI_HEADER_TYPE);
                
                if (pci_device_count < MAX_PCI_DEVICES) {
                    pci_devices[pci_device_count].bus = bus;
                    pci_devices[pci_device_count].device = device;
                    pci_devices[pci_device_count].function = func;
                    pci_devices[pci_device_count].vendor_id = vendor_id;
                    pci_devices[pci_device_count].device_id = device_id;
                    pci_devices[pci_device_count].class_code = class_code;
                    pci_devices[pci_device_count].subclass = subclass;
                    pci_devices[pci_device_count].prog_if = prog_if;
                    pci_devices[pci_device_count].header_type = header_type;
                    pci_devices[pci_device_count].bar0 = pci_config_read_dword(bus, device, func, PCI_BAR0);
                    pci_devices[pci_device_count].bar1 = pci_config_read_dword(bus, device, func, PCI_BAR1);
                    pci_devices[pci_device_count].bar2 = pci_config_read_dword(bus, device, func, PCI_BAR2);
                    pci_devices[pci_device_count].bar3 = pci_config_read_dword(bus, device, func, PCI_BAR3);
                    pci_devices[pci_device_count].bar4 = pci_config_read_dword(bus, device, func, PCI_BAR4);
                    pci_devices[pci_device_count].bar5 = pci_config_read_dword(bus, device, func, PCI_BAR5);
                    pci_device_count++;
                }
            }
        }
    }
}

static void pci_scan_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        pci_check_device(bus, device);
    }
}

void pci_init(void) {
    serial_printf("PCI: Scanning PCI bus...\n");
    pci_device_count = 0;
    
    // Iteratively scan buses 0-7. This avoids recursion issues with bridges.
    for (uint8_t bus = 0; bus < 8; bus++) {
        pci_scan_bus(bus);
    }
    
    serial_printf("PCI: Found %d device(s)\n", pci_device_count);
}

int pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t* pci_get_device(int index) {
    if (index < 0 || index >= pci_device_count) {
        return 0;
    }
    return &pci_devices[index];
}

pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return 0;
}
