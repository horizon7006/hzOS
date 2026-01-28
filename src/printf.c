#include "printf.h"
#include "terminal.h"

#include <stdarg.h>

static void print_dec(int value) {
    char buf[16];
    int i = 0;
    int neg = 0;

    if (value == 0) {
        terminal_putc('0');
        return;
    }

    if (value < 0) {
        neg = 1;
        value = -value;
    }

    while (value > 0 && i < (int)sizeof(buf)) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (neg) {
        terminal_putc('-');
    }

    while (i-- > 0) {
        terminal_putc(buf[i]);
    }
}

static void print_hex(uint32_t value) {
    const char* digits = "0123456789ABCDEF";
    terminal_write("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (value >> i) & 0xF;
        terminal_putc(digits[nibble]);
    }
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (size_t i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            terminal_putc(fmt[i]);
            continue;
        }

        i++;
        char spec = fmt[i];
        switch (spec) {
            case 'c': {
                char c = (char)va_arg(args, int);
                terminal_putc(c);
                break;
            }
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                terminal_write(s);
                break;
            }
            case 'd':
            case 'i': {
                int v = va_arg(args, int);
                print_dec(v);
                break;
            }
            case 'x': {
                uint32_t v = va_arg(args, uint32_t);
                print_hex(v);
                break;
            }
            case '%': {
                terminal_putc('%');
                break;
            }
            default:
                // Unknown specifier, print it literally
                terminal_putc('%');
                terminal_putc(spec);
                break;
        }
    }

    va_end(args);
}

