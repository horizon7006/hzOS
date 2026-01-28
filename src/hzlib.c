#include "hzlib.h"
#include "terminal.h"

/* Very small stubs that draw inside the existing text-mode terminal.
   Real GUI backends can replace these later. */

void hz_window_open(hz_window_t* win) {
    (void)win;
    terminal_writeln("[hzlib] window_open (stub)");
}

void hz_window_draw_text(hz_window_t* win, int x, int y, const char* text) {
    (void)win;
    (void)x;
    (void)y;
    terminal_writeln(text);
}

int hz_event_poll(hz_event_t* ev) {
    (void)ev;
    return 0;
}

