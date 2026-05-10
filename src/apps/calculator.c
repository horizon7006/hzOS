#include "calculator.h"
#include "../gui/graphics.h"
#include "../gui/window_manager.h"
#include "../lib/string.h"

/* Calculator state - INTEGER ONLY (no SSE) */
typedef struct {
    long value1;
    long value2;
    char op;
    char display[16];
    int display_len;
    int new_input;
    int negative;
} calc_state_t;

static calc_state_t calc = {0, 0, 0, "0", 1, 1, 0};

/* Button layout: 4 columns x 5 rows */
static const char* buttons[] = {
    "C", "+/-", "%", "/",
    "7", "8", "9", "*",
    "4", "5", "6", "-",
    "1", "2", "3", "+",
    "0", "0", "=", ""
};

#define BTN_SIZE 40
#define BTN_GAP 5
#define DISPLAY_H 40
#define WIN_W (4 * BTN_SIZE + 5 * BTN_GAP)
#define WIN_H (DISPLAY_H + 5 * BTN_SIZE + 6 * BTN_GAP)

static void calc_clear(void) {
    calc.value1 = 0;
    calc.value2 = 0;
    calc.op = 0;
    calc.display[0] = '0';
    calc.display[1] = 0;
    calc.display_len = 1;
    calc.new_input = 1;
    calc.negative = 0;
}

static long calc_parse_display(void) {
    long result = 0;
    int neg = 0;
    
    for (int i = 0; i < calc.display_len; i++) {
        char c = calc.display[i];
        if (c == '-' && i == 0) {
            neg = 1;
        } else if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
        }
    }
    return neg ? -result : result;
}

static void calc_set_display(long val) {
    int neg = (val < 0);
    if (neg) val = -val;
    
    int i = 0;
    if (neg) calc.display[i++] = '-';
    
    if (val == 0) {
        calc.display[i++] = '0';
    } else {
        char tmp[16];
        int j = 0;
        while (val > 0 && j < 14) {
            tmp[j++] = '0' + (val % 10);
            val /= 10;
        }
        while (j > 0) calc.display[i++] = tmp[--j];
    }
    calc.display[i] = 0;
    calc.display_len = i;
}

static void calc_compute(void) {
    long result = calc.value1;
    switch (calc.op) {
        case '+': result = calc.value1 + calc.value2; break;
        case '-': result = calc.value1 - calc.value2; break;
        case '*': result = calc.value1 * calc.value2; break;
        case '/': 
            if (calc.value2 != 0) result = calc.value1 / calc.value2;
            else { calc.display[0] = 'E'; calc.display[1] = 'r'; calc.display[2] = 'r'; calc.display[3] = 0; calc.display_len = 3; return; }
            break;
        case '%':
            if (calc.value2 != 0) result = calc.value1 % calc.value2;
            else { calc.display[0] = 'E'; calc.display[1] = 'r'; calc.display[2] = 'r'; calc.display[3] = 0; calc.display_len = 3; return; }
            break;
    }
    calc.value1 = result;
    calc_set_display(result);
    calc.op = 0;
    calc.new_input = 1;
}

static void calc_input(char c) {
    if (c >= '0' && c <= '9') {
        if (calc.new_input) {
            calc.display[0] = c;
            calc.display[1] = 0;
            calc.display_len = 1;
            calc.new_input = 0;
        } else if (calc.display_len < 14) {
            calc.display[calc.display_len++] = c;
            calc.display[calc.display_len] = 0;
        }
    } else if (c == 'n') {  /* negate */
        long val = calc_parse_display();
        calc_set_display(-val);
    } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
        if (calc.op) calc_compute();
        calc.value1 = calc_parse_display();
        calc.op = c;
        calc.new_input = 1;
    } else if (c == '=') {
        if (calc.op) {
            calc.value2 = calc_parse_display();
            calc_compute();
        }
    } else if (c == 'C') {
        calc_clear();
    }
}

static void calculator_paint(window_t *win, uint32_t *buf, int stride, int height) {
    (void)buf;
    (void)stride;
    (void)height;
    
    /* Background */
    wm_fill_rect(win, 0, 0, win->width, win->height, 0xFF1A1A2E);
    
    /* Display area */
    wm_fill_rect(win, BTN_GAP, BTN_GAP, WIN_W - 2*BTN_GAP, DISPLAY_H - BTN_GAP, 0xFF16213E);
    wm_draw_string(win, WIN_W - 10 - calc.display_len * 8, BTN_GAP + 12, calc.display, 0xFF00FF88);
    
    /* Buttons */
    int idx = 0;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 4; col++) {
            if (buttons[idx][0] == 0) { idx++; continue; }
            
            int bx = BTN_GAP + col * (BTN_SIZE + BTN_GAP);
            int by = DISPLAY_H + BTN_GAP + row * (BTN_SIZE + BTN_GAP);
            
            uint32_t color = 0xFF2D4A6B;
            char c = buttons[idx][0];
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') color = 0xFF3D5A80;
            else if (c == '=') color = 0xFF00AA55;
            else if (c == 'C') color = 0xFFAA3333;
            
            wm_fill_rect(win, bx, by, BTN_SIZE, BTN_SIZE, color);
            wm_draw_string(win, bx + BTN_SIZE/2 - 4, by + BTN_SIZE/2 - 4, buttons[idx], 0xFFFFFFFF);
            idx++;
        }
    }
}

static void calculator_click(window_t *win, int x, int y) {
    (void)win;
    
    y -= DISPLAY_H;
    if (y < 0) return;
    
    int col = (x - BTN_GAP) / (BTN_SIZE + BTN_GAP);
    int row = (y - BTN_GAP) / (BTN_SIZE + BTN_GAP);
    
    if (col < 0 || col >= 4 || row < 0 || row >= 5) return;
    
    int idx = row * 4 + col;
    if (idx < 20 && buttons[idx][0] != 0) {
        const char* btn = buttons[idx];
        if (btn[0] == '+' && btn[1] == '/') {
            calc_input('n');  /* negate */
        } else {
            calc_input(btn[0]);
        }
    }
}

static void calculator_key(window_t *win, char key) {
    (void)win;
    if ((key >= '0' && key <= '9') || 
        key == '+' || key == '-' || key == '*' || key == '/' || key == '%' ||
        key == '=' || key == '\n') {
        if (key == '\n') key = '=';
        calc_input(key);
    } else if (key == 'c' || key == 'C' || key == 27) {
        calc_clear();
    }
}

void calculator_create(void) {
    calc_clear();
    
    window_t *win = wm_create_window(250, 100, WIN_W, WIN_H, "Calculator");
    if (!win) return;
    
    win->bg_color = 0xFF1A1A2E;
    win->on_paint = calculator_paint;
    win->on_click = calculator_click;
    win->on_key = calculator_key;
}

