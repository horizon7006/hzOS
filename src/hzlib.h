#ifndef HZLIB_H
#define HZLIB_H

#include "common.h"

/* Tiny application API for the hzOS GUI.
   For now this is just a placeholder we can grow. */

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* title;
} hz_window_t;

typedef enum {
    HZ_EVENT_NONE,
    HZ_EVENT_KEY,
    HZ_EVENT_MOUSE_MOVE,
    HZ_EVENT_MOUSE_BUTTON
} hz_event_type_t;

typedef struct {
    hz_event_type_t type;
    int key;
    int mouse_x;
    int mouse_y;
    int mouse_buttons;
} hz_event_t;

void hz_window_open(hz_window_t* win);
void hz_window_draw_text(hz_window_t* win, int x, int y, const char* text);
int  hz_event_poll(hz_event_t* ev);

#endif

