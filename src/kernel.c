#include "common.h"
#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "filesystem.h"
#include "command.h"
#include "printf.h"

void kernel_main(void) {
    gdt_init();
    terminal_init();
    terminal_writeln("hzOS: starting up...");

    fs_init();
    command_init();

    idt_init();
    terminal_writeln("IDT installed. Interrupts enabled.");

    __asm__ __volatile__("sti");

    keyboard_init();
    terminal_writeln("Keyboard ready. Type 'help' and press Enter.");

    // main loop: just halt until next interrupt
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}