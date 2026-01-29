.set MULTIBOOT_PAGE_ALIGN,    1<<0
.set MULTIBOOT_MEMORY_INFO,   1<<1
.set MULTIBOOT_VIDEO_MODE,    1<<2

.set MULTIBOOT_MAGIC,  0x1BADB002
.set MULTIBOOT_FLAGS,  MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    /* Multiboot header: must be in first 8KiB and in a loadable section */
    .section .multiboot, "a"
    .align 4
    .long MULTIBOOT_MAGIC
    .long MULTIBOOT_FLAGS
    .long MULTIBOOT_CHECKSUM
    /* Address fields (ignored if bit 16 is 0) */
    .long 0, 0, 0, 0, 0
    /* Video mode request (bit 2 is 1) */
    .long 0    /* mode_type: 0 for linear graphics */
    .long 1024 /* width */
    .long 768  /* height */
    .long 32   /* depth */

    .section .text
    .global _start
    .extern kernel_main

_start:
    cli
    mov $stack_top, %esp
    push %ebx  /* Multiboot info structure address */
    push %eax  /* Multiboot magic value */
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