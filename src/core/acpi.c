#include "acpi.h"
#include "../lib/printf.h"
#include "../lib/memory.h"

static rsdp_descriptor_t* g_rsdp = NULL;
static acpi_sdt_header_t* g_root_sdt = NULL;
static int g_use_xsdt = 0;
extern uint64_t g_hhdm_offset;

static int acpi_validate_checksum(void* addr, int len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += ((uint8_t*)addr)[i];
    }
    return sum == 0;
}

void acpi_init_with_rsdp(void* suggested_rsdp) {
    if (suggested_rsdp) {
        g_rsdp = (rsdp_descriptor_t*)suggested_rsdp;
        kprintf("ACPI: Using provided RSDP at %p\n", g_rsdp);
    } else {
        // Search for RSDP in EBDA (0x80000 to 0x9FFFF) and Main BIOS (0xE0000 to 0xFFFFF)
        for (uintptr_t addr = 0x80000; addr < 0x100000; addr += 16) {
            void* virt_addr = (void*)(addr + g_hhdm_offset);
            if (memcmp(virt_addr, "RSD PTR ", 8) == 0) {
                if (acpi_validate_checksum(virt_addr, 20)) {
                    g_rsdp = (rsdp_descriptor_t*)virt_addr;
                    break;
                }
            }
        }
    }

    if (!g_rsdp) {
        kprintf("ACPI: Failed to find RSDP descriptor\n");
        return;
    }

    kprintf("ACPI: Found RSDP at %p (Rev %d)\n", g_rsdp, g_rsdp->revision);
    
    if (g_rsdp->revision >= 2) {
        rsdp_descriptor_v2_t* rsdp_v2 = (rsdp_descriptor_v2_t*)g_rsdp;
        g_root_sdt = (acpi_sdt_header_t*)(uintptr_t)(rsdp_v2->xsdt_address + g_hhdm_offset);
        g_use_xsdt = 1;
        kprintf("ACPI: Using XSDT at %p\n", g_root_sdt);
    } else {
        g_root_sdt = (acpi_sdt_header_t*)(uintptr_t)(g_rsdp->rsdt_address + g_hhdm_offset);
        g_use_xsdt = 0;
        kprintf("ACPI: Using RSDT at %p\n", g_root_sdt);
    }

    if (!acpi_validate_checksum(g_root_sdt, g_root_sdt->length)) {
        kprintf("ACPI: Root SDT checksum failed\n");
        return;
    }

    acpi_parse_madt();
}

void* acpi_find_table(const char* signature) {
    if (!g_root_sdt) return NULL;

    int entry_size = g_use_xsdt ? 8 : 4;
    int entries = (g_root_sdt->length - sizeof(acpi_sdt_header_t)) / entry_size;

    for (int i = 0; i < entries; i++) {
        acpi_sdt_header_t* header;
        if (g_use_xsdt) {
            uint64_t* tables = (uint64_t*)((uintptr_t)g_root_sdt + sizeof(acpi_sdt_header_t));
            header = (acpi_sdt_header_t*)(uintptr_t)(tables[i] + g_hhdm_offset);
        } else {
            uint32_t* tables = (uint32_t*)((uintptr_t)g_root_sdt + sizeof(acpi_sdt_header_t));
            header = (acpi_sdt_header_t*)(uintptr_t)(tables[i] + g_hhdm_offset);
        }

        if (memcmp(header->signature, signature, 4) == 0) {
            if (acpi_validate_checksum(header, header->length)) {
                return header;
            }
        }
    }

    return NULL;
}

static uintptr_t g_lapic_addr = 0;
static uintptr_t g_ioapic_addr = 0;
static uint8_t g_cpu_ids[64];
static uint32_t g_cpu_count = 0;

void acpi_parse_madt(void) {
    madt_t* madt = (madt_t*)acpi_find_table("APIC");
    if (!madt) {
        kprintf("ACPI: MADT not found\n");
        return;
    }

    g_lapic_addr = madt->lapic_addr;
    kprintf("ACPI: Default LAPIC Addr: %x\n", g_lapic_addr);

    uint8_t* ptr = (uint8_t*)(madt + 1);
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    g_cpu_count = 0;
    while (ptr < end) {
        madt_entry_header_t* entry = (madt_entry_header_t*)ptr;
        if (entry->type == 0) { // Processor Local APIC
            madt_lapic_t* lapic = (madt_lapic_t*)ptr;
            if (g_cpu_count < 64) {
                g_cpu_ids[g_cpu_count++] = lapic->apic_id;
            }
            kprintf("ACPI: Found CPU %d, APIC ID %d, Flags %x\n", 
                    lapic->processor_id, lapic->apic_id, lapic->flags);
        } else if (entry->type == 1) { // I/O APIC
            madt_ioapic_t* ioapic = (madt_ioapic_t*)ptr;
            g_ioapic_addr = ioapic->ioapic_addr;
            kprintf("ACPI: Found IOAPIC ID %d at %x, GSI Base %d\n", 
                    ioapic->ioapic_id, ioapic->ioapic_addr, ioapic->global_system_interrupt_base);
        }
        ptr += entry->length;
    }
}

uintptr_t acpi_get_lapic_addr(void) { return g_lapic_addr; }
uintptr_t acpi_get_ioapic_addr(void) { return g_ioapic_addr; }
uint32_t acpi_get_cpu_count(void) { return g_cpu_count; }
uint8_t acpi_get_cpu_apic_id(uint32_t index) { return g_cpu_ids[index]; }
