#include "about.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"


static void about_paint(window_t *win, uint32_t *buf, int stride, int height) {
  (void)buf;
  (void)stride;
  (void)height;

  /* Blue gradient-ish background */
  wm_fill_rect(win, 0, 0, win->width, win->height, 0xFF1E3A5F);

  /* Title */
  wm_draw_string(win, 60, 20, "hzOS", 0xFFFFFFFF);

  /* Version */
  wm_draw_string(win, 40, 45, "Beta Build 1", 0xFFAACCFF);

  /* Description */
  wm_draw_string(win, 20, 75, "An x64 hobby operating system", 0xFFCCCCCC);
  wm_draw_string(win, 20, 90, "written in C", 0xFFCCCCCC);

  /* Credits */
  wm_draw_string(win, 20, 120, "Created by Horizon", 0xFF88FF88);

  /* Fun message */
  wm_draw_string(win, 20, 150, "Made with love <3", 0xFFFF8888);
}

void about_create(void) {
  window_t *win = wm_create_window(200, 150, 180, 180, "About hzOS");
  if (!win)
    return;

  win->bg_color = 0xFF1E3A5F;
  win->on_paint = about_paint;
}
