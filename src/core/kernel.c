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
#include "acpi.h"
#include "apic.h"
#include "hpet.h"
#include "smp.h"

#include "../drivers/pci.h"
#include "../drivers/nvme.h"
#include "../drivers/xhci.h"
#include "../drivers/hda.h"
#include "../drivers/ahci.h"
#include "../drivers/rtl8139.h"
#include "../gui/graphics.h"
#include "../gui/gui.h"
#include "../drivers/serial.h"
#include "paging.h"
#include "process.h"
#include "vesa.h"
#include "limine.h"

// Set the base revision to 2, this is recommended as this is the latest
// revision described by the Limine boot protocol specification.
// See specification for further info.
// Set the base revision to 2.
__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

// Limine requests markers for v2 protocol
__attribute__((used, section(".requests_start")))
static volatile uint64_t requests_start_marker[4] = {
    0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf,
    0x785c6ed015d3e316, 0x181e920a7852b9d9
};

__attribute__((used, section(".requests_end")))
static volatile uint64_t requests_end_marker[2] = {
    0xadc0e0531bb10d03, 0x9572709f31764c62
};

// The Limine framebuffer request.
__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// The Limine HHDM request.
__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

// The Limine RSDP request.
__attribute__((used, section(".requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

// The Limine memory map request.
__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

uint64_t g_hhdm_offset = 0;

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

void kernel_main(void) {
  if (hhdm_request.response != NULL) {
    g_hhdm_offset = hhdm_request.response->offset;
  }

  // Initialize heap using first large usable memory chunk AS EARLY AS POSSIBLE
  int heap_initialized = 0;
  if (memmap_request.response != NULL) {
    // First pass: find a reasonably large chunk (>= 1MB)
    for (uint64_t i = 0; i < memmap_request.response->entry_count && !heap_initialized; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= 1*1024*1024) {
            uintptr_t heap_base = entry->base + g_hhdm_offset;
            extern void memory_init(uintptr_t base);
            memory_init(heap_base);
            heap_initialized = 1;
        }
    }
  }

  serial_init();
  
  // Debug: Show where heap is
  extern uintptr_t get_heap_base(void);
  serial_printf("hzOS: Kernel Main (Limine UEFI Mode)\n");
  serial_printf("Memory: HHDM offset = %p\n", (void*)g_hhdm_offset);
  serial_printf("Memory: Heap base = %p\n", (void*)get_heap_base());

  gdt_init();

  // Check if we have a framebuffer from Limine
  if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
    serial_printf("Limine: No framebuffer found!\n");
  } else {
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    serial_printf("Limine VESA: %dx%dx%dbpp at %p\n", fb->width, fb->height, fb->bpp, fb->address);
    vesa_init_limine(fb);
  }

  // paging_init(); // Disabled for 64-bit port (using identity map from boot_64.s)
  terminal_init();
  kprintf("hzOS: paging enabled and console initialized\n");

  kprintf("acpi: initializing...\n");
  void* rsdp_addr = NULL;
  if (rsdp_request.response != NULL) {
    rsdp_addr = (void*)rsdp_request.response->address;
  }
  

  extern void acpi_init_with_rsdp(void*);
  acpi_init_with_rsdp(rsdp_addr);

  kprintf("hpet: initializing...\n");
  hpet_init();

  kprintf("apic: initializing...\n");
  lapic_init();
  ioapic_init();

  kprintf("idt: initializing interrupts...\n");
  idt_init();

  // kprintf("smp: initializing...\n");
  // smp_init();

  kprintf("fs: initializing dynamic filesystem...\n");
  fs_init();

  command_init();

  kprintf("process: initializing process manager...\n");
  process_init();

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

  kprintf("hda: initializing...\n");
  hda_init();

  kprintf("ahci: initializing storage...\n");
  ahci_init();

  kprintf("net: initializing network (rtl8139)....\n");
  rtl8139_init();

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