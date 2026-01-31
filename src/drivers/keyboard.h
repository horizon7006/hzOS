#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

typedef enum {
    KB_LAYOUT_US,
    KB_LAYOUT_TR
} kb_layout_t;

void keyboard_init(void);
void keyboard_set_callback(void (*cb)(char));
void keyboard_set_layout(kb_layout_t layout);
kb_layout_t keyboard_get_layout(void);
void keyboard_poll(void); // optional, not needed if using IRQs only

#endif