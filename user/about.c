#include "libhz.h"

void redraw(hz_window_t* win) {
    hz_window_fill_rect(win, 0, 0, win->width, win->height, 0xFFFFFFFF);
    hz_window_draw_text(win, 20, 30, "hzOS (Horizon Operating System)", 0xFF000000);
    hz_window_draw_text(win, 20, 50, "Version: Beta Build 1000", 0xFF000000);
    hz_window_draw_text(win, 20, 80, "Developed by: Horizon7006", 0xFF000000);
    hz_window_draw_text(win, 20, 100, "Kernel: Custom 32-bit", 0xFF000000);
    hz_window_draw_text(win, 20, 150, "Click anywhere to close", 0xFFFF0000);
}

int main() {
    hz_window_t win;
    win.x = 200; win.y = 150; win.width = 320; win.height = 200;
    win.title = "About hzOS";
    
    hz_window_open(&win);
    redraw(&win);
    
    int running = 1;
    while (running) {
        hz_event_t ev;
        if (hz_event_poll(&win, &ev)) {
            if (ev.type == HZ_EVENT_QUIT || ev.type == HZ_EVENT_MOUSE_BUTTON) {
                running = 0;
            }
        }
    }
    return 0;
}
