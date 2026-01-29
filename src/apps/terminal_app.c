#include "terminal_app.h"
#include "../gui/window_manager.h"
#include "../lib/memory.h"
#include "../shell/command.h"
#include "../shell/terminal.h"

#define TERM_ROWS 25
#define TERM_COLS 50

typedef struct {
    char lines[TERM_ROWS][TERM_COLS];
    int current_row;
    int current_col;
    char cmd_buf[128];
    int cmd_len;
} term_state_t;

static term_state_t* g_term_capture = 0;

static void term_putc(term_state_t* st, char c);

static void gui_sink(char c) {
    if (g_term_capture) {
        term_putc(g_term_capture, c);
    }
}

static void term_paint(window_t* win, uint32_t* buf, int stride, int height) {
    (void)buf; (void)stride; (void)height;
    term_state_t* st = (term_state_t*)win->user_data;
    
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFF000000);
    
    for(int i=0; i<TERM_ROWS; i++) {
        wm_draw_string(win, 2, 2 + i*10, st->lines[i], 0xFF00FF00); 
    }
    
    wm_draw_string(win, 2 + st->current_col * 8, 2 + st->current_row * 10, "_", 0xFF00FF00);
}

static void term_newline(term_state_t* st) {
    if (st->current_row < TERM_ROWS - 1) {
        st->current_row++;
    } else {
        for(int i=0; i<TERM_ROWS-1; i++) {
            memcpy(st->lines[i], st->lines[i+1], TERM_COLS);
        }
        memset(st->lines[TERM_ROWS-1], 0, TERM_COLS);
    }
    st->current_col = 0;
}

static void term_putc(term_state_t* st, char c) {
    if (c == '\n') {
        term_newline(st);
    } else if (c == '\b') {
        if (st->current_col > 0) {
             st->current_col--;
             st->lines[st->current_row][st->current_col] = ' '; 
        } else if (st->current_row > 0) {
            st->current_row--;
            st->current_col = TERM_COLS - 1;
            st->lines[st->current_row][st->current_col] = ' ';
        }
    } else if (c == '\r') {
        st->current_col = 0;
    } else {
        if (st->current_col < TERM_COLS - 1) {
            st->lines[st->current_row][st->current_col++] = c;
            st->lines[st->current_row][st->current_col] = 0;
        }
    }
}

static void term_print(term_state_t* st, const char* str) {
    while(*str) term_putc(st, *str++);
}

static void term_key(window_t* win, char c) {
    term_state_t* st = (term_state_t*)win->user_data;
    
    if (c == '\n') {
        st->cmd_buf[st->cmd_len] = 0;
        
        term_putc(st, '\n');
        
        if (st->cmd_len > 0) {
             g_term_capture = st;
             terminal_set_sink(gui_sink);
             
             command_execute(st->cmd_buf);
             
             terminal_set_sink(0);
             g_term_capture = 0;
        }
        
        st->cmd_len = 0;
        term_print(st, "> ");
    } 
    else if (c == '\b') {
        if (st->cmd_len > 0) {
            st->cmd_len--;
            if (st->current_col > 2) { 
                 st->current_col--;
                 st->lines[st->current_row][st->current_col] = 0; 
            }
        }
    }
    else {
        if (st->cmd_len < 127) {
            st->cmd_buf[st->cmd_len++] = c;
            term_putc(st, c);
        }
    }
}

void terminal_app_create() {
    window_t* win = wm_create_window(100, 300, 420, 260, "Terminal");
    win->bg_color = 0xFF000000;
    
    term_state_t* st = (term_state_t*)kmalloc_z(sizeof(term_state_t));
    win->user_data = st;
    win->on_paint = term_paint;
    win->on_key = term_key;
    
    term_print(st, "hzOS Terminal [v1.0]\n");
    term_print(st, "> ");
}
