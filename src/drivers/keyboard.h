#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

void keyboard_init(void);
void keyboard_set_callback(void (*cb)(char));
void keyboard_poll(void); // optional, not needed if using IRQs only

#endif