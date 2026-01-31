#include "window_manager.h"
#include "../core/io.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"
#include "../drivers/rtc.h"
#include "../drivers/vesa.h"
#include "../lib/memory.h"
#include "font.h"
#include "graphics.h"

/* Binary/process loading commented out for now
#include "../core/process.h"
#include "../core/elf.h"
*/
#include "../apps/about.h"
#include "../apps/notepad.h"
#include "../apps/terminal_app.h"
#include "../fs/filesystem.h"

static uint32_t *backbuffer = 0;
static window_t *windows = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_l = 0;
static int prev_mouse_l = 0;

static window_t *dragging_window = 0;
static int drag_off_x = 0;
static int drag_off_y = 0;
static int start_menu_open = 0;
static int wm_running = 1;

/* Clipping context for current window drawing */
static int clip_x, clip_y, clip_w, clip_h;
static uint32_t *clip_buf;
static int clip_stride;

int wm_is_running() { return wm_running; }

void wm_shutdown() { wm_running = 0; }

void wm_init() {
  if (!vesa_video_memory)
    return;

  backbuffer = (uint32_t *)kmalloc_a(vesa_width * vesa_height * 4);

  mouse_set_bounds(vesa_width, vesa_height);
  mouse_x = vesa_width / 2;
  mouse_y = vesa_height / 2;

  // Reset mouse driver to match our initial position
  mouse_reset_position(mouse_x, mouse_y);

  wm_running = 1;
}

window_t *wm_create_window(int x, int y, int w, int h, const char *title) {

  window_t *win = (window_t *)kmalloc_z(sizeof(window_t));
  if (!win)
    return NULL;
  win->magic = WM_MAGIC;
  win->x = x;
  win->y = y;
  win->width = w;
  win->height = h;
  win->bg_color = WIN7_WINDOW_BG;
  win->flags = 0;

  char *d = win->title;
  const char *s = title;
  for (int i = 0; i < 31 && *s; i++)
    *d++ = *s++;
  *d = 0;

  win->next = windows;
  windows = win;

  // Allocate surface for window (required for persistent userland drawing)
  win->surface = (uint32_t *)kmalloc_z(w * h * 4);

  return win;
}

/* Binary app launching commented out - using integrated apps instead
void wm_launch_app(const char* path) {
    size_t size = 0;
    const char* data = fs_read(path, &size);
    if (!data) return;

    if (elf_validate(data) != 0) return;
    uint32_t entry = elf_get_entry(data);

    process_t* p = process_create(path, entry);
    if (!p) return;

    process_load_and_execute(p, data, size);
}
*/

static void wm_remove_window(window_t *win) {
  if (!win)
    return;
  if (windows == win) {
    windows = win->next;
    return;
  }
  window_t *curr = windows;
  while (curr && curr->next != win) {
    curr = curr->next;
  }
  if (curr) {
    curr->next = win->next;
  }

  // Reclaim memory
  win->magic = 0; // Invalidate handle for any ongoing syscalls
  if (win->surface) {
    kfree(win->surface);
    win->surface = NULL;
  }
  kfree(win);
}

void wm_push_event(window_t *win, hz_event_t ev) {
  if (!win)
    return;
  int next = (win->event_head + 1) % EVENT_QUEUE_SIZE;
  if (next == win->event_tail)
    return; // Drop if full
  win->event_queue[win->event_head] = ev;
  win->event_head = next;
}

static void wm_focus_window(window_t *win) {
  if (!win || windows == win)
    return;

  window_t *curr = windows;
  while (curr && curr->next != win) {
    curr = curr->next;
  }
  if (curr) {
    curr->next = win->next;
    win->next = windows;
    windows = win;
  }
}

void wm_prepare_window_drawing(window_t *win) {
  if (win->surface) {
    clip_buf = win->surface;
    clip_x = 0;
    clip_y = 0;
    clip_w = win->width;
    clip_h = win->height;
    clip_stride = win->width;
  } else {
    clip_buf = backbuffer;
    clip_x = win->x;
    clip_y = win->y;
    clip_w = win->width;
    clip_h = win->height;
    clip_stride = vesa_width;
  }
}

