#include "file_browser.h"
#include "../fs/filesystem.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"
#include "../lib/printf.h"
#include "image_viewer.h"
#include "notepad.h"


typedef struct {
  char current_path[256];
  // Simple state: just path.
  // In a real app we'd track selection, scroll offset, etc.
} fb_state_t;

static void fb_paint(window_t *win, uint32_t *buf, int stride, int height) {
  (void)buf;
  (void)stride;
  (void)height;

  fb_state_t *state = (fb_state_t *)win->user_data;
  if (!state)
    return;

  // Background
  wm_fill_rect(win, 0, 0, win->width, win->height, 0xFFFFFFFF);

  // Header (Path)
  wm_fill_rect(win, 0, 0, win->width, 20, 0xFFDDDDDD);
  wm_draw_string(win, 4, 4, state->current_path, 0xFF000000);

  // List files
  fs_node_t *node = fs_find(state->current_path);
  if (!node || node->type != FS_NODE_DIR) {
    wm_draw_string(win, 4, 24, "Invalid Directory", 0xFFFF0000);
    return;
  }

  int y = 24;
  fs_node_t *child = node->first_child;

  // Draw ".." if not root
  if (node->parent) {
    wm_draw_string(win, 4, y, "[DIR]  ..", 0xFF000080);
    y += 12;
  }

  while (child) {
    if (y > win->height - 12)
      break; // Clip

    uint32_t col = (child->type == FS_NODE_DIR) ? 0xFF000080 : 0xFF000000;
    char line[64];

    // Manual formatting since no sprintf in this env?
    // Actually printf.h usually has kprintf, maybe sprintf/snprintf?
    // We'll trust we can format carefully or use simple strings.
    // Let's just draw type and name.

    int x = 4;
    if (child->type == FS_NODE_DIR) {
      wm_draw_string(win, x, y, "[DIR] ", col);
      x += 48;
    } else {
      wm_draw_string(win, x, y, "[FILE]", col);
      x += 48;
    }

    wm_draw_string(win, x, y, child->name, col);

    y += 12;
    child = child->next_sibling;
  }
}

// Removed internal app headers as they are now userland binaries

// ... (fb_paint remains same, but we need to match logic)

static void fb_key(window_t *win, char key) {
  (void)win;
  (void)key;
}

static void fb_on_click(window_t *win, int x, int y) {
  (void)x;
  fb_state_t *state = (fb_state_t *)win->user_data;
  if (!state)
    return;

  if (y < 24)
    return; // Header

  // ... (rest is same logic, just modifying file opening part)

  // Logic from before...
  int rel_y = y - 24;
  int idx = rel_y / 12;

  fs_node_t *node = fs_find(state->current_path);
  if (!node || node->type != FS_NODE_DIR)
    return;

  int current_idx = 0;

  if (node->parent) {
    if (current_idx == idx) {
      // .. logic
      int len = 0;
      while (state->current_path[len])
        len++;
      while (len > 0 && state->current_path[len] != '/')
        len--;
      if (len == 0) {
        state->current_path[0] = '/';
        state->current_path[1] = 0;
      } else {
        state->current_path[len] = 0;
      }
      return;
    }
    current_idx++;
  }

  fs_node_t *child = node->first_child;
  while (child) {
    if (current_idx == idx) {
      if (child->type == FS_NODE_DIR) {
        // ... same dir logic
        int plen = 0;
        while (state->current_path[plen])
          plen++;
        int nlen = 0;
        while (child->name[nlen])
          nlen++;
        if (plen + 1 + nlen < 255) {
          if (plen > 1)
            state->current_path[plen++] = '/';
          else if (plen == 1 && state->current_path[0] == '/') {
          }
          int k = 0;
          while (k < nlen)
            state->current_path[plen++] = child->name[k++];
          state->current_path[plen] = 0;
        }
      } else {
        // Open File
        char full_path[256];
        int p = 0;
        int k = 0;
        while (state->current_path[k])
          full_path[p++] = state->current_path[k++];
        if (p > 1)
          full_path[p++] = '/';
        else if (p == 1 && full_path[0] == '/') {
        }
        k = 0;
        while (child->name[k])
          full_path[p++] = child->name[k++];
        full_path[p] = 0;

        // Check extension
        int len = p;
        if (len > 4 && full_path[len - 4] == '.' && full_path[len - 3] == 'p' &&
            full_path[len - 2] == 'n' && full_path[len - 1] == 'g') {
          image_viewer_create(full_path);
        } else {
          notepad_create();
        }
      }
      return;
    }
    child = child->next_sibling;
    current_idx++;
  }
}

void file_browser_create(const char *start_path) {
  window_t *win = wm_create_window(50, 50, 300, 250, "File Browser");
  win->bg_color = 0xEEF0F0F0;

  fb_state_t *state = (fb_state_t *)kmalloc_z(sizeof(fb_state_t));

  // Copy path
  int i = 0;
  const char *p = start_path ? start_path : "/";
  while (p[i] && i < 255) {
    state->current_path[i] = p[i];
    i++;
  }
  state->current_path[i] = 0;

  win->user_data = state;
  win->on_paint = fb_paint;
  win->on_key = fb_key;
  win->on_click = fb_on_click;
}
