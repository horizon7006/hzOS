#include "terminal_app.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"
#include "../shell/command.h"
#include "../shell/terminal.h"


#define TERM_ROWS 16
#define TERM_COLS 52
#define MAX_CMD 64

#define DEFAULT_FG 0xFF00FF00
#define DEFAULT_BG 0xCC000000

typedef struct {
  char screen[TERM_ROWS][TERM_COLS + 1];
  uint32_t colors[TERM_ROWS][TERM_COLS];
  int cursor_row;
  int cursor_col;
  char cmd_line[MAX_CMD];
  int cmd_len;
  window_t *win;
  
  // ANSI parsing state
  int in_escape;
  char escape_buf[16];
  int escape_len;
  uint32_t current_color;
} terminal_app_state_t;

/* Global pointer for the sink callback */
static terminal_app_state_t *active_terminal = 0;

static const uint32_t ansi_colors[] = {
    0xFF000000, // 30 Black
    0xFFCC0000, // 31 Red
    0xFF4E9A06, // 32 Green
    0xFFC4A000, // 33 Yellow
    0xFF3465A4, // 34 Blue
    0xFF75507B, // 35 Magenta
    0xFF06989A, // 36 Cyan
    0xFFD3D7CF  // 37 White
};

static void term_scroll(terminal_app_state_t *state) {
  for (int i = 0; i < TERM_ROWS - 1; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      state->screen[i][j] = state->screen[i + 1][j];
      state->colors[i][j] = state->colors[i + 1][j];
    }
  }
  for (int j = 0; j < TERM_COLS; j++) {
    state->screen[TERM_ROWS - 1][j] = ' ';
    state->colors[TERM_ROWS - 1][j] = DEFAULT_FG;
  }
  if (state->cursor_row > 0)
    state->cursor_row--;
}

static void term_parse_escape(terminal_app_state_t *state) {
    state->escape_buf[state->escape_len] = 0;
    // Expected format: "[3Xm" or "[0m"
    if (state->escape_buf[0] == '[') {
        int color_code = 0;
        int i = 1;
        while (state->escape_buf[i] >= '0' && state->escape_buf[i] <= '9') {
            color_code = color_code * 10 + (state->escape_buf[i] - '0');
            i++;
        }
        if (state->escape_buf[i] == 'm') {
            if (color_code == 0) {
                state->current_color = DEFAULT_FG;
            } else if (color_code >= 30 && color_code <= 37) {
                state->current_color = ansi_colors[color_code - 30];
            } else if (color_code >= 90 && color_code <= 97) {
                // Bright colors mapped to same set for simplicity, but shifted
                state->current_color = ansi_colors[color_code - 90] | 0x00555555; 
            }
        }
    }
}

static void term_putc(terminal_app_state_t *state, char c) {
  if (state->in_escape) {
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
          // End of escape sequence
          if (state->escape_len < 15) {
              state->escape_buf[state->escape_len++] = c;
          }
          term_parse_escape(state);
          state->in_escape = 0;
      } else {
          if (state->escape_len < 15) {
              state->escape_buf[state->escape_len++] = c;
          }
      }
      return;
  }

  if (c == '\033') { // ESC
      state->in_escape = 1;
      state->escape_len = 0;
      return;
  }

  if (c == '\n' || c == '\r') {
    state->cursor_col = 0;
    state->cursor_row++;
  } else if (c >= 32 && c <= 126) {
    if (state->cursor_col < TERM_COLS) {
      state->screen[state->cursor_row][state->cursor_col] = c;
      state->colors[state->cursor_row][state->cursor_col] = state->current_color;
      state->cursor_col++;
    }
  }

  if (state->cursor_col >= TERM_COLS) {
    state->cursor_col = 0;
    state->cursor_row++;
  }
  if (state->cursor_row >= TERM_ROWS) {
    term_scroll(state);
  }
}

static void term_puts(terminal_app_state_t *state, const char *s) {
  while (*s)
    term_putc(state, *s++);
}

/* Sink callback for terminal output redirection */
static void terminal_app_sink(char c) {
  if (active_terminal) {
    term_putc(active_terminal, c);
  }
}

static void terminal_app_paint(window_t *win, uint32_t *buf, int stride,
                               int height) {
  (void)buf;
  (void)stride;
  (void)height;

  terminal_app_state_t *state = (terminal_app_state_t *)win->user_data;
  if (!state)
    return;

  /* Translucent black background */
  wm_fill_rect(win, 0, 0, win->width, win->height, 0xCC000000);

  /* Draw screen buffer */
  for (int i = 0; i < TERM_ROWS; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      char c = state->screen[i][j];
      if (c >= 32 && c <= 126) {
        char buf[2] = {c, 0};
        wm_draw_string(win, 4 + j * 8, 4 + i * 10, buf, state->colors[i][j]);
      }
    }
  }

  /* Draw command line prompt */
  int prompt_y = 4 + TERM_ROWS * 10 + 8;
  wm_draw_string(win, 4, prompt_y, "> ", DEFAULT_FG);

  char display_cmd[MAX_CMD + 2];
  int i;
  for (i = 0; i < state->cmd_len && i < MAX_CMD - 1; i++) {
    display_cmd[i] = state->cmd_line[i];
  }
  display_cmd[i++] = '_';
  display_cmd[i] = 0;
  wm_draw_string(win, 20, prompt_y, display_cmd, 0xFFFFFFFF);
}

static void terminal_app_key(window_t *win, char key) {
  terminal_app_state_t *state = (terminal_app_state_t *)win->user_data;
  if (!state)
    return;

  if (key == '\b') {
    if (state->cmd_len > 0) {
      state->cmd_len--;
      state->cmd_line[state->cmd_len] = 0;
    }
  } else if (key == '\n' || key == '\r') {
    state->cmd_line[state->cmd_len] = 0;

    /* Echo command to screen */
    term_puts(state, "> ");
    term_puts(state, state->cmd_line);
    term_putc(state, '\n');

    if (state->cmd_len > 0) {
      /* Set this terminal as active for output capture */
      active_terminal = state;
      terminal_set_sink(terminal_app_sink);

      /* Execute command */
      command_execute(state->cmd_line);

      /* Clear sink */
      terminal_set_sink(0);
      active_terminal = 0;
    }

    state->cmd_len = 0;
    state->cmd_line[0] = 0;
  } else if (key >= 32 && key <= 126) {
    if (state->cmd_len < MAX_CMD - 1) {
      state->cmd_line[state->cmd_len++] = key;
      state->cmd_line[state->cmd_len] = 0;
    }
  }
}

void terminal_app_create(void) {
  window_t *win = wm_create_window(80, 120, 440, 220, "Terminal");
  if (!win)
    return;

  win->bg_color = 0xCC000000;

  terminal_app_state_t *state =
      (terminal_app_state_t *)kmalloc_z(sizeof(terminal_app_state_t));

  /* Initialize screen buffer with spaces and colors */
  for (int i = 0; i < TERM_ROWS; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      state->screen[i][j] = ' ';
      state->colors[i][j] = DEFAULT_FG;
    }
    state->screen[i][TERM_COLS] = 0;
  }

  state->cursor_row = 0;
  state->cursor_col = 0;
  state->cmd_len = 0;
  state->cmd_line[0] = 0;
  state->win = win;
  state->in_escape = 0;
  state->escape_len = 0;
  state->current_color = DEFAULT_FG;

  /* Print welcome message */
  term_puts(state, "hzOS Terminal - Type 'help' for commands\n");

  win->user_data = state;
  win->on_paint = terminal_app_paint;
  win->on_key = terminal_app_key;
}
