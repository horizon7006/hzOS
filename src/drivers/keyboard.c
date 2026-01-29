#include "keyboard.h"
#include "io.h"
#include "isr.h"
#include "terminal.h"
#include "command.h"

static char keymap[128] = {
    0,  27, '1','2','3','4','5','6',
    '7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u',
    'i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k',
    'l',';','\'','`', 0, '\\','z','x',
    'c','v','b','n','m',',','.','/',
    0,   0,   0,   ' ', 0, 0, 0, 0,
    /* rest zero */
};

#define KEYBOARD_BUFFER_SIZE 128
static char input_buffer[KEYBOARD_BUFFER_SIZE];
static int input_len = 0;

static void keyboard_handle_command(void);
/* External hook for GUI */
void wm_handle_key(char c);

// Callback pointer
static void (*g_kb_callback)(char) = 0;

void keyboard_set_callback(void (*cb)(char)) {
    g_kb_callback = cb;
}

static void keyboard_irq_handler(struct registers* regs) {
    UNUSED(regs);
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        return;
    }

    char c = keymap[scancode];
    if (!c) return;

    if (g_kb_callback) {
        g_kb_callback(c);
        // If we consume it, return? 
        // Or if we want shell echo logic inside driver (bad)?
        // Driver currently has shell echo logic!
        // If callback is set, we skip driver's built-in shell logic (echo, buffer).
        return; 
    }

    /* Hook for GUI */
    wm_handle_key(c);

    if (c == '\n') {
        terminal_putc('\n');
        input_buffer[input_len] = '\0';
        keyboard_handle_command();
        input_len = 0;
        terminal_write("> ");
        return;
    } else if (c == '\b') {
        if (input_len > 0) {
            input_len--;
            terminal_putc('\b');
        }
        return;
    }
    else {
        if (input_len < KEYBOARD_BUFFER_SIZE - 1) {
            input_buffer[input_len++] = c;
            terminal_putc(c);
        }
    }
}

static void keyboard_handle_command(void) {
    if (input_len == 0) return;
    input_buffer[input_len] = '\0';
    command_execute(input_buffer);
}

void keyboard_init(void) {
    irq_register_handler(1, keyboard_irq_handler);
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1);
    outb(0x21, mask);
    terminal_write("> ");
}

void keyboard_poll(void) {

}