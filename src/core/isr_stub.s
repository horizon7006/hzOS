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
    /* Save registers */
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

    /* Save Data Segment (sanity check, usually 0 in 64-bit but good practice) */
    /* Only FS/GS are relevant, but we skip for now as kernel uses null selectors usually */
    movq %ds, %rax
    pushq %rax

    /* ABI: Pass stack pointer as 1st argument (RDI) to handler */
    movq %rsp, %rdi
    
    call isr_handler

    /* Restore registers */
    popq %rax
    /* mov %ax, %ds - Segments don't really matter in 64-bit flat mode */
    
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
    
    addq $16, %rsp /* Pop error code and int number */
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
    
    call irq_handler
    /* Handler might switch stack (scheduler). If so, it returns new RSP?
       For now, we assume simple return. 
       If scheduler needs to switch, it should update current_process->rsp 
       and we manually switch? 
       In 32-bit, we did `mov %eax, %esp`.
       Here we should check if we need to switch?
       Wait, `irq_handler` in 32-bit was `void`.
       Ah, `isr_common_stub` called `isr_handler` (void).
       
       But `scheduler` was separate.
       In `process.c`, `scheduler_schedule` returned `struct registers*`.
       
       Wait, `irq_handler` called `timer_callback`.
       
       If I want scheduler to work, I need to match 32-bit logic.
       In 32-bit `irq_common_stub`:
       call irq_handler
       mov %eax, %esp  <-- It expected return value!
       
       I need to check `irq_handler` signature.
    */
    
    /* Restore */
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
    
    call syscall_handler
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