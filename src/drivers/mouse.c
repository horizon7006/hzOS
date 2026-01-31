#include "mouse.h"
#include "io.h"
#include "isr.h"
#include "terminal.h"

/* Basic PS/2 mouse driver: decodes 3-byte packets from IRQ12. */

static mouse_state_t state = {40, 12, 0, 0, 0};
static int max_x = 79;
static int max_y = 24;

static uint8_t packet[3];
static int packet_index = 0;
static int mouse_initialized = 0;

void mouse_set_bounds(int width, int height) {
    max_x = width - 1;
    max_y = height - 1;
}

static void mouse_write(uint8_t value) {
    /* Wait for controller to be ready for command */
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xD4);   /* tell controller next byte is for mouse */
    while (inb(0x64) & 0x02) { }
    outb(0x60, value);
}

static uint8_t mouse_read(void) {
    while (!(inb(0x64) & 0x21)) { }
    return inb(0x60);
}

static void mouse_irq_handler(struct registers* regs) {
    (void)regs;
    uint8_t data = inb(0x60);

    packet[packet_index++] = data;
    if (packet_index < 3) {
        return;
    }
    packet_index = 0;

    int dx = (int8_t)packet[1];
    int dy = (int8_t)packet[2];

    state.x += dx;
    state.y -= dy;  /* screen y grows down, mouse packet grows up */

    if (state.x < 0) state.x = 0;
    if (state.y < 0) state.y = 0;
    if (state.x > max_x) state.x = max_x;
    if (state.y > max_y) state.y = max_y;

    state.left_down   = packet[0] & 0x1;
    state.right_down  = packet[0] & 0x2;
    state.middle_down = packet[0] & 0x4;
}

void mouse_init(void) {
    /* Enable auxiliary device (mouse) */
    uint8_t status = inb(0x64);
    (void)status;

    /* Enable mouse via controller */
    outb(0x64, 0xA8);

    /* Enable IRQ12 in controller */
    outb(0x64, 0x20);
    uint8_t compaq = inb(0x60);
    compaq |= 0x02;   /* enable mouse IRQ12 */
    outb(0x64, 0x60);
    outb(0x60, compaq);

    /* Reset and enable streaming */
    mouse_write(0xF6); /* set defaults */
    mouse_read();
    mouse_write(0xF4); /* enable data reporting */
    mouse_read();

    irq_register_handler(12, mouse_irq_handler);

    /* Unmask IRQ12 on PIC (slave, bit 4 of 0xA1) */
    uint8_t mask = inb(0xA1);
    mask &= ~(1 << 4);
    outb(0xA1, mask);

    mouse_initialized = 1;
    terminal_writeln("mouse: PS/2 mouse initialized");
}

mouse_state_t mouse_get_state(void) {
    return state;
}

void mouse_reset_position(int x, int y) {
    state.x = x;
    state.y = y;
    packet_index = 0;  // Reset packet state
}

