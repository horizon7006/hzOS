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

/* Windows 7 Color Palette (Aero-ish) */
#define WIN7_BG_COLOR   0xFF1B58B8  /* Standard Win7 Desktop Blue */
#define WIN7_TASKBAR    0xDD000000  /* Transparent black */
#define WIN7_WINDOW_BG  0xFFF0F0F0
#define WIN7_TITLE_BG   0xFF5078A0  /* Active title bar */

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gfx_draw_char(int x, int y, char c, uint32_t color);
void gfx_draw_string(int x, int y, const char* str, uint32_t color);

/* Rendering to a specific buffer (e.g. backbuffer or window surface) */
void gfx_draw_char_to_buffer(uint32_t* buf, int x, int y, int stride, int height, char c, uint32_t color);
void gfx_draw_string_to_buffer(uint32_t* buf, int x, int y, int stride, int height, const char* str, uint32_t color);
void gfx_fill_rect_to_buffer(uint32_t* buf, int x, int y, int w, int h, int stride, int buf_h, uint32_t color);

#endif
