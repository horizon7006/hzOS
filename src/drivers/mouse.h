#ifndef MOUSE_H
#define MOUSE_H

#include "common.h"

typedef struct {
    int x;
    int y;
    int left_down;
    int right_down;
    int middle_down;
} mouse_state_t;

void mouse_init(void);
mouse_state_t mouse_get_state(void);
void mouse_set_bounds(int width, int height);
void mouse_reset_position(int x, int y);

#endif