void wm_draw_pixel(window_t *win, int x, int y, uint32_t color) {
  (void)win;
  int px = clip_x + x;
  int py = clip_y + y;
  if (x < 0 || x >= clip_w || y < 0 || y >= clip_h)
    return;
  clip_buf[py * clip_stride + px] = color;
}

void wm_fill_rect(window_t *win, int x, int y, int w, int h, uint32_t color) {
  int rx = clip_x + x;
  int ry = clip_y + y;

  if (rx < clip_x) {
    w -= (clip_x - rx);
    rx = clip_x;
  }
  if (ry < clip_y) {
    h -= (clip_y - ry);
    ry = clip_y;
  }
  if (rx + w > clip_x + clip_w)
    w = (clip_x + clip_w) - rx;
  if (ry + h > clip_y + clip_h)
    h = (clip_y + clip_h) - ry;

  if (w <= 0 || h <= 0)
    return;

  for (int j = 0; j < h; j++) {
    uint32_t *row = clip_buf + (ry + j) * clip_stride + rx;
    for (int i = 0; i < w; i++)
      *row++ = color;
  }
}

void wm_draw_char(window_t *win, int x, int y, char c, uint32_t color) {
  if (c < 0 || c > 127)
    return;
  const uint8_t *glyph = font8x8_basic[(int)c];
  for (int row = 0; row < 8; row++) {
    uint8_t bits = glyph[row];
    for (int col = 0; col < 8; col++) {
      if ((bits >> (7 - col)) & 1) {
        wm_draw_pixel(win, x + col, y + row, color);
      }
    }
  }
}

void wm_draw_string(window_t *win, int x, int y, const char *str,
                    uint32_t color) {
  while (*str) {
    wm_draw_char(win, x, y, *str++, color);
    x += 8;
  }
}

static void buf_draw_char(uint32_t *buf, int x, int y, char c, uint32_t color) {
  gfx_draw_char_to_buffer(buf, x, y, vesa_width, vesa_height, c, color);
}

static void buf_draw_string(uint32_t *buf, int x, int y, const char *str,
                            uint32_t color) {
  gfx_draw_string_to_buffer(buf, x, y, vesa_width, vesa_height, str, color);
}

static void buf_fill_rect(uint32_t *buf, int x, int y, int w, int h,
                          uint32_t color) {
  gfx_fill_rect_to_buffer(buf, x, y, w, h, vesa_width, vesa_height, color);
}

static void draw_desktop_elements(uint32_t *buf) {
  int total = vesa_width * vesa_height;
  for (int i = 0; i < total; i++)
    buf[i] = WIN7_BG_COLOR;

  buf_fill_rect(buf, 20, 20, 32, 32, 0xFFFFFFFF);
  buf_draw_char(buf, 30, 30, 'N', 0xFF000000);
  buf_draw_string(buf, 15, 55, "Notepad", COLOR_WHITE);

  buf_fill_rect(buf, 20, 90, 32, 32, 0xFF000000);
  buf_draw_string(buf, 22, 100, ">_", 0xFF00FF00);
  buf_draw_string(buf, 15, 125, "Terminal", COLOR_WHITE);

  buf_fill_rect(buf, 20, 160, 32, 32, 0xFF0000FF);
  buf_draw_char(buf, 32, 170, '?', 0xFFFFFFFF);
  buf_draw_string(buf, 20, 195, "About", COLOR_WHITE);

  int tb_height = 40;
  int tb_y = vesa_height - tb_height;

  buf_fill_rect(buf, 0, tb_y, vesa_width, tb_height, 0xFF202020);

  int start_x = 5;
  int start_y = tb_y + 5;
  int start_w = 40;
  int start_h = 30;

  uint32_t outline_col = start_menu_open ? 0xFF00FF00 : 0xFFFFFFFF;

  buf_fill_rect(buf, start_x, start_y, start_w, 2, outline_col);
  buf_fill_rect(buf, start_x, start_y + start_h - 2, start_w, 2, outline_col);
  buf_fill_rect(buf, start_x, start_y, 2, start_h, outline_col);
  buf_fill_rect(buf, start_x + start_w - 2, start_y, 2, start_h, outline_col);

  buf_draw_string(buf, start_x + 12, start_y + 11, "hz", outline_col);

  rtc_time_t t;
  rtc_read(&t);
  char time_str[16];
  time_str[0] = '0' + (t.hour / 10);
  time_str[1] = '0' + (t.hour % 10);
  time_str[2] = ':';
  time_str[3] = '0' + (t.minute / 10);
  time_str[4] = '0' + (t.minute % 10);
  time_str[5] = 0;
  buf_draw_string(buf, vesa_width - 60, tb_y + 16, time_str, COLOR_WHITE);

  // Layout Indicator
  kb_layout_t layout = keyboard_get_layout();
  const char *layout_str = (layout == KB_LAYOUT_TR) ? "TR" : "US";
  buf_draw_string(buf, vesa_width - 100, tb_y + 16, layout_str, 0xFF00FF00);

  int tb_btn_x = 60;
  int tb_btn_w = 100;
  int tb_btn_gap = 5;

  window_t *w = windows;
  while (w) {
    uint32_t task_col = (w->flags & 1) ? 0xFF404040 : 0xFF606060;
    buf_fill_rect(buf, tb_btn_x, start_y, tb_btn_w, 30, task_col);

    char tmp[10];
    for (int i = 0; i < 9; i++)
      tmp[i] = w->title[i];
    tmp[9] = 0;
    buf_draw_string(buf, tb_btn_x + 5, start_y + 10, tmp, COLOR_WHITE);

    tb_btn_x += tb_btn_w + tb_btn_gap;
    w = w->next;
  }
}

