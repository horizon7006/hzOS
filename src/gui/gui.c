#include "terminal.h"
#include "window_manager.h"


void gui_start(void) {
  terminal_writeln("Starting Windows 7-style Desktop...");

  wm_init();

  // Desktop icons are rendered by window_manager.
  // No apps open on startup.

  while (wm_is_running()) {
    wm_update();
    wm_draw();

    /* Yield to allow QEMU display thread to catch up */
    __asm__ __volatile__("hlt");
  }
}
