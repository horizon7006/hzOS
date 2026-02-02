#ifndef HPET_H
#define HPET_H

#include <stdint.h>

#define HPET_REG_GENERAL_CAPABILITIES      0x00
#define HPET_REG_GENERAL_CONFIGURATION     0x10
#define HPET_REG_GENERAL_INTERRUPT_STATUS  0x20
#define HPET_REG_MAIN_COUNTER_VALUE        0xF0

#define HPET_REG_TIMER_CONFIGURATION_RAW(n) (0x100 + 0x20 * n)
#define HPET_REG_TIMER_COMPARATOR_RAW(n)    (0x108 + 0x20 * n)
#define HPET_REG_TIMER_FSB_INTERRUPT_RAW(n) (0x110 + 0x20 * n)

void hpet_init(void);
void hpet_sleep(uint64_t ms);
uint64_t hpet_get_nanos(void);

#endif
