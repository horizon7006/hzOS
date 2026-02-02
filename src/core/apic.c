#include "apic.h"
#include "acpi.h"
#include "io.h"
#include "../lib/printf.h"

static uintptr_t g_lapic_base = 0;
static uintptr_t g_ioapic_base = 0;
extern uint64_t g_hhdm_offset;

void lapic_write(uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(g_lapic_base + reg) = val;
}

uint32_t lapic_read(uint32_t reg) {
    return *(volatile uint32_t*)(g_lapic_base + reg);
}

void lapic_init(void) {
    g_lapic_base = acpi_get_lapic_addr() + g_hhdm_offset;
    if (!g_lapic_base) return;

    kprintf("APIC: Initializing LAPIC at %lx\n", g_lapic_base);

    // 1. Disable legacy PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    // 2. Set Spurious Interrupt Vector Register
    // Enable APIC by setting bit 8 and mapping spurious vector to 0xFF
    lapic_write(LAPIC_SVR, lapic_read(LAPIC_SVR) | 0x1FF);

    kprintf("APIC: LAPIC ID %d Version %x\n", lapic_get_id(), lapic_read(LAPIC_VER));
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}

uint32_t lapic_get_id(void) {
    return lapic_read(LAPIC_ID) >> 24;
}

/* IOAPIC implementation */
static void ioapic_write(uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(g_ioapic_base) = reg;
    *(volatile uint32_t*)(g_ioapic_base + 0x10) = val;
}

static uint32_t ioapic_read(uint32_t reg) {
    *(volatile uint32_t*)(g_ioapic_base) = reg;
    return *(volatile uint32_t*)(g_ioapic_base + 0x10);
}

void ioapic_init(void) {
    g_ioapic_base = acpi_get_ioapic_addr() + g_hhdm_offset;
    if (!g_ioapic_base) return;

    kprintf("APIC: Initializing IOAPIC at %lx\n", g_ioapic_base);
    
    uint32_t ver = ioapic_read(0x01);
    uint32_t max_intr = (ver >> 16) & 0xFF;
    kprintf("APIC: IOAPIC Version %x, Max Interrupts %d\n", ver & 0xFF, max_intr + 1);

    // Disable all IRQs initially
    for (uint32_t i = 0; i <= max_intr; i++) {
        ioapic_set_irq(i, 0, (1 << 16)); // Masked
    }
}

void ioapic_set_irq(uint8_t irq, uint32_t apic_id, uint32_t flags_vector) {
    uint32_t low = flags_vector;
    uint32_t high = (apic_id << 24);

    ioapic_write(0x10 + irq * 2, low);
    ioapic_write(0x11 + irq * 2, high);
}
