.set MULTIBOOT_MAGIC,  0x1BADB002
.set MULTIBOOT_FLAGS,  0x0
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    /* Multiboot header: must be in first 8KiB and in a loadable section */
    .section .multiboot, "a"
    .align 4
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAGS
    .long MULTIBOOT_CHECKSUM

    .section .text
    .global _start
    .extern kernel_main

_start:
    cli
    mov $stack_top, %esp
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

    .section .bss
    .align 16
stack_bottom:
    .skip 16384
stack_top: