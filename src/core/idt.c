#include "idt.h"
#include "isr.h"

/* 64-bit IDT Entry (16 bytes) */
struct idt_entry {
    uint16_t base_low;   // Offset 0-15
    uint16_t sel;        // Segment Selector
    uint8_t  ist;        // Interrupt Stack Table (0 for now)
    uint8_t  flags;      // Type and Attributes
    uint16_t base_mid;   // Offset 16-31
    uint32_t base_high;  // Offset 32-63
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void idt_load(uint64_t); // Updated to take 64-bit pointer address (passed in RDI)

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_mid  = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    
    idt[num].sel       = sel;
    idt[num].ist       = 0;
    idt[num].flags     = flags;
    idt[num].reserved  = 0;
}

void idt_ap_load(void) {
    idt_load((uint64_t)&idtp);
}

void idt_init(void) {
    idtp.limit = sizeof(struct idt_entry) * 256 - 1;
    idtp.base  = (uint64_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Install handlers (implemented in isr.c using calls to idt_set_gate)
    isr_install();
    irq_install();

    idt_load((uint64_t)&idtp);
}