#include "isr.h"
#include "io.h"
#include "terminal.h"

#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_INT32   0x0E
#define IDT_FLAG_RING0   0x00

extern void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

/* ISR handler function prototypes from assembly */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/* IRQ handlers from assembly */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

static irq_handler_t irq_handlers[16] = { 0 };

void irq_register_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

/* Called by common ISR stub */
void isr_handler(struct registers* regs) {
    terminal_writeln("Unhandled exception! Halting.");
    UNUSED(regs);
    for (;;);
}

/* Called by common IRQ stub */
void irq_handler(struct registers* regs) {
    int irq = regs->int_no - 32;

    if (irq >= 0 && irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](regs);
    }

    /* Send End of Interrupt (EOI) to PICs */
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);  // slave
    }
    outb(0x20, 0x20);      // master
}

void isr_install(void) {
    uint8_t flags = IDT_FLAG_PRESENT | IDT_FLAG_INT32 | IDT_FLAG_RING0;
    uint16_t sel = 0x08; // kernel code segment

    idt_set_gate(0,  (uint32_t)isr0,  sel, flags);
    idt_set_gate(1,  (uint32_t)isr1,  sel, flags);
    idt_set_gate(2,  (uint32_t)isr2,  sel, flags);
    idt_set_gate(3,  (uint32_t)isr3,  sel, flags);
    idt_set_gate(4,  (uint32_t)isr4,  sel, flags);
    idt_set_gate(5,  (uint32_t)isr5,  sel, flags);
    idt_set_gate(6,  (uint32_t)isr6,  sel, flags);
    idt_set_gate(7,  (uint32_t)isr7,  sel, flags);
    idt_set_gate(8,  (uint32_t)isr8,  sel, flags);
    idt_set_gate(9,  (uint32_t)isr9,  sel, flags);
    idt_set_gate(10, (uint32_t)isr10, sel, flags);
    idt_set_gate(11, (uint32_t)isr11, sel, flags);
    idt_set_gate(12, (uint32_t)isr12, sel, flags);
    idt_set_gate(13, (uint32_t)isr13, sel, flags);
    idt_set_gate(14, (uint32_t)isr14, sel, flags);
    idt_set_gate(15, (uint32_t)isr15, sel, flags);
    idt_set_gate(16, (uint32_t)isr16, sel, flags);
    idt_set_gate(17, (uint32_t)isr17, sel, flags);
    idt_set_gate(18, (uint32_t)isr18, sel, flags);
    idt_set_gate(19, (uint32_t)isr19, sel, flags);
    idt_set_gate(20, (uint32_t)isr20, sel, flags);
    idt_set_gate(21, (uint32_t)isr21, sel, flags);
    idt_set_gate(22, (uint32_t)isr22, sel, flags);
    idt_set_gate(23, (uint32_t)isr23, sel, flags);
    idt_set_gate(24, (uint32_t)isr24, sel, flags);
    idt_set_gate(25, (uint32_t)isr25, sel, flags);
    idt_set_gate(26, (uint32_t)isr26, sel, flags);
    idt_set_gate(27, (uint32_t)isr27, sel, flags);
    idt_set_gate(28, (uint32_t)isr28, sel, flags);
    idt_set_gate(29, (uint32_t)isr29, sel, flags);
    idt_set_gate(30, (uint32_t)isr30, sel, flags);
    idt_set_gate(31, (uint32_t)isr31, sel, flags);
}

static void pic_remap(void) {
    /* Remap PIC 1 to 0x20-0x27, PIC 2 to 0x28-0x2F */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void irq_install(void) {
    uint8_t flags = IDT_FLAG_PRESENT | IDT_FLAG_INT32 | IDT_FLAG_RING0;
    uint16_t sel = 0x08;

    pic_remap();

    idt_set_gate(32, (uint32_t)irq0,  sel, flags);
    idt_set_gate(33, (uint32_t)irq1,  sel, flags); // keyboard
    idt_set_gate(34, (uint32_t)irq2,  sel, flags);
    idt_set_gate(35, (uint32_t)irq3,  sel, flags);
    idt_set_gate(36, (uint32_t)irq4,  sel, flags);
    idt_set_gate(37, (uint32_t)irq5,  sel, flags);
    idt_set_gate(38, (uint32_t)irq6,  sel, flags);
    idt_set_gate(39, (uint32_t)irq7,  sel, flags);
    idt_set_gate(40, (uint32_t)irq8,  sel, flags);
    idt_set_gate(41, (uint32_t)irq9,  sel, flags);
    idt_set_gate(42, (uint32_t)irq10, sel, flags);
    idt_set_gate(43, (uint32_t)irq11, sel, flags);
    idt_set_gate(44, (uint32_t)irq12, sel, flags);
    idt_set_gate(45, (uint32_t)irq13, sel, flags);
    idt_set_gate(46, (uint32_t)irq14, sel, flags);
    idt_set_gate(47, (uint32_t)irq15, sel, flags);
}