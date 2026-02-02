#ifndef APIC_H
#define APIC_H

#include <stdint.h>

/* LAPIC Register Offsets */
#define LAPIC_ID            0x20
#define LAPIC_VER           0x30
#define LAPIC_TPR           0x80
#define LAPIC_EOI           0xB0
#define LAPIC_LDR           0xD0
#define LAPIC_DFR           0xE0
#define LAPIC_SVR           0xF0
#define LAPIC_ESR           0x280
#define LAPIC_ICRL          0x300
#define LAPIC_ICRH          0x310
#define LAPIC_LVT_TMR       0x320
#define LAPIC_LVT_PERF      0x340
#define LAPIC_LVT_LINT0     0x350
#define LAPIC_LVT_LINT1     0x360
#define LAPIC_LVT_ERR       0x370
#define LAPIC_TMRINIT       0x380
#define LAPIC_TMRCURR       0x390
#define LAPIC_TMRDIV        0x3E0

void lapic_init(void);
void lapic_eoi(void);
uint32_t lapic_get_id(void);
void lapic_write(uint32_t reg, uint32_t val);
uint32_t lapic_read(uint32_t reg);

void ioapic_init(void);
void ioapic_set_irq(uint8_t irq, uint32_t apic_id, uint32_t flags_vector);

#endif
