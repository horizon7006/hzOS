.section .text
.code64
.align 8

.global idt_load
.extern isr_handler
.extern irq_handler
.extern syscall_handler

/* Load IDT pointer */
idt_load:
    lidt (%rdi)
    ret

/* Macro for exception without error code */
.macro ISR_NOERR num
.global isr\num
isr\num:
    cli
    pushq $0          /* fake error code */
    pushq $\num       /* interrupt number */
    jmp isr_common_stub
.endm

/* Macro for exception with error code */
.macro ISR_ERR num
.global isr\num
isr\num:
    cli
    pushq $\num
    jmp isr_common_stub
.endm

/* Define ISRs */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

/* System Call ISR */
.global isr80
isr80:
    cli
    pushq $0
    pushq $80
    jmp syscall_common_stub

/* Macro for IRQ */
.macro IRQ num
.global irq\num
irq\num:
    cli
    pushq $0
    pushq $(32+\num)
    jmp irq_common_stub
.endm

/* Define IRQs */
IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

/* Common ISR Stub */
isr_common_stub:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %ds, %rax
    pushq %rax

    movq %rsp, %rdi
    
    /* Align stack to 16 bytes for C call */
    movq %rsp, %rax
    pushq %rax
    andq $-16, %rsp
    
    call isr_handler
    
    /* Restore stack */
    popq %rsp

    popq %rax
    /* mov %ax, %ds */
    
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    
    addq $16, %rsp
    iretq

/* Common IRQ Stub */
irq_common_stub:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %ds, %rax
    pushq %rax

    movq %rsp, %rdi
    
    /* Align stack to 16 bytes for C call */
    movq %rsp, %rax
    pushq %rax
    andq $-16, %rsp
    
    call irq_handler
    
    /* RAX now contains the (potentially new) registers pointer.
       We need to carefully restore the stack pointer.
       If task switching occurred, the returned RAX is the new RSP.
    */
    movq %rax, %rsp

    popq %rax
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    
    addq $16, %rsp
    iretq

/* Common Syscall Stub */
syscall_common_stub:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %ds, %rax
    pushq %rax

    movq %rsp, %rdi
    
    /* Align stack to 16 bytes for C call */
    movq %rsp, %rax
    pushq %rax
    andq $-16, %rsp
    
    call syscall_handler
    
    popq %rsp
    /* syscall_handler returned new ESP in 32-bit.
       Here it should return new RSP?
       
       Let's assume for now no task switching in syscall
    */
    
    popq %rax
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    
    addq $16, %rsp
    iretq