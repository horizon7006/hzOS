#include "terminal_app.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"
#include "../shell/command.h"
#include "../shell/terminal.h"


#define TERM_ROWS 16
#define TERM_COLS 52
#define MAX_CMD 64

typedef struct {
  char screen[TERM_ROWS][TERM_COLS + 1];
  int cursor_row;
  int cursor_col;
  char cmd_line[MAX_CMD];
  int cmd_len;
  window_t *win;
} terminal_app_state_t;

/* Global pointer for the sink callback */
static terminal_app_state_t *active_terminal = 0;

static void term_scroll(terminal_app_state_t *state) {
  for (int i = 0; i < TERM_ROWS - 1; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      state->screen[i][j] = state->screen[i + 1][j];
    }
  }
  for (int j = 0; j < TERM_COLS; j++) {
    state->screen[TERM_ROWS - 1][j] = ' ';
  }
  if (state->cursor_row > 0)
    state->cursor_row--;
}

static void term_putc(terminal_app_state_t *state, char c) {
  if (c == '\n' || c == '\r') {
    state->cursor_col = 0;
    state->cursor_row++;
  } else if (c >= 32 && c <= 126) {
    if (state->cursor_col < TERM_COLS) {
      state->screen[state->cursor_row][state->cursor_col++] = c;
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
    state->screen[i][TERM_COLS] = 0;
    wm_draw_string(win, 4, 4 + i * 10, state->screen[i], 0xFF00FF00);
  }

  /* Draw command line prompt */
  int prompt_y = 4 + TERM_ROWS * 10 + 8;
  wm_draw_string(win, 4, prompt_y, "> ", 0xFFFFFFFF);

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

  /* Initialize screen buffer with spaces */
  for (int i = 0; i < TERM_ROWS; i++) {
    for (int j = 0; j < TERM_COLS; j++) {
      state->screen[i][j] = ' ';
    }
    state->screen[i][TERM_COLS] = 0;
  }

  state->cursor_row = 0;
  state->cursor_col = 0;
  state->cmd_len = 0;
  state->cmd_line[0] = 0;
  state->win = win;

  /* Print welcome message */
  term_puts(state, "hzOS Terminal - Type 'help' for commands\n");

  win->user_data = state;
  win->on_paint = terminal_app_paint;
  win->on_key = terminal_app_key;
}