static void draw_window(uint32_t *buf, window_t *w) {
  if (w->flags & 1)
    return;

  int wx = w->x;
  int wy = w->y;
  int ww = w->width;
  int wh = w->height;

  int title_h = 24;
  int border = 4;

  int outer_x = wx - border;
  int outer_y = wy - title_h - border;
  int outer_w = ww + 2 * border;
  int outer_h = wh + 2 * border + title_h;
  uint32_t border_col = 0xFF60A0D0;

  buf_fill_rect(buf, outer_x, outer_y, outer_w, border, border_col);
  buf_fill_rect(buf, outer_x, outer_y + outer_h - border, outer_w, border,
                border_col);
  buf_fill_rect(buf, outer_x, outer_y, border, outer_h, border_col);
  buf_fill_rect(buf, outer_x + outer_w - border, outer_y, border, outer_h,
                border_col);
  buf_fill_rect(buf, outer_x + border, outer_y + border, ww, title_h,
                border_col);

  buf_draw_string(buf, outer_x + 6, outer_y + 6, w->title, COLOR_WHITE);

  int btn_size = 14;
  int btn_y = outer_y + 5;
  int btn_x = outer_x + outer_w - 20;

  buf_fill_rect(buf, btn_x, btn_y, btn_size, btn_size, 0xFFE04040);
  buf_draw_string(buf, btn_x + 3, btn_y + 3, "X", COLOR_WHITE);

  btn_x -= 20;
  buf_fill_rect(buf, btn_x, btn_y, btn_size, btn_size, 0xFF4040E0);

  btn_x -= 20;
  buf_fill_rect(buf, btn_x, btn_y, btn_size, btn_size, 0xFF40E040);
  buf_draw_string(buf, btn_x + 3, btn_y + 3, "_", COLOR_WHITE);

  if (w->on_paint) {
    /* Kernel-mode app with paint callback */
    wm_prepare_window_drawing(w);
    w->on_paint(w, buf, vesa_width, vesa_height);
  }
  
  if (w->surface) {
    // Blit userland surface to backbuffer
    for (int y = 0; y < wh; y++) {
      if (wy + y < 0 || wy + y >= vesa_height)
        continue;
      memcpy(&buf[(wy + y) * vesa_width + wx], &w->surface[y * ww], ww * 4);
    }
  }
}

