#include "terminal.h"
#include "../drivers/vesa.h"
#include "../gui/font.h"

extern uint64_t g_hhdm_offset;
static uint16_t* VIDEO_MEMORY = 0; // Will be set in terminal_init
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static int terminal_row = 0;
static int terminal_column = 0;
static uint8_t terminal_color = 0x0F; 
static int vesa_enabled = 0;

static const int VESA_CHAR_W = 8;
static const int VESA_CHAR_H = 8;

#define TERM_BUF_SIZE 1024
static char term_buffer[TERM_BUF_SIZE];
static int term_buf_head = 0;
static int term_buf_tail = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint8_t)c;
}

static void terminal_put_at(char c, int x, int y) {
    VIDEO_MEMORY[y * VGA_WIDTH + x] = vga_entry(c, terminal_color);
}

static void terminal_scroll(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VIDEO_MEMORY[(y - 1) * VGA_WIDTH + x] = VIDEO_MEMORY[y * VGA_WIDTH + x];
        }
    }
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
    if (!VIDEO_MEMORY) return; // Not initialized yet
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VIDEO_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    
    // Skip VESA clear - framebuffer might not be properly mapped
    // if (vesa_video_memory) {
    //     vesa_clear(0x00000000); // Black background for boot log
    // }

    terminal_row = 0;
    terminal_column = 0;
}

static void vesa_draw_char(char c, int x, int y, uint32_t color) {
    if (!vesa_video_memory) return;
    uint8_t* glyph = font8x8_basic[(uint8_t)c];
    for (int cy = 0; cy < 8; cy++) {
        for (int cx = 0; cx < 8; cx++) {
            if (glyph[cy] & (1 << cx)) {
                vesa_put_pixel(x + cx, y + cy, color);
            }
        }
    }
}

static void vesa_scroll(void) {
    if (!vesa_video_memory) return;
    
    // MMIO reads (memcpy from VRAM) are slow and can trigger QEMU lock contention.
    // For boot logging, we'll just clear and reset to top for now.
    vesa_clear(0x00000000);
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_init(void) {
    VIDEO_MEMORY = (uint16_t*)(0xB8000 + g_hhdm_offset);
    terminal_setcolor(0x0F);
    terminal_clear();
}

static term_sink_t terminal_sink = 0;

void terminal_set_sink(term_sink_t sink) {
    terminal_sink = sink;
}

void terminal_putc(char c) {
    if (terminal_sink) {
        terminal_sink(c);
    }

    // Capture in buffer for userland Terminal app
    int next = (term_buf_head + 1) % TERM_BUF_SIZE;
    if (next != term_buf_tail) {
        term_buffer[term_buf_head] = c;
        term_buf_head = next;
    }

    if (terminal_sink) return;

    // VESA Boot Console Support
    if (vesa_video_memory) {
        int max_cols = vesa_width / VESA_CHAR_W;
        int max_rows = vesa_height / VESA_CHAR_H;

        if (c == '\n') {
            terminal_column = 0;
            terminal_row++;
        } else if (c == '\r') {
            terminal_column = 0;
        } else {
            vesa_draw_char(c, terminal_column * VESA_CHAR_W, terminal_row * VESA_CHAR_H, 0xFFFFFFFF);
            terminal_column++;
            if (terminal_column >= max_cols) {
                terminal_column = 0;
                terminal_row++;
            }
        }

        if (terminal_row >= max_rows) {
            vesa_scroll();
        }
        return;
    }

    // Original VGA Fallback
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

int terminal_get_char(void) {
    if (term_buf_head == term_buf_tail) return -1;
    char c = term_buffer[term_buf_tail];
    term_buf_tail = (term_buf_tail + 1) % TERM_BUF_SIZE;
    return (int)c;
}