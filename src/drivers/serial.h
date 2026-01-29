#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>

void serial_init();
void serial_putc(char c);
void serial_write(const char* str);
void serial_printf(const char* fmt, ...);

#endif
