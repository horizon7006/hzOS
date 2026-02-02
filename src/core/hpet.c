#include "hpet.h"
#include "acpi.h"
#include "../lib/printf.h"

static uintptr_t g_hpet_base = 0;
static uint64_t g_hpet_period = 0; // In femtoseconds
extern uint64_t g_hhdm_offset;

static void hpet_write(uint32_t reg, uint64_t val) {
    *(volatile uint64_t*)(g_hpet_base + reg) = val;
}

static uint64_t hpet_read(uint32_t reg) {
    return *(volatile uint64_t*)(g_hpet_base + reg);
}

void hpet_init(void) {
    hpet_table_t* hpet = (hpet_table_t*)acpi_find_table("HPET");
    if (!hpet) {
        kprintf("ACPI: HPET table not found\n");
        return;
    }

    g_hpet_base = hpet->address + g_hhdm_offset;
    kprintf("HPET: Found HPET at %lx\n", g_hpet_base);

    uint64_t caps = hpet_read(HPET_REG_GENERAL_CAPABILITIES);
    g_hpet_period = caps >> 32;

    kprintf("HPET: Period %ld fs, Vendor %x\n", g_hpet_period, (uint32_t)(caps >> 16) & 0xFFFF);

    // Enable HPET
    hpet_write(HPET_REG_GENERAL_CONFIGURATION, hpet_read(HPET_REG_GENERAL_CONFIGURATION) | 1);
}

void hpet_sleep(uint64_t ms) {
    uint64_t target = hpet_read(HPET_REG_MAIN_COUNTER_VALUE) + (ms * 1000000000000ULL) / g_hpet_period;
    while (hpet_read(HPET_REG_MAIN_COUNTER_VALUE) < target);
}

uint64_t hpet_get_nanos(void) {
    return (hpet_read(HPET_REG_MAIN_COUNTER_VALUE) * g_hpet_period) / 1000000ULL;
}
