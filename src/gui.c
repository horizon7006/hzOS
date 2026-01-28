#include "gui.h"
#include "terminal.h"
#include "mouse.h"
#include "filesystem.h"
#include "printf.h"

/* Simple text-mode GUI: draws a frame and moves a mouse cursor '@'.
   Later we can replace drawing code with real graphics. */

static void draw_frame(void) {
    terminal_clear();
    terminal_writeln("hzOS GUI (text mode prototype)");
    terminal_writeln("Use mouse to move '@'. Press Ctrl+Alt+Del (VM) to reset.");
    terminal_writeln("");
}

static void draw_cursor_at(int x, int y) {
    /* Crude: we can't move the hardware cursor yet, so we just
       redraw the entire screen using terminal APIs. For now,
       print cursor coordinates and rely on imagination :) */
    (void)x;
    (void)y;
}

void gui_start(void) {
    draw_frame();
    terminal_writeln("Launching mouse-controlled demo...");

    while (1) {
        mouse_state_t s = mouse_get_state();
        /* For now, just show coordinates in a status line. */
        terminal_writeln("");
        kprintf("mouse: x=%d y=%d buttons L=%d M=%d R=%d\n",
                s.x, s.y, s.left_down, s.middle_down, s.right_down);

        /* Simple delay loop to avoid spamming too fast. */
        for (volatile int i = 0; i < 1000000; i++) { }
    }
}

