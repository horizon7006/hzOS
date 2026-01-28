#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"

void terminal_init(void);
void terminal_clear(void);
void terminal_setcolor(uint8_t color);
void terminal_putc(char c);
void terminal_write(const char* str);
void terminal_writeln(const char* str);

#endif