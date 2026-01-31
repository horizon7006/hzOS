#include "common.h"
#include "isr.h"
#include "io.h"
#include "printf.h"

static uint32_t tick = 0;

static void timer_callback(struct registers* regs) {
    (void)regs;
    tick++;
}

void timer_init(uint32_t frequency) {
    // Register timer callback (redundant since scheduler handles IRQ 0, but good for counting ticks)
    irq_register_handler(0, timer_callback);

    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency.
    uint32_t divisor = 1193180 / frequency;

    // Send the command byte.
    outb(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    outb(0x40, l);
    outb(0x40, h);
    
    kprintf("timer: initialized at %d Hz\n", frequency);
}
