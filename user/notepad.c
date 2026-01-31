#include "libhz.h"

#define MAX_TEXT 1024
char text[MAX_TEXT];
int text_len = 0;

void redraw(hz_window_t* win) {
    hz_window_fill_rect(win, 0, 0, win->width, win->height, 0xFFFFFFFF);
    hz_window_draw_text(win, 4, 4, text, 0xFF000000);
    int cursor_x = 4 + text_len * 8;
    hz_window_draw_text(win, cursor_x, 4, "_", 0xFF000000);
}

int main() {
    hz_window_t win;
    win.x = 100; win.y = 100; win.width = 400; win.height = 300;
    win.title = "Notepad (User)";
    
    hz_window_open(&win);
    text[0] = 0;
    redraw(&win);

    int running = 1;
    while (running) {
        hz_event_t ev;
        if (hz_event_poll(&win, &ev)) {
            if (ev.type == HZ_EVENT_QUIT) {
                running = 0;
            } else if (ev.type == HZ_EVENT_KEY) {
                if (ev.key == '\b') {
                    if (text_len > 0) text[--text_len] = 0;
                } else if (ev.key >= 32 && ev.key <= 126) {
                    if (text_len < MAX_TEXT - 1) {
                        text[text_len++] = ev.key;
                        text[text_len] = 0;
                    }
                }
                redraw(&win);
            }
        }
    }
    return 0;
}
