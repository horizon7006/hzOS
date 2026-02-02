#include "libhz.h"

static inline int syscall(int num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a" (ret)
        : "a" (num), "b" (a), "c" (b), "d" (c), "S" (d), "D" (e)
        : "memory"
    );
    return ret;
}

void exit(int code) {
    syscall(SYS_EXIT, (uint32_t)code, 0, 0, 0, 0);
}

int write(int fd, const void* buf, size_t count) {
    return syscall(SYS_WRITE, (uint32_t)fd, (uint32_t)buf, (uint32_t)count, 0, 0);
}

int read(int fd, void* buf, size_t count) {
    return syscall(SYS_READ, (uint32_t)fd, (uint32_t)buf, (uint32_t)count, 0, 0);
}

void print(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    write(1, str, len);
}

void hz_window_open(hz_window_t* win) {
    win->handle = (uint32_t)syscall(SYS_GUI_WINDOW_OPEN, (uint32_t)win->x, (uint32_t)win->y, 
                                     (uint32_t)win->width, (uint32_t)win->height, (uint32_t)win->title);
}

void hz_window_draw_text(hz_window_t* win, int x, int y, const char* text, uint32_t color) {
    syscall(SYS_GUI_DRAW_TEXT, win->handle, (uint32_t)x, (uint32_t)y, (uint32_t)text, color);
}

void hz_window_fill_rect(hz_window_t* win, int x, int y, int w, int h, uint32_t color) {
    // For 6+ args, we'll pack some. Let's pack x,y and w,h
    uint32_t xy = ((uint32_t)x << 16) | (uint32_t)y;
    uint32_t wh = ((uint32_t)w << 16) | (uint32_t)h;
    syscall(SYS_GUI_FILL_RECT, win->handle, xy, wh, color, 0);
}

int hz_event_poll(hz_window_t* win, hz_event_t* ev) {
    return (int)syscall(SYS_GUI_EVENT_POLL, win->handle, (uint32_t)ev, 0, 0, 0);
}

void hz_shell_exec(const char* cmd) {
    syscall(SYS_SHELL_EXEC, (uint32_t)cmd, 0, 0, 0, 0);
}

int hz_terminal_get_char(void) {
    return syscall(SYS_TERMINAL_GET_CHAR, 0, 0, 0, 0, 0);
}
