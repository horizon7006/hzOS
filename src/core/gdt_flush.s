.section .text
.code64
.global gdt_flush

gdt_flush:
    /* Arg1 (RDI) = Pointer to GDT Pointer structure */
    lgdt (%rdi)

    /* Reload Data Segments */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Reload CS using Far Return (push stack and lretq) */
    pushq $0x08         /* Push Code Segment */
    leaq .flush_label(%rip), %rax
    pushq %rax          /* Push Return Address (RIP) */
    lretq

.flush_label:
    ret