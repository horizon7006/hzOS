#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stddef.h>

typedef struct rsdp_descriptor {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) rsdp_descriptor_t;

typedef struct rsdp_descriptor_v2 {
    rsdp_descriptor_t first_part;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_descriptor_v2_t;

typedef struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct madt {
    acpi_sdt_header_t header;
    uint32_t lapic_addr;
    uint32_t flags;
} __attribute__((packed)) madt_t;

typedef struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) madt_entry_header_t;

typedef struct madt_lapic {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_t;

typedef struct madt_ioapic {
    madt_entry_header_t header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_addr;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) madt_ioapic_t;

typedef struct hpet_table {
    acpi_sdt_header_t header;
    uint32_t event_timer_block_id;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) hpet_table_t;

void acpi_init(void);
void* acpi_find_table(const char* signature);
void acpi_parse_madt(void);
uintptr_t acpi_get_lapic_addr(void);
uintptr_t acpi_get_ioapic_addr(void);
uint32_t acpi_get_cpu_count(void);
uint8_t acpi_get_cpu_apic_id(uint32_t index);

#endif
