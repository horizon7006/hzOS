.section .text
.global _start
.extern main

_start:
    # Entry point from kernel
    call main
    
    # Exit syscall
    mov %eax, %ebx # Save main's return value to EBX
    mov $1, %eax   # SYS_EXIT
    int $0x80
    
    # Return to kernel (process_execute caller)
    ret
