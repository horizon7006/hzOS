#include "graphics.h"
#include "vesa.h"
#include "font.h"

int abs(int v) { return v < 0 ? -v : v; }

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    vesa_fill_rect(x, y, w, h, color);
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
    vesa_fill_rect(x, y, w, 1, color);         // Top
    vesa_fill_rect(x, y + h - 1, w, 1, color); // Bottom
    vesa_fill_rect(x, y, 1, h, color);         // Left
    vesa_fill_rect(x + w - 1, y, 1, h, color); // Right
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        vesa_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_draw_char(int x, int y, char c, uint32_t color) {
    if (c < 0 || c > 127) return;
    const uint8_t* glyph = font8x8_basic[(int)c];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if ((bits >> (7 - col)) & 1) {
                vesa_put_pixel(x + col, y + row, color);
            }
        }
    }
}

void gfx_draw_string(int x, int y, const char* str, uint32_t color) {
    while (*str) {
        gfx_draw_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

void gfx_fill_rect_to_buffer(uint32_t* buf, int x, int y, int w, int h, int stride, int buf_h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > stride) w = stride - x;
    if (y + h > buf_h) h = buf_h - y;
    
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        uint32_t* row = buf + (y + j) * stride + x;
        for (int i = 0; i < w; i++) {
            *row++ = color;
        }
    }
}

void gfx_draw_char_to_buffer(uint32_t* buf, int x, int y, int stride, int height, char c, uint32_t color) {
    if (c < 0 || c > 127) return;
    const uint8_t* glyph = font8x8_basic[(int)c];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if ((bits >> (7 - col)) & 1) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < stride && py >= 0 && py < height) {
                    buf[py * stride + px] = color;
                }
            }
        }
    }
}

void gfx_draw_string_to_buffer(uint32_t* buf, int x, int y, int stride, int height, const char* str, uint32_t color) {
    while (*str) {
        gfx_draw_char_to_buffer(buf, x, y, stride, height, *str++, color);
        x += 8;
    }
}