static void draw_start_menu(uint32_t *buf) {
  int m_w = 120;
  int m_h = 125;
  int m_x = 0;
  int m_y = vesa_height - 40 - m_h;

  buf_fill_rect(buf, m_x, m_y, m_w, m_h, 0xFF303030);
  buf_fill_rect(buf, m_x + m_w, m_y, 2, m_h, 0xFF606060);
  buf_fill_rect(buf, m_x, m_y - 2, m_w + 2, 2, 0xFF606060);

  int item_h = 25;

  buf_draw_string(buf, m_x + 10, m_y + 8, "Notepad", 0xFFFFFFFF);
  buf_draw_string(buf, m_x + 10, m_y + 8 + item_h, "Terminal", 0xFFFFFFFF);
  buf_draw_string(buf, m_x + 10, m_y + 8 + item_h * 2, "About", 0xFFFFFFFF);
  buf_draw_string(buf, m_x + 10, m_y + 8 + item_h * 3, "Restart", 0xFFFFFF00);
  buf_draw_string(buf, m_x + 10, m_y + 8 + item_h * 4, "Shutdown", 0xFFFFA0A0);
}

static void draw_cursor(uint32_t *buf, int mx, int my) {
  static const int cursor[12][12] = {{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                     {1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                     {1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                                     {1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
                                     {1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0},
                                     {1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0},
                                     {1, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0},
                                     {1, 2, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
                                     {1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
                                     {1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
                                     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
  for (int y = 0; y < 12; y++) {
    for (int x = 0; x < 12; x++) {
      int px = mx + x;
      int py = my + y;
      if (px >= vesa_width || py >= vesa_height)
        continue;
      int c = cursor[y][x];
      if (c == 0)
        continue;
      buf[py * vesa_width + px] = (c == 1) ? COLOR_BLACK : COLOR_WHITE;
    }
  }
}

void wm_draw() {
  if (!backbuffer)
    return;
  draw_desktop_elements(backbuffer);

  void draw_rec(window_t * n) {
    if (!n)
      return;
    draw_rec(n->next);
    draw_window(backbuffer, n);
  }
  draw_rec(windows);

  if (start_menu_open) {
    draw_start_menu(backbuffer);
  }

  draw_cursor(backbuffer, mouse_x, mouse_y);
  if (vesa_video_memory) {
    uint32_t *dst = (uint32_t *)vesa_video_memory;
    uint32_t *src = backbuffer;
    int total_pixels = vesa_width * vesa_height;
    for (int i = 0; i < total_pixels; i++) {
      dst[i] = src[i];
    }
  }
}

void wm_update() {
  mouse_state_t s = mouse_get_state();
  mouse_x = s.x;
  mouse_y = s.y;
  mouse_l = s.left_down;

  if (mouse_x < 0)
    mouse_x = 0;
  if (mouse_y < 0)
    mouse_y = 0;
  if (mouse_x >= vesa_width)
    mouse_x = vesa_width - 1;
  if (mouse_y >= vesa_height)
    mouse_y = vesa_height - 1;

  if (dragging_window) {
    if (mouse_l) {
      dragging_window->x = mouse_x - drag_off_x;
      dragging_window->y = mouse_y - drag_off_y;
    } else {
      dragging_window = 0;
    }
    prev_mouse_l = mouse_l;
    return;
  }

  if (mouse_l && !prev_mouse_l) {
    prev_mouse_l = mouse_l;

    int tb_height = 40;
    int tb_y = vesa_height - tb_height;
    int start_x = 5;
    int start_y = tb_y + 5;

    if (mouse_x >= start_x && mouse_x < start_x + 40 && mouse_y >= start_y &&
        mouse_y < start_y + 30) {
      start_menu_open = !start_menu_open;
      return;
    }

    if (start_menu_open) {
      int m_w = 120;
      int m_h = 125;
      int m_x = 0;
      int m_y = vesa_height - 40 - m_h;

      if (mouse_x >= m_x && mouse_x < m_x + m_w && mouse_y >= m_y &&
          mouse_y < m_y + m_h) {
        int rel_y = mouse_y - m_y;
        int item_idx = rel_y / 25;

        // We'll use a helper to launch apps from the GUI
        // For now, let's assume we have a way to run them.
        // Since this is single-tasking, it will block the WM.
        // We'll need a better process manager eventually.
        if (item_idx == 0) {
          notepad_create();
        } else if (item_idx == 1) {
          terminal_app_create();
        } else if (item_idx == 2) {
          about_create();
        } else if (item_idx == 3) {
          /* Restart */
          outb(0x64, 0xFE);
          asm volatile("hlt");
        } else if (item_idx == 4) {
          /* Shutdown */
          asm volatile("cli");

          // Try multiple shutdown methods
          outw(0x604, 0x2000);  // QEMU
          outw(0xB004, 0x2000); // Bochs
          outw(0x4004, 0x3400); // VirtualBox
          outw(0x1004, 0x2000); // ACPI PM1a
          outw(0x404, 0x2000);  // ACPI PM1a alternate

          // If still running, halt
          while (1)
            asm volatile("hlt");
        }
        start_menu_open = 0;
        return;
      } else {
        start_menu_open = 0;
      }
    }

    window_t *w = windows;
    int hit = 0;
    while (w) {
      if (w->flags & 1) {
        w = w->next;
        continue;
      }

      int title_h = 24;
      int border = 4;
      int ox = w->x - border;
      int oy = w->y - title_h - border;
      int ow = w->width + 2 * border;
      int oh = w->height + 2 * border + title_h;

      if (mouse_x >= ox && mouse_x < ox + ow && mouse_y >= oy &&
          mouse_y < oy + oh) {
        wm_focus_window(w);
        hit = 1;

        if (w->is_userland) {
          hz_event_t ev;
          ev.type = HZ_EVENT_MOUSE_BUTTON;
          ev.mouse_x = mouse_x - w->x;
          ev.mouse_y = mouse_y - w->y;
          ev.mouse_buttons = mouse_l;
          wm_push_event(w, ev);
        }

        int btn_size = 14;
        int btn_y = oy + 5;
        int btn_x = ox + ow - 20;

        if (mouse_x >= btn_x && mouse_x < btn_x + btn_size &&
            mouse_y >= btn_y && mouse_y < btn_y + btn_size) {
          wm_remove_window(w);
        } else if (mouse_x >= btn_x - 40 && mouse_x < btn_x - 40 + btn_size &&
                   mouse_y >= btn_y && mouse_y < btn_y + btn_size) {
          w->flags |= 1;
        } else if (mouse_y < w->y) {
          dragging_window = w;
          drag_off_x = mouse_x - w->x;
          drag_off_y = mouse_y - w->y;
        } else if (mouse_x >= w->x && mouse_x < w->x + w->width &&
                   mouse_y >= w->y && mouse_y < w->y + w->height) {
          // Client Area Click
          if (w->on_click) {
            w->on_click(w, mouse_x - w->x, mouse_y - w->y);
          }
        }
        break;
      }
      w = w->next;
    }

    if (!hit) {
      if (mouse_y >= tb_y) {
        // Taskbar Click
        if (mouse_x >= vesa_width - 110 && mouse_x < vesa_width - 70) {
          // Layout Toggle
          kb_layout_t layout = keyboard_get_layout();
          keyboard_set_layout((layout == KB_LAYOUT_US) ? KB_LAYOUT_TR
                                                       : KB_LAYOUT_US);
        } else {
          int tb_btn_x = 60;
          int tb_btn_w = 100;
          w = windows;
          while (w) {
            if (mouse_x >= tb_btn_x && mouse_x < tb_btn_x + tb_btn_w) {
              if (w->flags & 1) {
                w->flags &= ~1;
                wm_focus_window(w);
              } else {
                if (windows == w) {
                  w->flags |= 1;
                } else {
                  w->flags &= ~1;
                  wm_focus_window(w);
                }
              }
              break;
            }
            tb_btn_x += tb_btn_w + 5;
            w = w->next;
          }
        }
      } else {
        if (mouse_x >= 20 && mouse_x < 20 + 32) {
          if (mouse_y >= 20 && mouse_y < 20 + 32) {
            notepad_create();
          } else if (mouse_y >= 90 && mouse_y < 90 + 32) {
            terminal_app_create();
          } else if (mouse_y >= 160 && mouse_y < 160 + 32) {
            about_create();
          }
        }
      }
    }
  }
  prev_mouse_l = mouse_l;
}

void wm_handle_mouse(mouse_state_t state) { (void)state; }

void wm_handle_key(char c) {
  if (windows && !(windows->flags & 1)) {
    if (windows->is_userland) {
      hz_event_t ev;
      ev.type = HZ_EVENT_KEY;
      ev.key = c;
      wm_push_event(windows, ev);
    } else if (windows->on_key) {
      windows->on_key(windows, c);
    }
  }
}
