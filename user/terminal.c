#include "libhz.h"

#define MAX_LINE 64
#define TERM_ROWS 12
#define TERM_COLS 50

char screen_buf[TERM_ROWS][TERM_COLS];
int cursor_row = 0;
int cursor_col = 0;

char cmd_line[MAX_LINE];
int cmd_len = 0;

void scroll() {
    for (int i = 0; i < TERM_ROWS - 1; i++) {
        for (int j = 0; j < TERM_COLS; j++) {
            screen_buf[i][j] = screen_buf[i+1][j];
        }
    }
    for (int j = 0; j < TERM_COLS; j++) screen_buf[TERM_ROWS-1][j] = ' ';
    if (cursor_row > 0) cursor_row--;
}

void putc_internal(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c >= 32 && c <= 126) {
        screen_buf[cursor_row][cursor_col++] = c;
    }

    if (cursor_col >= TERM_COLS) {
        cursor_col = 0;
        cursor_row++;
    }
    if (cursor_row >= TERM_ROWS) {
        scroll();
    }
}

void redraw(hz_window_t* win) {
    hz_window_fill_rect(win, 0, 0, win->width, win->height, 0xFF000000);
    
    for (int i = 0; i < TERM_ROWS; i++) {
        char line[TERM_COLS + 1];
        for (int j = 0; j < TERM_COLS; j++) line[j] = screen_buf[i][j];
        line[TERM_COLS] = 0;
        hz_window_draw_text(win, 10, 10 + i * 16, line, 0xFF00FF00);
    }

    char prompt[MAX_LINE + 4];
    int p_idx = 0;
    prompt[p_idx++] = '>';
    prompt[p_idx++] = ' ';
    for (int i = 0; i < cmd_len; i++) prompt[p_idx++] = cmd_line[i];
    prompt[p_idx++] = '_';
    prompt[p_idx] = 0;
    
    hz_window_draw_text(win, 10, 10 + TERM_ROWS * 16 + 10, prompt, 0xFFFFFFFF);
}

int main() {
    hz_window_t win;
    win.x = 50; win.y = 150; win.width = 440; win.height = 280;
    win.title = "Terminal Bridge";
    
    for (int i = 0; i < TERM_ROWS; i++)
        for (int j = 0; j < TERM_COLS; j++) screen_buf[i][j] = ' ';

    hz_window_open(&win);
    redraw(&win);

    int running = 1;
    while (running) {
        int c;
        while ((c = hz_terminal_get_char()) != -1) {
            putc_internal((char)c);
            redraw(&win);
        }

        hz_event_t ev;
        if (hz_event_poll(&win, &ev)) {
            if (ev.type == HZ_EVENT_QUIT) {
                running = 0;
            } else if (ev.type == HZ_EVENT_KEY) {
                if (ev.key == '\b') {
                    if (cmd_len > 0) cmd_len--;
                } else if (ev.key == '\n') {
                    cmd_line[cmd_len] = 0;
                    if (cmd_len > 0) hz_shell_exec(cmd_line);
                    cmd_len = 0;
                } else if (ev.key >= 32 && ev.key <= 126) {
                    if (cmd_len < MAX_LINE - 2) cmd_line[cmd_len++] = ev.key;
                }
                redraw(&win);
            }
        }
    }
    return 0;
}
