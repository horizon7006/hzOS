#ifndef VESA_H
#define VESA_H

#include <stdint.h>
#include "multiboot.h"

/* VESA Global Graphics State */
extern uint32_t* vesa_video_memory;
extern int vesa_width;
extern int vesa_height;
extern int vesa_pitch;
extern int vesa_bpp;

void vesa_init(multiboot_info_t* mbi);
void vesa_put_pixel(int x, int y, uint32_t color);
void vesa_fill_rect(int x, int y, int w, int h, uint32_t color);
void vesa_clear(uint32_t color);

#endif
