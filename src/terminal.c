#include "terminal.h"

static uint16_t* const VIDEO_MEMORY = (uint16_t*)0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static int terminal_row = 0;
static int terminal_column = 0;
static uint8_t terminal_color = 0x0F; // white on black

static inline uint16_t vga_entry(char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint8_t)c;
}

static void terminal_put_at(char c, int x, int y) {
    VIDEO_MEMORY[y * VGA_WIDTH + x] = vga_entry(c, terminal_color);
}

static void terminal_scroll(void) {
    // scroll everything up by one line
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VIDEO_MEMORY[(y - 1) * VGA_WIDTH + x] = VIDEO_MEMORY[y * VGA_WIDTH + x];
        }
    }
    // clear last line
    for (int x = 0; x < VGA_WIDTH; x++) {
        VIDEO_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
    if (terminal_row > 0) {
        terminal_row--;
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VIDEO_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_init(void) {
    terminal_setcolor(0x0F);
    terminal_clear();
}

void terminal_putc(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (c == '\r') {
        terminal_column = 0;
    } else if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
        }
        // erase the character at the new cursor position
        terminal_put_at(' ', terminal_column, terminal_row);
    } else {
        terminal_put_at(c, terminal_column, terminal_row);
        terminal_column++;
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
    }
}

void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putc(str[i]);
    }
}

void terminal_writeln(const char* str) {
    terminal_write(str);
    terminal_putc('\n');
}