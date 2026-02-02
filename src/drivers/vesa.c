#include "vesa.h"
#include "multiboot.h"
#include "serial.h"

uint32_t* vesa_video_memory = 0;
int vesa_width = 0;
int vesa_height = 0;
int vesa_pitch = 0;
int vesa_bpp = 0;

void vesa_init_limine(struct limine_framebuffer* fb) {
    if (!fb) {
        serial_printf("VESA: Limine framebuffer is NULL\n");
        return;
    }

    // Limine should provide HHDM-mapped address, but verify
    // If address appears to be physical (below HHDM range), add offset
    extern uint64_t g_hhdm_offset;
    uintptr_t addr = (uintptr_t)fb->address;
    
    // HHDM addresses are typically > 0xffff800000000000 (higher half)
    // If address is low, it's physical and needs mapping
    if (addr < 0x100000000ULL) {
        // Address is below 4GB, likely physical - add HHDM offset
        addr += g_hhdm_offset;
        serial_printf("VESA: Applying HHDM offset to framebuffer\n");
    }
    
    vesa_video_memory = (uint32_t*)addr;
    vesa_width = fb->width;
    vesa_height = fb->height;
    vesa_pitch = fb->pitch;
    vesa_bpp = fb->bpp;

    serial_printf("VESA (Limine): %dx%dx%d at %p P=%d\n", vesa_width, vesa_height, vesa_bpp, (void*)vesa_video_memory, vesa_pitch);
    
    /* Immediate visual feedback: Blue Screen */
    vesa_clear(0xFF0000FF);
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
