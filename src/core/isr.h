#ifndef ISR_H
#define ISR_H

#include "common.h"

struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

void isr_install(void);
void irq_install(void);

typedef void (*irq_handler_t)(struct registers* regs);
void irq_register_handler(int irq, irq_handler_t handler);

#endif