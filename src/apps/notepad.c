#include "notepad.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"


#define NOTEPAD_MAX_TEXT 1024

typedef struct {
  char text[NOTEPAD_MAX_TEXT];
  int text_len;
} notepad_state_t;

static void notepad_paint(window_t *win, uint32_t *buf, int stride,
                          int height) {
  (void)buf;
  (void)stride;
  (void)height;

  notepad_state_t *state = (notepad_state_t *)win->user_data;
  if (!state)
    return;

  /* White background */
  wm_fill_rect(win, 0, 0, win->width, win->height, 0xFFFFFFFF);

  /* Draw text */
  int x = 4;
  int y = 4;
  for (int i = 0; i < state->text_len; i++) {
    char c = state->text[i];
    if (c == '\n') {
      x = 4;
      y += 10;
    } else {
      wm_draw_char(win, x, y, c, 0xFF000000);
      x += 8;
      if (x > win->width - 12) {
        x = 4;
        y += 10;
      }
    }
  }

  /* Draw cursor */
  wm_draw_char(win, x, y, '_', 0xFF000000);
}

static void notepad_key(window_t *win, char key) {
  notepad_state_t *state = (notepad_state_t *)win->user_data;
  if (!state)
    return;

  if (key == '\b') {
    if (state->text_len > 0) {
      state->text_len--;
      state->text[state->text_len] = 0;
    }
  } else if (key >= 32 && key <= 126) {
    if (state->text_len < NOTEPAD_MAX_TEXT - 1) {
      state->text[state->text_len++] = key;
      state->text[state->text_len] = 0;
    }
  } else if (key == '\n' || key == '\r') {
    if (state->text_len < NOTEPAD_MAX_TEXT - 1) {
      state->text[state->text_len++] = '\n';
      state->text[state->text_len] = 0;
    }
  }
}

void notepad_create(void) {
  window_t *win = wm_create_window(100, 100, 400, 300, "Notepad");
  if (!win)
    return;

  win->bg_color = 0xFFFFFFFF;

  notepad_state_t *state =
      (notepad_state_t *)kmalloc_z(sizeof(notepad_state_t));
  state->text[0] = 0;
  state->text_len = 0;

  win->user_data = state;
  win->on_paint = notepad_paint;
  win->on_key = notepad_key;
}
