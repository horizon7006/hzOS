#include "notepad.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"
#include "../gui/graphics.h"

#define MAX_TEXT 1024

typedef struct {
    char text[MAX_TEXT];
    int len;
} notepad_state_t;

static void notepad_paint(window_t* win, uint32_t* buf, int stride, int height) {
    (void)buf; (void)stride; (void)height; 

    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFFFFFFFF);

    notepad_state_t* state = (notepad_state_t*)win->user_data;
    if (!state) return;

    wm_draw_string(win, 4, 4, state->text, 0xFF000000);
    
    int cursor_x = 4 + (state->len * 8);
    wm_draw_string(win, cursor_x, 4, "_", 0xFF000000);
}

static void notepad_key(window_t* win, char key) {
    notepad_state_t* state = (notepad_state_t*)win->user_data;
    if (!state) return;

    if (key == '\b') { 
        if (state->len > 0) {
            state->text[--state->len] = 0;
        }
    } else if (key >= 32 && key <= 126) {
        if (state->len < MAX_TEXT - 1) {
            state->text[state->len++] = key;
            state->text[state->len] = 0;
        }
    }
}

void notepad_create() {
    window_t* win = wm_create_window(150, 150, 400, 300, "Notepad");
    win->bg_color = 0xFFFFFFFF; 
    
    notepad_state_t* state = (notepad_state_t*)kmalloc_z(sizeof(notepad_state_t));
    win->user_data = state;
    win->on_paint = notepad_paint;
    win->on_key = notepad_key;
    
    const char* welcome = "Welcome to hzOS Notepad!";
    for (int i=0; welcome[i]; i++) {
        state->text[i] = welcome[i];
    }
    state->len = 24;
}
