#include "syscall.h"
#include "../shell/terminal.h"
#include "../shell/command.h"
#include "../lib/printf.h"
#include "isr.h"
#include "process.h"
#include "../gui/window_manager.h"
#include "../gui/graphics.h"

struct registers* scheduler_schedule(struct registers* regs);

void sys_exit(int code) {
    kprintf("syscall: exit(%d)\n", code);
}

int sys_write(int fd, const char* buf, int count) {
    if (fd == 1) {
        for (int i = 0; i < count; i++) {
            terminal_putc(buf[i]);
            serial_printf("%c", buf[i]);
        }
        return count;
    }
    return -1;
}

int sys_read(int fd, char* buf, int count) {
    if (fd != 0 || !buf || count <= 0) {
        return -1;
    }

    int read_count = 0;
    while (read_count < count) {
        int c = terminal_get_char();
        if (c < 0) {
            break;
        }
        buf[read_count++] = (char)c;
    }

    return read_count;
}

static void* sys_write_wrapper(uint64_t fd, uint64_t buf, uint64_t count, uint64_t d, uint64_t e) {
    (void)d; (void)e;
    return (void*)(uint64_t)sys_write((int)fd, (const char*)buf, (int)count);
}

static void* sys_read_wrapper(uint64_t fd, uint64_t buf, uint64_t count, uint64_t d, uint64_t e) {
    (void)d; (void)e;
    return (void*)(uint64_t)sys_read((int)fd, (char*)buf, (int)count);
}

static void* sys_exit_wrapper(uint64_t code, uint64_t b, uint64_t c, uint64_t d, uint64_t e) {
    (void)b; (void)c; (void)d; (void)e;
    sys_exit((int)code);
    return NULL;
}

static void* sys_gui_window_open_wrapper(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t title) {
    window_t* win = wm_create_window((int)x, (int)y, (int)w, (int)h, (const char*)title);
    if (win) {
        win->is_userland = 1;
        win->bg_color = 0xFFFFFFFF;
    }
    return (void*)win;
}

static void* sys_gui_draw_text_wrapper(uint64_t handle, uint64_t x, uint64_t y, uint64_t text, uint64_t color) {
    window_t* win = (window_t*)handle;
    if (win && win->magic == WM_MAGIC) {
        wm_prepare_window_drawing(win);
        wm_draw_string(win, (int)x, (int)y, (const char*)text, (uint32_t)color);
    }
    return NULL;
}

static void* sys_gui_fill_rect_wrapper(uint64_t handle, uint64_t xy, uint64_t wh, uint64_t color, uint64_t unused) {
    (void)unused;
    window_t* win = (window_t*)handle;
    if (win && win->magic == WM_MAGIC) {
        int x = (int)(xy >> 16);
        int y = (int)(xy & 0xFFFF);
        int w = (int)(wh >> 16);
        int h = (int)(wh & 0xFFFF);
        wm_prepare_window_drawing(win);
        wm_fill_rect(win, x, y, w, h, (uint32_t)color);
    }
    return NULL;
}

static void* sys_gui_event_poll_wrapper(uint64_t handle, uint64_t ev_ptr, uint64_t c, uint64_t d, uint64_t e) {
    (void)c; (void)d; (void)e;
    window_t* win = (window_t*)handle;
    hz_event_t* dest = (hz_event_t*)ev_ptr;

    if (!win || !ev_ptr || win->magic != WM_MAGIC) {
        if (dest) dest->type = HZ_EVENT_QUIT;
        return (void*)0; 
    }

    if (win->event_head == win->event_tail) return (void*)0;

    *dest = win->event_queue[win->event_tail];
    win->event_tail = (win->event_tail + 1) % EVENT_QUEUE_SIZE;

    return (void*)1;
}

static void* sys_shell_exec_wrapper(uint64_t command, uint64_t b, uint64_t c, uint64_t d, uint64_t e) {
    (void)b; (void)c; (void)d; (void)e;
    command_execute((const char*)command);
    return NULL;
}

static void* sys_terminal_get_char_wrapper(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e;
    return (void*)(uint64_t)terminal_get_char();
}

typedef void* (*syscall_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static syscall_t syscalls[] = {
    [SYS_EXIT]  = sys_exit_wrapper,
    [SYS_READ]  = sys_read_wrapper,
    [SYS_WRITE] = sys_write_wrapper,
    [SYS_GUI_WINDOW_OPEN] = sys_gui_window_open_wrapper,
    [SYS_GUI_DRAW_TEXT]   = sys_gui_draw_text_wrapper,
    [SYS_GUI_FILL_RECT]   = sys_gui_fill_rect_wrapper,
    [SYS_GUI_EVENT_POLL]  = sys_gui_event_poll_wrapper,
    [SYS_SHELL_EXEC]      = sys_shell_exec_wrapper,
    [SYS_TERMINAL_GET_CHAR] = sys_terminal_get_char_wrapper,
};

static const int num_syscalls = sizeof(syscalls) / sizeof(syscalls[0]);

struct registers* syscall_handler(struct registers* regs) {
    uint64_t num = regs->rax;
    if (num >= (uint64_t)num_syscalls) {
        kprintf("syscall: invalid syscall %ld\n", num);
        return regs;
    }

    syscall_t func = syscalls[num];
    if (func) {
        regs->rax = (uint64_t)func(regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi);
    }

    return regs;
}

void syscall_init(void) {
    kprintf("syscall: initialized\n");
}
