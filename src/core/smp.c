#include "smp.h"
#include "acpi.h"
#include "apic.h"
#include "hpet.h"
#include "../lib/memory.h"
#include "../lib/printf.h"
#include "gdt.h"
#include "idt.h"
extern uint64_t g_hhdm_offset;

extern uint8_t _binary_build_smp_trampoline_bin_start[];
extern uint8_t _binary_build_smp_trampoline_bin_end[];

static volatile uint32_t g_cpus_online = 1;

void kernel_ap_main(void) {
    // Adopt kernel state
    extern void gdt_ap_load(void);
    extern void idt_ap_load(void);
    gdt_ap_load();
    idt_ap_load();

    lapic_init(); // Init LAPIC for this CPU
    serial_printf("SMP: CPU %d is online\n", lapic_get_id());
    __atomic_add_fetch(&g_cpus_online, 1, __ATOMIC_SEQ_CST);
    while(1) { __asm__("hlt"); }
}

void smp_boot_cpu(uint8_t apic_id) {
    uintptr_t trampoline_phys = 0x8000;
    size_t trampoline_size = _binary_build_smp_trampoline_bin_end - _binary_build_smp_trampoline_bin_start;
    
    // Copy trampoline
    memcpy((void*)(trampoline_phys + g_hhdm_offset), _binary_build_smp_trampoline_bin_start, trampoline_size);

    // Setup data at the end of trampoline
    // We'll use the last 24 bytes as defined in .S
    uint64_t pml4;
    __asm__ volatile("mov %%cr3, %0" : "=r"(pml4));

    void* stack = kmalloc(16384); // 16KB stack
    
    // Offsets from end of trampoline: 20 (ap_pml4), 16 (ap_stack), 8 (ap_entry_ptr)
    // Offsets from end of trampoline: 20 (ap_pml4), 16 (ap_stack), 8 (ap_entry_ptr)
    *(uint32_t*)(trampoline_phys + g_hhdm_offset + trampoline_size - 20) = pml4;
    *(uint64_t*)(trampoline_phys + g_hhdm_offset + trampoline_size - 16) = (uintptr_t)stack + 16384;
    *(uint64_t*)(trampoline_phys + g_hhdm_offset + trampoline_size - 8) = (uintptr_t)kernel_ap_main;

    // Send INIT IPI
    lapic_write(LAPIC_ICRH, (uint32_t)apic_id << 24);
    lapic_write(LAPIC_ICRL, 0x00004500); // Level Assert, INIT

    hpet_sleep(10); // Wait 10ms

    // Send STARTUP IPI (Vector 0x08 -> 0x8000)
    lapic_write(LAPIC_ICRH, (uint32_t)apic_id << 24);
    lapic_write(LAPIC_ICRL, 0x00004608); // Start-up, Vector 0x08

    hpet_sleep(1);
}

void smp_init(void) {
    uint32_t cpu_count = acpi_get_cpu_count();
    uint8_t bsp_id = lapic_get_id();

    kprintf("SMP: Detected %d CPUs. BSP ID is %d\n", cpu_count, bsp_id);

    for (uint32_t i = 0; i < cpu_count; i++) {
        uint8_t id = acpi_get_cpu_apic_id(i);
        if (id != bsp_id) {
            uint32_t count_before = g_cpus_online;
            smp_boot_cpu(id);
            
            // Wait for CPU to come online (max 1 second)
            int timeout = 1000;
            while (g_cpus_online == count_before && timeout--) {
                hpet_sleep(1);
            }
        }
    }

    kprintf("SMP: %d CPUs online total.\n", g_cpus_online);
}
