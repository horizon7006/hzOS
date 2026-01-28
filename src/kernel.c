#include "common.h"
#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "filesystem.h"
#include "command.h"
#include "printf.h"
#include "ramdisk.h"
#include "ext2.h"
#include "mouse.h"

extern uint8_t _binary_ext2_img_start[];
extern uint8_t _binary_ext2_img_end[];

void kernel_main(void) {
    gdt_init();
    terminal_init();
    terminal_writeln("hzOS: starting up...");

    fs_init();
    command_init();

    /* Initialize ramdisk backed by embedded ext2 image, if present. */
    uint32_t rd_size = (uint32_t)(_binary_ext2_img_end - _binary_ext2_img_start);
    if (rd_size > 0) {
        ramdisk_init(_binary_ext2_img_start, rd_size);
        if (ext2_mount_from_ramdisk() == 0) {
            ext2_print_super();
        }
    }

    idt_init();
    terminal_writeln("IDT installed. Interrupts enabled.");

    __asm__ __volatile__("sti");

    keyboard_init();
    mouse_init();
    terminal_writeln("Input ready. Type 'help' and press Enter.");

    // main loop: just halt until next interrupt
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}