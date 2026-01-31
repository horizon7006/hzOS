#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "mouse.h"
#include <stdint.h>

typedef struct window window_t;

typedef void (*paint_callback_t)(window_t *win, uint32_t *buf, int w, int h);
typedef void (*key_callback_t)(window_t *win, char key);
typedef void (*click_callback_t)(window_t *win, int x, int y);

#define EVENT_QUEUE_SIZE 16
#define WM_MAGIC 0x485A574D // 'HZWM'

typedef enum {
  HZ_EVENT_NONE,
  HZ_EVENT_KEY,
  HZ_EVENT_MOUSE_MOVE,
  HZ_EVENT_MOUSE_BUTTON,
  HZ_EVENT_QUIT
} hz_event_type_t;

typedef struct {
  hz_event_type_t type;
  int key;
  int mouse_x;
  int mouse_y;
  int mouse_buttons;
} hz_event_t;

struct window {
  uint32_t magic;
  int x, y;
  int width, height;
  char title[32];
  uint32_t bg_color;
  struct window *next;

  /* Flags */
  int flags;
  int is_userland;

  /* Event Queue (for userland) */
  hz_event_t event_queue[EVENT_QUEUE_SIZE];
  int event_head;
  int event_tail;

  /* Surface (for userland persistence) */
  uint32_t *surface;

  /* Callbacks and User Data (for kernel-mode) */
  void *user_data;
  paint_callback_t on_paint;
  key_callback_t on_key;
  click_callback_t on_click;
};

/* Initialize Window Manager and allocate backbuffers */
void wm_init(void);

/* Create a new window */
window_t *wm_create_window(int x, int y, int w, int h, const char *title);

/* Update internal state, handle inputs */
void wm_update(void);

/* Render everything to screen */
void wm_draw(void);

int wm_is_running(void);
void wm_push_event(window_t *win, hz_event_t ev);
void wm_prepare_window_drawing(window_t *win);

/* Drawing functions for kernel-mode apps */
void wm_fill_rect(window_t *win, int x, int y, int w, int h, uint32_t color);
void wm_draw_char(window_t *win, int x, int y, char c, uint32_t color);
void wm_draw_string(window_t *win, int x, int y, const char *str,
                    uint32_t color);

/* Binary app launching commented out - using integrated apps instead
void wm_launch_app(const char* path);
*/

#endif
