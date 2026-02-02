.section .text
.code64
.global _start
.extern kernel_main

_start:
    /* Disable interrupts */
    cli

    /* Setup a temporary stack for kernel_main */
    mov $stack_top, %rsp

    /* Linker might put us at ffffffff80000000, 
       ensure stack is also in higher half if we use it. 
       For now, we use identity mapping if Limine provides it 
       or we use Limine's HHDM. */

    /* Jump to kernel_main */
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

.section .bss
.align 16
stack_bottom:
    .skip 32768
stack_top: