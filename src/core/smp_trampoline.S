[bits 16]
[org 0x8000]

ap_trampoline:
    cli
    cld

    ; Load GDT
    lgdt [ap_gdt_ptr - ap_trampoline + 0x8000]

    ; Enable PM
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Jump to 32-bit
    jmp 0x08:(ap_pm_entry - ap_trampoline + 0x8000)

[bits 32]
ap_pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Here we need to transition to LONG MODE
    ; This requires pre-calculating the PML4 address and passing it
    
    ; 1. Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 2. Load PML4
    mov eax, [ap_pml4 - ap_trampoline + 0x8000]
    mov cr3, eax

    ; 3. Enable LME (Long Mode Enable) in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 4. Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; 5. Load 64-bit GDT and Jump
    lgdt [ap_gdt64_ptr - ap_trampoline + 0x8000]
    jmp 0x08:(ap_lm_entry - ap_trampoline + 0x8000)

[bits 64]
ap_lm_entry:
    ; We are in LONG MODE!
    ; Load segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup stack for this CPU
    mov rsp, [ap_stack - ap_trampoline + 0x8000]

    ; Call kernel_ap_main
    mov rax, [ap_entry_ptr - ap_trampoline + 0x8000]
    call rax

    ; Should never return
.halt:
    hlt
    jmp .halt

align 16
ap_gdt:
    dq 0x0000000000000000 ; Null
    dq 0x00CF9A000000FFFF ; Code 32
    dq 0x00CF92000000FFFF ; Data 32
ap_gdt_ptr:
    dw $ - ap_gdt - 1
    dd ap_gdt - ap_trampoline + 0x8000

align 16
ap_gdt64:
    dq 0x0000000000000000 ; Null
    dq 0x00AF9A000000FFFF ; Code 64
    dq 0x00AF92000000FFFF ; Data 64
ap_gdt64_ptr:
    dw $ - ap_gdt64 - 1
    dq ap_gdt64 - ap_trampoline + 0x8000

; Data provided by SMP initialization code
align 8
ap_pml4         dd 0
ap_stack        dq 0
ap_entry_ptr    dq 0

ap_trampoline_end:
