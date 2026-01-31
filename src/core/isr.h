#ifndef ISR_H
#define ISR_H

#include "common.h"

struct registers {
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void isr_install(void);
void irq_install(void);

typedef void (*irq_handler_t)(struct registers* regs);
void irq_register_handler(int irq, irq_handler_t handler);

#endif