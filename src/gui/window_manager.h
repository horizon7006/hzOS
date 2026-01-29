#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdint.h>
#include "mouse.h"

typedef struct window window_t;

typedef void (*paint_callback_t)(window_t* win, uint32_t* buf, int w, int h);
typedef void (*key_callback_t)(window_t* win, char key);

struct window {
    int x, y;
    int width, height;
    char title[32];
    uint32_t bg_color;
    struct window* next;
    
    /* Flags */
    // 0x1 = Minimized
    // 0x2 = Maximized (optional)
    int flags;
    
    /* Callbacks and User Data */
    void* user_data;
    paint_callback_t on_paint;
    key_callback_t on_key; 
};

/* Initialize Window Manager and allocate backbuffers */
void wm_init();

/* Create a new window */
window_t* wm_create_window(int x, int y, int w, int h, const char* title);

/* Update internal state, handle inputs */
void wm_update();

/* Render everything to screen */
void wm_draw();

#endif
