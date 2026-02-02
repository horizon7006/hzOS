#include "mouse.h"
#include "io.h"
#include "isr.h"
#include "terminal.h"
#include "../lib/printf.h"

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

static void mouse_wait(uint8_t type) {
    int timeout = 100000;
    if (type == 0) {
        while (timeout--) { // data
            if ((inb(0x64) & 1) == 1) {
                return;
            }
        }
        return;
    } else {
        while (timeout--) { // signal
            if ((inb(0x64) & 2) == 0) {
                return;
            }
        }
        return;
    }
}

static void mouse_write(uint8_t value) {
    /* Wait for controller to be ready for command */
    mouse_wait(1);
    outb(0x64, 0xD4);   /* tell controller next byte is for mouse */
    mouse_wait(1);
    outb(0x60, value);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

static void mouse_irq_handler(struct registers* regs) {
    (void)regs;
    uint8_t data = inb(0x60);

    // Sync: if we are expecting the first byte (packet_index == 0),
    // verify that bit 3 is set (always 1 for standard PS/2 mouse packet).
    // If not, we are out of sync or getting garbage.
    if (packet_index == 0 && !(data & 0x08)) {
        return; 
    }

    packet[packet_index++] = data;
    if (packet_index < 3) {
        return;
    }
    packet_index = 0;

    // Packet breakdown:
    // Byte 1: Yoverflow Xoverflow Ysign Xsign 1 Middle Right Left
    // Byte 2: X Movement
    // Byte 3: Y Movement

    int dx = (int8_t)packet[1];
    int dy = (int8_t)packet[2];

    // Handle overflow bits if necessary (often ignored in simple drivers, but good to know)
    if (packet[0] & 0x40 || packet[0] & 0x80) {
        dx = 0; // discard erratic movement
        dy = 0;
    }

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
    mouse_wait(1);
    outb(0x64, 0xA8);

    /* Enable IRQ12 in controller */
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t compaq = inb(0x60);
    compaq |= 0x02;   /* enable mouse IRQ12 */
    
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, compaq);

    /* Reset and enable streaming */
    mouse_write(0xF6); /* set defaults */
    mouse_read();      /* acknowledge */

    mouse_write(0xF4); /* enable data reporting */
    mouse_read();      /* acknowledge */

    irq_register_handler(12, mouse_irq_handler);

    /* Route IRQ12 to BSP via IOAPIC */
    // Vector 32+12 = 44 (0x2C)
    // Destination 0 (BSP)
    extern void ioapic_set_irq(uint8_t irq, uint32_t apic_id, uint32_t flags_vector);
    ioapic_set_irq(12, 0, 44);

    mouse_initialized = 1;
    terminal_writeln("mouse: PS/2 mouse initialized with optimized driver");
}

mouse_state_t mouse_get_state(void) {
    return state;
}

void mouse_reset_position(int x, int y) {
    state.x = x;
    state.y = y;
    packet_index = 0;  // Reset packet state
}


