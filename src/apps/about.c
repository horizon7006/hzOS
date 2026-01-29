#include "about.h"
#include "../gui/window_manager.h"

static void about_paint(window_t* win, uint32_t* buf, int stride, int height) {
    (void)buf; (void)stride; (void)height;
    
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFFE0E0E0);
    
    int lx = 20, ly = 20, lw = 40, lh = 40;
    uint32_t l_col = 0xFF208020;
    
    wm_fill_rect(win, lx, ly, lw, 2, l_col);        
    wm_fill_rect(win, lx, ly+lh-2, lw, 2, l_col);   
    wm_fill_rect(win, lx, ly, 2, lh, l_col);        
    wm_fill_rect(win, lx+lw-2, ly, 2, lh, l_col);   
    
    wm_draw_string(win, lx+12, ly+16, "hz", l_col);
    
    wm_draw_string(win, 80, 20, "hzOS (horizonOS)", 0xFF000000);
    wm_draw_string(win, 80, 35, "Version: ALPHA A.5", 0xFF000000);
    wm_draw_string(win, 80, 50, "Kernel: 32-bit hzKernel", 0xFF000000);
    wm_draw_string(win, 80, 65, "Copyright (C) 2026", 0xFF000000);
    
    wm_draw_string(win, 20, 100, "A simple OS.", 0xFF404040);
}

void about_create() {
    window_t* win = wm_create_window(200, 200, 300, 150, "About hzOS");
    win->bg_color = 0xFFE0E0E0;
    win->on_paint = about_paint;
    win->on_key = 0;
}
