#include "gdt.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr   gp;

extern void gdt_flush(uint64_t);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

void gdt_flush(uint64_t);
 
void gdt_ap_load(void) {
    gdt_flush((uint64_t)&gp);
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (uint64_t)&gdt;

    // Null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // Code segment: base=0, limit=0, Access=0x9A, Gran=0xAF (L-bit set, D-bit clear)
    // Access: Present, Ring0, Code, Exec/Read
    // Granularity: 4KB, Long Mode (Bit 21 of high dword = Bit 5 of gran byte)
    // Old: 0xCF (1100 1111) -> 32-bit Protected
    // New: 0xAF (1010 1111) -> 64-bit Long Mode (L=1, D=0)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);

    // Data segment: base=0, limit=0, Access=0x92, Gran=0xCF
    // Do we need L-bit for data? No.
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_flush((uint64_t)&gp);
}