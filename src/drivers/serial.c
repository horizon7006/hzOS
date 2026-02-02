#include "serial.h"
#include <stdarg.h>

/* I/O port wrappers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#define PORT 0x3f8   /* COM1 */

void serial_init() {
    outb(PORT + 1, 0x00);    // Disable all interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
    return inb(PORT + 5) & 0x20;
}

static int serial_lock = 0;

void serial_putc(char c) {
    if (serial_lock) return;
    serial_lock = 1;
    while (is_transmit_empty() == 0);
    outb(PORT, c);
    serial_lock = 0;
}

void serial_write(const char* str) {
    while (*str) {
        serial_putc(*str++);
    }
}

static void print_dec(int value) {
    char buf[16];
    int i = 0;
    int neg = 0;
    if (value == 0) { serial_putc('0'); return; }
    if (value < 0) { neg = 1; value = -value; }
    while (value > 0 && i < 16) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    if (neg) serial_putc('-');
    while (i-- > 0) serial_putc(buf[i]);
}

static void print_hex(uint32_t value) {
    const char* digits = "0123456789ABCDEF";
    serial_write("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (value >> i) & 0xF;
        serial_putc(digits[nibble]);
    }
}

static void print_hex64(uint64_t value) {
    const char* digits = "0123456789ABCDEF";
    serial_write("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (value >> i) & 0xF;
        serial_putc(digits[nibble]);
    }
}

void serial_printf(const char* fmt, ...) {
    static int internal_lock = 0;
    if (internal_lock) return;
    internal_lock = 1;

    va_list args;
    va_start(args, fmt);
    for (size_t i = 0; fmt[i] != 0; i++) {
        if (fmt[i] != '%') {
            serial_putc(fmt[i]);
            continue;
        }
        i++;
        // Check for 'l' modifier
        int is_long = 0;
        if (fmt[i] == 'l') {
            is_long = 1;
            i++;
        }
        switch (fmt[i]) {
            case 'c': serial_putc((char)va_arg(args, int)); break;
            case 's': serial_write(va_arg(args, const char*)); break;
            case 'd': print_dec(va_arg(args, int)); break;
            case 'u': {
                uint32_t v = va_arg(args, uint32_t);
                if (v == 0) serial_putc('0');
                else {
                    char buf[16]; int j = 0;
                    while (v > 0) { buf[j++] = '0' + (v % 10); v /= 10; }
                    while (j-- > 0) serial_putc(buf[j]);
                }
                break;
            }
            case 'x':
                if (is_long) {
                    print_hex64(va_arg(args, uint64_t));
                } else {
                    print_hex(va_arg(args, uint32_t));
                }
                break;
            case 'p': print_hex64(va_arg(args, uint64_t)); break;
            default: serial_putc('%'); if (is_long) serial_putc('l'); serial_putc(fmt[i]); break;
        }
    }
    va_end(args);
    internal_lock = 0;
}
