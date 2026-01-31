#ifndef SYSCALL_H
#define SYSCALL_H

#include "common.h"

/* System call numbers */
#define SYS_EXIT  1
#define SYS_WRITE 4
#define SYS_READ  3
#define SYS_GUI_WINDOW_OPEN 10
#define SYS_GUI_DRAW_TEXT   11
#define SYS_GUI_FILL_RECT   12
#define SYS_GUI_EVENT_POLL  13
#define SYS_SHELL_EXEC        14
#define SYS_TERMINAL_GET_CHAR 15

/* Initialize syscall interface */
#include "isr.h"
struct registers* syscall_handler(struct registers* regs);
void syscall_init(void);

/* System call handlers */
void sys_exit(int code);
int sys_write(int fd, const char* buf, int count);
int sys_read(int fd, char* buf, int count);

#endif
