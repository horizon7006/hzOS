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

#include "vesa.h"
#include "multiboot.h"
#include "../gui/graphics.h"

extern uint8_t _binary_ext2_img_start[];
extern uint8_t _binary_ext2_img_end[];

/* Minimal VESA TTY state */
static int tty_x = 0;
static int tty_y = 0;
static char tty_line_buf[128];
static int tty_line_len = 0;

static void tty_sink(char c) {
    if (c == '\n') {
        tty_x = 0;
        tty_y += 10;
        if (tty_y >= vesa_height - 10) {
            tty_y = 0;
            gfx_fill_rect(0, 0, vesa_width, vesa_height, 0xFF000000);
        }
    } else if (c == '\b') {
        if (tty_x >= 8) {
            tty_x -= 8;
            gfx_fill_rect(tty_x, tty_y, 8, 8, 0xFF000000);
        }
    } else {
        gfx_draw_char(tty_x, tty_y, c, 0xFF00FF00); // Terminal Green
        tty_x += 8;
        if (tty_x >= vesa_width) {
            tty_x = 0;
            tty_y += 10;
        }
    }
}

static void tty_key_handler(char c) {
    // Determine if backspace
    if (c == '\b') {
        if (tty_line_len > 0) {
            tty_line_len--;
            tty_sink('\b');
        }
        return;
    }

    // Echo 
    tty_sink(c);
    
    // Buffer
    if (c == '\n') {
        tty_line_buf[tty_line_len] = 0;
        command_execute(tty_line_buf);
        tty_line_len = 0;
        kprintf("> ");
    } else {
        if (tty_line_len < 127) {
            tty_line_buf[tty_line_len++] = c;
        }
    }
}

void kernel_main(unsigned long magic, unsigned long addr) {
    serial_init();
    serial_printf("hzOS: Kernel Main. Magic=%x Addr=%x\n", magic, addr);

    gdt_init();
    terminal_init(); 
    
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        serial_printf("VESA: Initializing...\n");
        vesa_init((multiboot_info_t*)addr);
    }

    fs_init();
    command_init();

    idt_init();
    __asm__ __volatile__("sti");

    keyboard_init();
    mouse_init();

    serial_printf("Starting GUI...\n");
    gui_start();

    // GUI Returned - Switch to TTY Mode
    serial_printf("GUI Exited. Switching to TTY...\n");
    
    // Clear Screen
    gfx_fill_rect(0, 0, vesa_width, vesa_height, 0xFF000000);
    
    // Setup TTY Sink for kprintf
    terminal_set_sink(tty_sink);
    
    // Hook Keyboard
    keyboard_set_callback(tty_key_handler);
    
    kprintf("hzOS TTY Shell\n> ");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}