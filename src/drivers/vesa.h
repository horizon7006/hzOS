#ifndef VESA_H
#define VESA_H

#include <stdint.h>
#include "../core/limine.h"

/* VESA Global Graphics State */
extern uint32_t* vesa_video_memory;
extern int vesa_width;
extern int vesa_height;
extern int vesa_pitch;
extern int vesa_bpp;

struct limine_framebuffer;
void vesa_init_limine(struct limine_framebuffer* fb);
void vesa_put_pixel(int x, int y, uint32_t color);
void vesa_fill_rect(int x, int y, int w, int h, uint32_t color);
void vesa_clear(uint32_t color);

#endif
