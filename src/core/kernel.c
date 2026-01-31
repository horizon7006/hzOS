#include "command.h"
#include "common.h"
#include "ext2.h"
#include "filesystem.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "mouse.h"
#include "printf.h"
#include "ramdisk.h"
#include "terminal.h"

#include "../drivers/pci.h"
#include "../drivers/nvme.h"
#include "../drivers/xhci.h"
#include "../drivers/rtl8139.h"
#include "../gui/graphics.h"
#include "../gui/gui.h"
#include "../drivers/serial.h"
#include "multiboot.h"
#include "paging.h"
#include "process.h"
#include "vesa.h"

extern uint8_t _binary_ext2_img_start[];
extern uint8_t _binary_ext2_img_end[];

/* Minimal VESA TTY state */
static int tty_x = 0;
static int tty_y = 0;
static char tty_line_buf[128];
static int tty_line_len = 0;

static void tty_sink(char c) {
  if (c == '\n') {
    tty_x = 0;
    tty_y += 10;
    if (tty_y >= vesa_height - 10) {
      tty_y = 0;
      gfx_fill_rect(0, 0, vesa_width, vesa_height, 0xFF000000);
    }
  } else if (c == '\b') {
    if (tty_x >= 8) {
      tty_x -= 8;
      gfx_fill_rect(tty_x, tty_y, 8, 8, 0xFF000000);
    }
  } else {
    gfx_draw_char(tty_x, tty_y, c, 0xFF00FF00); // Terminal Green
    tty_x += 8;
    if (tty_x >= vesa_width) {
      tty_x = 0;
      tty_y += 10;
    }
  }
}

static void tty_key_handler(char c) {
  // Determine if backspace
  if (c == '\b') {
    if (tty_line_len > 0) {
      tty_line_len--;
      tty_sink('\b');
    }
    return;
  }

  // Echo
  tty_sink(c);

  // Buffer
  if (c == '\n') {
    tty_line_buf[tty_line_len] = 0;
    command_execute(tty_line_buf);
    tty_line_len = 0;
    kprintf("> ");
  } else {
    if (tty_line_len < 127) {
      tty_line_buf[tty_line_len++] = c;
    }
  }
}

void kernel_main(uint64_t magic, uint64_t addr) {
  serial_init();
  serial_printf("hzOS: Kernel Main. Magic=%lx Addr=%lx\n", magic, addr);

  gdt_init();

  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    serial_printf("VESA: Initializing...\n");
    vesa_init((multiboot_info_t *)addr);
  }

  // paging_init(); // Disabled for 64-bit port (using identity map from boot_64.s)
  terminal_init();
  kprintf("hzOS: paging enabled and console initialized\n");

  kprintf("fs: initializing dynamic filesystem...\n");
  fs_init();

  command_init();

  kprintf("process: initializing process manager...\n");
  process_init();

  kprintf("idt: initializing interrupts...\n");
  idt_init();

  kprintf("input: initializing keyboard and mouse...\n");
  keyboard_init();
  mouse_init();

  kprintf("scheduler: starting timer...\n");
  extern void timer_init(uint32_t frequency);
  timer_init(20); // 20Hz scheduler - ultra stable

  kprintf("pci: scanning bus...\n");
  pci_init();

  kprintf("nvme: initializing storage...\n");
  nvme_init();

  kprintf("xhci: initializing usb...\n");
  xhci_init();

  // Temporarily disabled for stability isolation
  // kprintf("ahci: initializing storage...\n");
  // ahci_init();

  // kprintf("net: initializing network (rtl8139)...\n");
  // rtl8139_init();

  // Auto-mount ramdisk for binaries/apps
  kprintf("kernel: initializing ramdisk...\n");
  ramdisk_init(_binary_ext2_img_start,
               _binary_ext2_img_end - _binary_ext2_img_start);
  ext2_mount_from_ramdisk();

  kprintf("hzOS: Finalizing initialization...\n");

  // START SCHEDULER NOW (Global Interrupts)
  __asm__ __volatile__("sti");

  serial_printf("Starting GUI...\n");
  gui_start();

  // GUI Returned - Switch to TTY Mode
  serial_printf("GUI Exited. Switching to TTY...\n");

  // Clear Screen
  gfx_fill_rect(0, 0, vesa_width, vesa_height, 0xFF000000);

  // Setup TTY Sink for kprintf
  terminal_set_sink(tty_sink);

  // Hook Keyboard
  keyboard_set_callback(tty_key_handler);

  kprintf("hzOS TTY Shell\n> ");

  for (;;) {
    __asm__ __volatile__("hlt");
  }
}