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

// Helper to check if a point is within a circle
static inline int is_in_circle(int x, int y, int r) {
    return (x*x + y*y) <= (r*r);
}

void gfx_fill_rounded_rect_to_buffer(uint32_t* buf, int x, int y, int w, int h, int r, int stride, int buf_h, uint32_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int px = x + i;
            int py = y + j;
            if (px < 0 || px >= stride || py < 0 || py >= buf_h) continue;

            int draw = 1;
            // Top-left
            if (i < r && j < r) {
                if (!is_in_circle(i - r, j - r, r)) draw = 0;
            }
            // Top-right
            else if (i >= w - r && j < r) {
                if (!is_in_circle(i - (w - r - 1), j - r, r)) draw = 0;
            }
            // Bottom-left
            else if (i < r && j >= h - r) {
                if (!is_in_circle(i - r, j - (h - r - 1), r)) draw = 0;
            }
            // Bottom-right
            else if (i >= w - r && j >= h - r) {
                if (!is_in_circle(i - (w - r - 1), j - (h - r - 1), r)) draw = 0;
            }

            if (draw) {
                // Alpha blending (simple)
                uint8_t alpha = (color >> 24) & 0xFF;
                if (alpha == 0xFF) {
                    buf[py * stride + px] = color;
                } else if (alpha > 0) {
                    uint32_t bg = buf[py * stride + px];
                    uint8_t r_b = (bg >> 16) & 0xFF, g_b = (bg >> 8) & 0xFF, b_b = bg & 0xFF;
                    uint8_t r_f = (color >> 16) & 0xFF, g_f = (color >> 8) & 0xFF, b_f = color & 0xFF;
                    uint8_t r_r = (r_f * alpha + r_b * (255 - alpha)) / 255;
                    uint8_t g_r = (g_f * alpha + g_b * (255 - alpha)) / 255;
                    uint8_t b_r = (b_f * alpha + b_b * (255 - alpha)) / 255;
                    buf[py * stride + px] = (0xFF << 24) | (r_r << 16) | (g_r << 8) | b_r;
                }
            }
        }
    }
}

void gfx_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    gfx_fill_rounded_rect_to_buffer(vesa_video_memory, x, y, w, h, r, vesa_width, vesa_height, color);
}

void gfx_draw_gradient(int x, int y, int w, int h, uint32_t color1, uint32_t color2, int vertical) {
    uint8_t r1 = (color1 >> 16) & 0xFF, g1 = (color1 >> 8) & 0xFF, b1 = color1 & 0xFF;
    uint8_t r2 = (color2 >> 16) & 0xFF, g2 = (color2 >> 8) & 0xFF, b2 = color2 & 0xFF;

    if (vertical) {
        for (int j = 0; j < h; j++) {
            uint8_t r = r1 + (r2 - r1) * j / h;
            uint8_t g = g1 + (g2 - g1) * j / h;
            uint8_t b = b1 + (b2 - b1) * j / h;
            uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
            vesa_fill_rect(x, y + j, w, 1, color);
        }
    } else {
        for (int i = 0; i < w; i++) {
            uint8_t r = r1 + (r2 - r1) * i / w;
            uint8_t g = g1 + (g2 - g1) * i / w;
            uint8_t b = b1 + (b2 - b1) * i / w;
            uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
            vesa_fill_rect(x + i, y, 1, h, color);
        }
    }
}

void gfx_draw_gradient_to_buffer(uint32_t* buf, int x, int y, int w, int h, int stride, int height, uint32_t color1, uint32_t color2, int vertical) {
    uint8_t r1 = (color1 >> 16) & 0xFF, g1 = (color1 >> 8) & 0xFF, b1 = color1 & 0xFF;
    uint8_t r2 = (color2 >> 16) & 0xFF, g2 = (color2 >> 8) & 0xFF, b2 = color2 & 0xFF;

    if (vertical) {
        for (int j = 0; j < h; j++) {
            int py = y + j;
            if (py >= height) break;
            uint8_t r = r1 + (r2 - r1) * j / h;
            uint8_t g = g1 + (g2 - g1) * j / h;
            uint8_t b = b1 + (b2 - b1) * j / h;
            uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
            
            for (int i = 0; i < w; i++) {
                int px = x + i;
                if (px < stride) {
                    buf[py * stride + px] = color;
                }
            }
        }
    } else {
        // Horizontal gradient implementation if needed
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px >= stride) break;
            uint8_t r = r1 + (r2 - r1) * i / w;
            uint8_t g = g1 + (g2 - g1) * i / w;
            uint8_t b = b1 + (b2 - b1) * i / w;
            uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;

            for (int j = 0; j < h; j++) {
                int py = y + j;
                if (py < height) {
                    buf[py * stride + px] = color;
                }
            }
        }
    }
}
