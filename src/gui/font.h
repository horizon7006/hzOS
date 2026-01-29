#ifndef FONT_H
#define FONT_H

#include <stdint.h>

/* Simple 8x8 CGA/VGA font bitmap (ASCII 0-127) */
/* This is a truncated version for space, usually one would include the full 4KB VGA font */
extern uint8_t font8x8_basic[128][8];

#endif
