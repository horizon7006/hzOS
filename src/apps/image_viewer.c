#include "image_viewer.h"
#include "../gui/window_manager.h"
#include "../gui/graphics.h"
#include "../lib/memory.h"
#include "../lib/upng.h"
#include "../fs/filesystem.h"

typedef struct {
    upng_t* upng;
    int width;
    int height;
} iv_state_t;

static void iv_paint(window_t* win, uint32_t* buf, int stride, int height) {
    (void)buf; (void)stride; (void)height;
    
    iv_state_t* state = (iv_state_t*)win->user_data;
    if (!state) return;
    
    // Background
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFF202020);
    
    if (!state->upng) {
        wm_draw_string(win, 10, 10, "Failed to load image.", 0xFFFF0000);
        return;
    }
    
    // Draw Image
    // Center it
    int img_w = state->width;
    int img_h = state->height;
    
    int x_off = (win->width - img_w) / 2;
    int y_off = (win->height - img_h) / 2;
    
    const unsigned char* pixels = upng_get_buffer(state->upng);
    unsigned bpp = upng_get_bpp(state->upng); // bits, e.g. 32 for RGBA8
    unsigned format = upng_get_format(state->upng);
    
    if (bpp != 32 || format != UPNG_RGBA8) {
        wm_draw_string(win, 10, 10, "Unsupported PNG format (only RGBA8)", 0xFFFF0000);
        return;
    }
    
    for (int y = 0; y < img_h; y++) {
        for (int x = 0; x < img_w; x++) {
             // Basic clipping
             if (x_off + x < 0 || x_off + x >= win->width) continue;
             if (y_off + y < 0 || y_off + y >= win->height) continue;
             
             // Get Pixel
             // RGBA8
             int idx = (y * img_w + x) * 4;
             uint8_t r = pixels[idx+0];
             uint8_t g = pixels[idx+1];
             uint8_t b = pixels[idx+2];
             uint8_t a = pixels[idx+3];
             
             // Simple Alpha Blending over dark background
             // (Can be improved to blend over existing buffer content, but this is simpler)
             // Bg is 0xFF202020
             // out = src * a + dst * (1-a)
             
             // Fast path: fully opaque
             if (a == 255) {
                 wm_draw_pixel(win, x_off + x, y_off + y, (0xFF << 24) | (r<<16) | (g<<8) | b);
             } else if (a > 0) {
                 uint32_t bg = 0xFF202020;
                 uint8_t br = (bg >> 16) & 0xFF;
                 uint8_t bg_g = (bg >> 8) & 0xFF;
                 uint8_t bb = bg & 0xFF;
                 
                 uint8_t or = (r * a + br * (255 - a)) / 255;
                 uint8_t og = (g * a + bg_g * (255 - a)) / 255;
                 uint8_t ob = (b * a + bb * (255 - a)) / 255;
                 
                 wm_draw_pixel(win, x_off + x, y_off + y, (0xFF << 24) | (or<<16) | (og<<8) | ob);
             }
        }
    }
}

void image_viewer_create(const char* path) {
    // Read file
    size_t fsize = 0;
    const char* fdata = fs_read(path, &fsize);
    if (!fdata) return;
    
    // Decode
    upng_t* upng = upng_new_from_bytes((const unsigned char*)fdata, (unsigned long)fsize);
    upng_decode(upng);
    
    int w = 300, h = 200;
    iv_state_t* state = (iv_state_t*)kmalloc_z(sizeof(iv_state_t));
    
    if (upng_get_error(upng) == UPNG_EOK) {
        state->upng = upng;
        state->width = upng_get_width(upng);
        state->height = upng_get_height(upng);
        w = state->width + 20;
        h = state->height + 40;
    } else {
        // failed
        upng_free(upng);
        state->upng = 0;
    }
    
    if (w > 600) w = 600;
    if (h > 400) h = 400;

    window_t* win = wm_create_window(100, 100, w, h, "Image Viewer");
    win->bg_color = 0xFF202020;
    win->user_data = state;
    win->on_paint = iv_paint;
}
