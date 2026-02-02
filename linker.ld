ENTRY(_start)

SECTIONS
{
    /* Use higher-half address for the kernel */
    . = 0xffffffff80000000;

    .text :
    {
        *(.text*)
    }

    /* Limine requests section */
    .requests :
    {
        KEEP(*(.requests_start))
        KEEP(*(.requests))
        KEEP(*(.requests_end))
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(.bss*)
        *(COMMON)
    }

    _end = .;
}