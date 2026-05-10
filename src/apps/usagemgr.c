#include "usagemgr.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

#define WIN_W 300
#define WIN_H 150

static void usagemgr_paint(window_t *win, uint32_t *buf, int stride, int height) {
    (void)buf;
    (void)stride;
    (void)height;
    
    // Background 
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFF202020);
    
    // Title
    wm_draw_string(win, 10, 10, "System Usage", 0xFFFFFFFF);
    
    // Fetch memory stats
    size_t total_kb = memory_get_total_kb();
    size_t used_kb = memory_get_used_kb();
    
    // Convert to MB for display
    size_t total_mb = total_kb / 1024;
    size_t used_mb = used_kb / 1024;
    
    // Safety against div by 0 and bound checks
    if (total_kb == 0) total_kb = 1; 
    if (used_kb > total_kb) used_kb = total_kb;
    
    // Ram Labels
    
    // Not using sprintf yet, manual conversion for now
    // A quick hack since we don't have a robust sprintf for userland strings yet
    // "RAM: X MB / Y MB"
    // For simplicity, we'll draw the string manually since kprintf prints to terminal. 
    // We already have string drawing. 
    // Let's implement a very simple num_to_str
    
    char num1[16];
    char num2[16];
    
    // Used MB
    int val = used_mb;
    int j = 0;
    if (val == 0) num1[j++] = '0';
    while (val > 0) {
        num1[j++] = '0' + (val % 10);
        val /= 10;
    }
    // Reverse
    for (int k = 0; k < j / 2; k++) {
        char tmp = num1[k];
        num1[k] = num1[j - 1 - k];
        num1[j - 1 - k] = tmp;
    }
    num1[j] = 0;
    
    // Total MB
    val = total_mb;
    j = 0;
    if (val == 0) num2[j++] = '0';
    while (val > 0) {
        num2[j++] = '0' + (val % 10);
        val /= 10;
    }
    for (int k = 0; k < j / 2; k++) {
        char tmp = num2[k];
        num2[k] = num2[j - 1 - k];
        num2[j - 1 - k] = tmp;
    }
    num2[j] = 0;

    wm_draw_string(win, 10, 40, "RAM Usage:", 0xFFAAAAAA);
    
    // Draw text values
    int cx = 100;
    char *p1 = num1;
    while (*p1) { char buf2[2] = {*p1++, 0}; wm_draw_string(win, cx, 40, buf2, 0xFFFFFFFF); cx += 8; }
    wm_draw_string(win, cx, 40, " MB / ", 0xFFFFFFFF); cx += 48;
    char *p2 = num2;
    while (*p2) { char buf2[2] = {*p2++, 0}; wm_draw_string(win, cx, 40, buf2, 0xFFFFFFFF); cx += 8; }
    wm_draw_string(win, cx, 40, " MB", 0xFFFFFFFF);
    
    // Draw visual bar
    int bar_x = 10;
    int bar_y = 60;
    int bar_w = WIN_W - 20;
    int bar_h = 24;
    
    // Bar Background
    wm_fill_rect(win, bar_x, bar_y, bar_w, bar_h, 0xFF404040);
    
    // Bar Foreground
    int fill_w = (int)((used_kb * bar_w) / total_kb);
    if (fill_w > bar_w) fill_w = bar_w;
    
    // Color depends on usage: Green -> Yellow -> Red
    uint32_t bar_color = 0xFF00DD55; // Green
    if (used_kb * 100 / total_kb > 75) bar_color = 0xFFDD0000; // Red
    else if (used_kb * 100 / total_kb > 50) bar_color = 0xFFDDDD00; // Yellow

    wm_fill_rect(win, bar_x, bar_y, fill_w, bar_h, bar_color);
    
    // Draw refresh hint
    wm_draw_string(win, 10, WIN_H - 24, "Hover to refresh...", 0xFF777777);
}

static void usagemgr_mouse_move(window_t *win, int x, int y) {
    (void)win;
    (void)x;
    (void)y;
    // Just force a repaint whenever mouse moves over window to update stats
    // A timer would be better, but we don't have userland timers yet.
}

void usagemgr_create(void) {
    window_t *win = wm_create_window(100, 100, WIN_W, WIN_H, "Usage Manager");
    if (!win) return;
    
    win->bg_color = 0xFF202020;
    win->on_paint = usagemgr_paint;
    win->on_click = usagemgr_mouse_move; // Trick: Use on_click to trigger repaints too if needed. 
    // We don't have on_mouse_move in window_t currently. Let's rely on global redraws for now.
}
