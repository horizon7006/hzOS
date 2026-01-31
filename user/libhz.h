#ifndef LIBHZ_H
#define LIBHZ_H

#include <stdint.h>
#include <stddef.h>

/* Syscall numbers */
#define SYS_EXIT  1
#define SYS_WRITE 4
#define SYS_READ  3
#define SYS_GUI_WINDOW_OPEN 10
#define SYS_GUI_DRAW_TEXT   11
#define SYS_GUI_FILL_RECT   12
#define SYS_GUI_EVENT_POLL  13
#define SYS_SHELL_EXEC     14
#define SYS_TERMINAL_GET_CHAR 15

/* GUI Types */
typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* title;
    uint32_t handle; // Internal window handle/ID
} hz_window_t;

typedef enum {
    HZ_EVENT_NONE,
    HZ_EVENT_KEY,
    HZ_EVENT_MOUSE_MOVE,
    HZ_EVENT_MOUSE_BUTTON,
    HZ_EVENT_QUIT
} hz_event_type_t;

typedef struct {
    hz_event_type_t type;
    int key;
    int mouse_x;
    int mouse_y;
    int mouse_buttons;
} hz_event_t;

/* API */
void exit(int code);
int write(int fd, const void* buf, size_t count);
void print(const char* str);

/* GUI API */
void hz_window_open(hz_window_t* win);
void hz_window_draw_text(hz_window_t* win, int x, int y, const char* text, uint32_t color);
void hz_window_fill_rect(hz_window_t* win, int x, int y, int w, int h, uint32_t color);
int  hz_event_poll(hz_window_t* win, hz_event_t* ev);
void hz_shell_exec(const char* cmd);
int  hz_terminal_get_char(void);

#endif
