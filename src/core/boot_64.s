.section .text
.code32
.global long_mode_start
.extern kernel_main

long_mode_start:
    /* 1. Check for Long Mode support */
    call check_cpuid
    call check_long_mode

    /* 2. Setup Paging (Identity Map first 4MB) */
    call setup_page_tables
    call enable_paging

    /* DEBUG: Spin here to check if paging enabled without crash */
    /* jmp . */

    /* 3. Load 64-bit GDT */
    lgdt gdt64_pointer

    /* 4. Jump to 64-bit code segment */
    push $0x08
    push $long_mode_entry
    lret

check_cpuid:
    pushfl
    pop %eax
    mov %eax, %ecx
    xor $1 << 21, %eax
    push %eax
    popfl
    pushfl
    pop %eax
    push %ecx
    popfl
    xor %ecx, %eax
    jz .no_cpuid
    ret
.no_cpuid:
    hlt

check_long_mode:
    mov $0x80000000, %eax
    cpuid
    cmp $0x80000001, %eax
    jb .no_long_mode

    mov $0x80000001, %eax
    cpuid
    test $1 << 29, %edx
    jz .no_long_mode
    ret
.no_long_mode:
    hlt

setup_page_tables:
    /* Clear L4, L3, and all four L2 tables */
    mov $page_table_l4, %edi
    xor %eax, %eax
    mov $6, %ecx /* L4, L3, L2*4 = 6 pages */
    shl $10, %ecx /* 6 * 1024 = 6144 longs */
    rep stosl

    /* Link L4[0] -> L3 */
    mov $page_table_l3, %eax
    or $0x3, %eax /* Present + Writable */
    mov %eax, page_table_l4

    /* Link L3[0..3] -> L2_0..L2_3 to cover 4GB */
    mov $page_table_l2, %eax
    or $0x3, %eax
    mov %eax, page_table_l3
    add $4096, %eax
    mov %eax, page_table_l3+8
    add $4096, %eax
    mov %eax, page_table_l3+16
    add $4096, %eax
    mov %eax, page_table_l3+24

    /* Identity map first 4GB using 2MB huge pages in L2s */
    mov $0, %ecx /* Current physical address */
    mov $page_table_l2, %edi
    mov $2048, %esi /* 512 entries * 4 tables = 2048 entries */

.map_p2_table:
    mov %ecx, %eax
    or $0x83, %eax /* Present + Writable + Huge */
    mov %eax, (%edi)
    add $8, %edi
    add $0x200000, %ecx
    dec %esi
    jnz .map_p2_table
    
    ret

enable_paging:
    /* Load P4 to CR3 */
    mov $page_table_l4, %eax
    mov %eax, %cr3

    /* Enable PAE */
    mov %cr4, %eax
    or $1 << 5, %eax
    mov %eax, %cr4

    /* Switch to Long Mode (EFER.LME) */
    mov $0xC0000080, %ecx
    rdmsr
    or $1 << 8, %eax
    wrmsr

    /* Enable Paging */
    mov %cr0, %eax
    or $1 << 31, %eax
    mov %eax, %cr0
    ret

/* 64-bit Entry Point */
.code64
.align 16
long_mode_entry:
    /* Clear Data Segments */
    xor %ax, %ax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    /* Setup 64-bit Stack */
    mov $stack64_top, %rsp
    
    /* Ensure 16-byte alignment */
    and $0xFFFFFFFFFFFFFFF0, %rsp
    
    /* Pass Multiboot Info (restore from Safe Mode) */
    mov multiboot_magic, %edi
    mov multiboot_info_ptr, %esi

    /* Jump to Kernel Main (No Return) */
    mov $kernel_main, %rax
    jmp *%rax
    hlt

.section .bss
.align 16
stack64_bottom:
    .skip 16384
stack64_top:
    
.section .bss
.align 4096
.global page_table_l4
page_table_l4:
    .skip 4096
page_table_l3:
    .skip 4096
page_table_l2:
    .skip 4096 * 4

.section .rodata
gdt64:
    .quad 0 /* Null */
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53) | (1<<41) /* Code: Present, Code, Readable, Long Mode */
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<41) /* Data: Present, Writeable */
gdt64_pointer:
    .word . - gdt64 - 1
    .long gdt64
