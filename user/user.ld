ENTRY(_start)

SECTIONS
{
    /* Userland binaries start at 0x01000000 (16MB) */
    . = 0x01000000;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
