#include "keyboard.h"
#include "../core/io.h"
#include "isr.h"
#include "terminal.h"
#include "command.h"

// Scancode definitions
#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_CAPSLOCK     0x3A
#define SC_NUMLOCK      0x45

// Modifier states
static int shift_pressed = 0;
static int caps_lock = 0;
static int num_lock = 1; // NumLock typically starts ON
static int extended_scancode = 0;
static kb_layout_t current_layout = KB_LAYOUT_US;

// Keymap for US (unshifted) state
static const char keymap_us[128] = {
    0,  27, '1','2','3','4','5','6',
    '7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u',
    'i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k',
    'l',';','\'','`', 0, '\\','z','x',
    'c','v','b','n','m',',','.','/',
    0,   '*',   0,   ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7',
    '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',
};

static const char keymap_us_shift[128] = {
    0,  27, '!','@','#','$','%','^',
    '&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U',
    'I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K',
    'L',':','"','~', 0, '|','Z','X',
    'C','V','B','N','M','<','>','?',
    0,   '*',   0,   ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7',
    '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',
};

// Keymap for TR (unshifted) state
static const char keymap_tr[128] = {
    0,  27, '1','2','3','4','5','6',
    '7','8','9','0','*','-', '\b',
    '\t','q','w','e','r','t','y','u',
    'i','o','p','\xf0','\xfc','\n', 0, // f0=g, fc=u
    'a','s','d','f','g','h','j','k',
    'l','\xfe','i',',', 0, '<','z','x', // fe=s
    'c','v','b','n','m','\xf6','\xe7','.', // f6=o, e7=c
    0,   '*',   0,   ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7',
    '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',
};

static const char keymap_tr_shift[128] = {
    0,  27, '!','"', 0,'$','%','&',
    '/','(',')','=','?','_', '\b',
    '\t','Q','W','E','R','T','Y','U',
    'I','O','P','\xd0','\xdc','\n', 0,
    'A','S','D','F','G','H','J','K',
    'L','\xde','\xdd',';', 0, '>','Z','X',
    'C','V','B','N','M','\xd6','\xc7',':',
    0,   '*',   0,   ' ', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7',
    '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.',
};

#define KEYBOARD_BUFFER_SIZE 128
static char input_buffer[KEYBOARD_BUFFER_SIZE];
static int input_len = 0;

static void keyboard_handle_command(void);
void wm_handle_key(char c);

static void (*g_kb_callback)(char) = 0;

void keyboard_set_callback(void (*cb)(char)) {
    g_kb_callback = cb;
}

/*
static void keyboard_set_leds(void) {
    // Set keyboard LEDs (bit 0 = ScrollLock, bit 1 = NumLock, bit 2 = CapsLock)
    uint8_t led_state = (caps_lock ? 0x04 : 0) | (num_lock ? 0x02 : 0);
    
    // Wait for keyboard to be ready
    while (inb(0x64) & 0x02);
    outb(0x60, 0xED); // LED command
    
    while (inb(0x64) & 0x02);
    outb(0x60, led_state);
}
*/

void keyboard_set_layout(kb_layout_t layout) {
    current_layout = layout;
}

kb_layout_t keyboard_get_layout(void) {
    return current_layout;
}

static void keyboard_irq_handler(struct registers* regs) {
    UNUSED(regs);
    uint8_t scancode = inb(0x60);

    // Handle extended scancode prefix
    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    // Handle key release (bit 7 set)
    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        if (key == SC_LSHIFT || key == SC_RSHIFT) {
            shift_pressed = 0;
        }
        extended_scancode = 0;
        return;
    }

    // Handle key press
    if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) {
        shift_pressed = 1;
        extended_scancode = 0;
        return;
    }
    
    if (scancode == SC_CAPSLOCK) {
        caps_lock = !caps_lock;
        extended_scancode = 0;
        return;
    }
    
    if (scancode == SC_NUMLOCK) {
        num_lock = !num_lock;
        extended_scancode = 0;
        return;
    }

    char c = 0;
    if (extended_scancode) {
        // Handle specific extended keys
        if (scancode == 0x35) { // Numpad /
            c = '/';
        }
        extended_scancode = 0;
    } else {
        // Get character based on layout and shift state
        if (current_layout == KB_LAYOUT_TR) {
            c = shift_pressed ? keymap_tr_shift[scancode] : keymap_tr[scancode];
        } else {
            c = shift_pressed ? keymap_us_shift[scancode] : keymap_us[scancode];
        }
    }
    
    // Apply CapsLock to letters
    if (caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    if (!c) return;

    if (g_kb_callback) {
        g_kb_callback(c);
        return; 
    }

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
            // terminal_putc(c);
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
    
    extern void ioapic_set_irq(uint8_t irq, uint32_t apic_id, uint32_t flags_vector);
    ioapic_set_irq(1, 0, 33); // IRQ 1 -> Vector 33 (0x21)
    
    // Set initial LED state
    // keyboard_set_leds(); // Disabled to prevent hang
    
    terminal_write("> ");
}

void keyboard_poll(void) {

}
