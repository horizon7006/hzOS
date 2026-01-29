#include "vesa.h"
#include "multiboot.h"
#include "serial.h"

uint32_t* vesa_video_memory = 0;
int vesa_width = 0;
int vesa_height = 0;
int vesa_pitch = 0;
int vesa_bpp = 0;

void vesa_init(multiboot_info_t* mbi) {
    if (!mbi) {
        serial_printf("VESA: MBI is NULL\n");
        return;
    }

    /* Check if MULTIBOOT_VIDEO_MODE flag (bit 11 in vbe, 12 in flags) */
    if (mbi->flags & (1 << 12)) {
        vesa_video_memory = (uint32_t*)(uintptr_t)mbi->framebuffer_addr;
        vesa_width = mbi->framebuffer_width;
        vesa_height = mbi->framebuffer_height;
        vesa_pitch = mbi->framebuffer_pitch;
        vesa_bpp = mbi->framebuffer_bpp;

        serial_printf("VESA: %dx%dx%d at %x P=%d\n", vesa_width, vesa_height, vesa_bpp, vesa_video_memory, vesa_pitch);
        
        /* Immediate visual feedback: Blue Screen */
        vesa_clear(0xFF0000FF);
    } else {
        serial_printf("VESA: Multiboot framebuffer info not present. Flags=%x\n", mbi->flags);
    }
}

void vesa_put_pixel(int x, int y, uint32_t color) {
    if (!vesa_video_memory) return;
    if (x < 0 || x >= vesa_width || y < 0 || y >= vesa_height) return;

    /* Pitch is usually in bytes, but we are accessing as uint32_t* if 32bpp.
       Wait, pitch is bytes per line.
       offset = y * pitch + x * (bpp/8)
    */
    
    // Assuming 32 bpp for now based on request
    if (vesa_bpp == 32) {
        // Pointer arithmetic: video_memory is uint32_t*
        // But pitch is in bytes.
        // address = base + y * pitch + x * 4
        uint8_t* pixel_addr = (uint8_t*)vesa_video_memory + y * vesa_pitch + x * 4;
        *(uint32_t*)pixel_addr = color;
    }
}

void vesa_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!vesa_video_memory) return;
    
    // Clip
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > vesa_width) w = vesa_width - x;
    if (y + h > vesa_height) h = vesa_height - y;
    
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        uint8_t* row_start = (uint8_t*)vesa_video_memory + (y + j) * vesa_pitch + x * 4;
        uint32_t* pixel = (uint32_t*)row_start;
        for (int i = 0; i < w; i++) {
            *pixel++ = color;
        }
    }
}

void vesa_clear(uint32_t color) {
    vesa_fill_rect(0, 0, vesa_width, vesa_height, color);
}
