#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

/* Colors (ARGB) */
#define COLOR_BLACK 0xFF000000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLUE  0xFF0000FF
#define COLOR_RED   0xFFFF0000
#define COLOR_GREEN 0xFF00FF00
#define COLOR_GRAY  0xFF808080
#define COLOR_LIGHTGRAY 0xFFC0C0C0

/* Nebula Dark Palette (Modern/Clean) */
#define NEBULA_BG_TOP       0xFF0F0C29  /* Deep space navy */
#define NEBULA_BG_BOTTOM    0xFF302B63  /* Galaxy purple */
#define NEBULA_GLASS        0x99111111  /* Translucent dark glass */
#define NEBULA_ACCENT       0xFF00D2FF  /* Cyber blue pulse */
#define NEBULA_TEXT         0xFFE0E0E0  /* Off-white soft text */
#define NEBULA_TITLE_BAR    0xEE202020  /* Dark title bar */

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gfx_draw_char(int x, int y, char c, uint32_t color);
void gfx_draw_string(int x, int y, const char* str, uint32_t color);

/* Modern UI Elements */
void gfx_draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void gfx_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void gfx_draw_gradient(int x, int y, int w, int h, uint32_t color1, uint32_t color2, int vertical);
void gfx_draw_gradient_to_buffer(uint32_t* buf, int x, int y, int w, int h, int stride, int height, uint32_t color1, uint32_t color2, int vertical);

/* Rendering to a specific buffer (e.g. backbuffer or window surface) */
void gfx_draw_char_to_buffer(uint32_t* buf, int x, int y, int stride, int height, char c, uint32_t color);
void gfx_draw_string_to_buffer(uint32_t* buf, int x, int y, int stride, int height, const char* str, uint32_t color);
void gfx_fill_rect_to_buffer(uint32_t* buf, int x, int y, int w, int h, int stride, int buf_h, uint32_t color);
void gfx_fill_rounded_rect_to_buffer(uint32_t* buf, int x, int y, int w, int h, int r, int stride, int buf_h, uint32_t color);

#endif
