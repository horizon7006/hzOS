.global gdt_flush

gdt_flush:
    mov 4(%esp), %eax    # argument: pointer to gdt_ptr
    lgdt (%eax)

    # Reload segment registers
    mov $0x10, %ax       # data segment selector (index 2 << 3)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Far jump to reload CS
    ljmp $0x08, $.flush_label

.flush_label:
    ret