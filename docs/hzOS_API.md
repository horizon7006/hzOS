# hzOS GUI Application Programming Interface

This document describes the API for creating graphical applications in hzOS.

## Overview

The hzOS GUI system is based on a simple window manager with event callbacks. Applications create windows, register event handlers, and use drawing primitives to render content.

## Core Structures

### window_t
Represents a GUI window.

```c
typedef struct window {
    int x, y;              // Window position
    int width, height;     // Window dimensions
    char title[32];        // Window title
    uint32_t bg_color;     // Background color (0xAARRGGBB)
    int flags;             // Window state flags
    void* user_data;       // Application-specific data
    
    // Event callbacks
    paint_callback_t on_paint;
    key_callback_t on_key;
    click_callback_t on_click;
} window_t;
```

## Event Callbacks

### on_paint
Called when the window needs to be redrawn.

```c
void my_paint(window_t* win, uint32_t* buf, int w, int h) {
    // Draw your content here
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFFFFFFFF);
    wm_draw_string(win, 10, 10, "Hello World", 0xFF000000);
}
```

### on_key
Called when a key is pressed while window has focus.

```c
void my_key(window_t* win, char key) {
    // Handle keyboard input
    if (key == '\n') {
        // Enter pressed
    }
}
```

### on_click
Called when the user clicks inside the window's client area.

```c
void my_click(window_t* win, int x, int y) {
    // Handle mouse click at relative coordinates (x, y)
}
```

## Drawing Functions

### wm_fill_rect
Fill a rectangle with a solid color.

```c
void wm_fill_rect(window_t* win, int x, int y, int w, int h, uint32_t color);
```

### wm_draw_pixel
Draw a single pixel.

```c
void wm_draw_pixel(window_t* win, int x, int y, uint32_t color);
```

### wm_draw_string
Draw text string (8x8 font).

```c
void wm_draw_string(window_t* win, int x, int y, const char* str, uint32_t color);
```

### wm_draw_char
Draw a single character.

```c
void wm_draw_char(window_t* win, int x, int y, char c, uint32_t color);
```

## Window Management

### wm_create_window
Create a new window.

```c
window_t* wm_create_window(int x, int y, int w, int h, const char* title);
```

**Example:**
```c
window_t* win = wm_create_window(100, 100, 400, 300, "My App");
win->bg_color = 0xFFFFFFFF;
win->on_paint = my_paint;
win->on_key = my_key;
win->on_click = my_click;
```

## Complete Example Application

```c
#include "../gui/window_manager.h"
#include "../lib/memory.h"

typedef struct {
    int counter;
} my_app_state_t;

static void my_app_paint(window_t* win, uint32_t* buf, int w, int h) {
    (void)buf; (void)w; (void)h;
    
    my_app_state_t* state = (my_app_state_t*)win->user_data;
    
    // Clear background
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFFE0E0E0);
    
    // Draw title
    wm_draw_string(win, 10, 10, "Click Counter", 0xFF000000);
    
    // Draw counter (simple number display)
    char buf_str[32];
    int num = state->counter;
    int i = 0;
    if (num == 0) {
        buf_str[i++] = '0';
    } else {
        char temp[32];
        int j = 0;
        while (num > 0) {
            temp[j++] = '0' + (num % 10);
            num /= 10;
        }
        while (j > 0) {
            buf_str[i++] = temp[--j];
        }
    }
    buf_str[i] = 0;
    
    wm_draw_string(win, 10, 30, "Clicks: ", 0xFF000000);
    wm_draw_string(win, 70, 30, buf_str, 0xFF0000FF);
}

static void my_app_click(window_t* win, int x, int y) {
    (void)x; (void)y;
    my_app_state_t* state = (my_app_state_t*)win->user_data;
    state->counter++;
}

void my_app_create(void) {
    window_t* win = wm_create_window(150, 150, 300, 200, "My App");
    win->bg_color = 0xFFE0E0E0;
    
    my_app_state_t* state = (my_app_state_t*)kmalloc_z(sizeof(my_app_state_t));
    state->counter = 0;
    
    win->user_data = state;
    win->on_paint = my_app_paint;
    win->on_click = my_app_click;
}
```

## Color Format

Colors are 32-bit ARGB values: `0xAARRGGBB`
- AA: Alpha channel (0xFF = opaque)
- RR: Red channel
- GG: Green channel
- BB: Blue channel

**Common colors:**
- White: `0xFFFFFFFF`
- Black: `0xFF000000`
- Red: `0xFFFF0000`
- Green: `0xFF00FF00`
- Blue: `0xFF0000FF`

## Best Practices

1. **State Management**: Use `user_data` to store application state
2. **Memory**: Allocate state with `kmalloc_z` for zero-initialized memory
3. **Coordinates**: All drawing coordinates are relative to window's client area (top-left is 0,0)
4. **Redraw**: Window manager handles redraw automatically; no manual refresh needed
5. **Event Handling**: Keep event handlers lightweight; complex logic should be deferred
