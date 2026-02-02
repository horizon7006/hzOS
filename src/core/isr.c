#include "isr.h"
#include "io.h"
#include "terminal.h"

#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_INT32   0x0E
#define IDT_FLAG_RING0   0x00

extern void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);

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
extern void isr80();

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

#include "serial.h"

static const char* exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overfow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
    "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
    "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
    "Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

/* Called by common ISR stub */
void isr_handler(struct registers* regs) {
    static int isr_panic_active = 0;
    if (isr_panic_active) {
        // Recursive fault during panic! Halt immediately to avoid QEMU shutdown.
        while(1) { __asm__ __volatile__("hlt"); }
    }
    isr_panic_active = 1;

    serial_printf("\n==================================================\n");
    serial_printf("EXCEPTION %d: %s\n", regs->int_no, 
                 regs->int_no < 32 ? exception_messages[regs->int_no] : "Unknown");
    serial_printf("Error code: 0x%x\n", regs->err_code);
    serial_printf("RIP: 0x%lx, CS: 0x%lx, RFLAGS: 0x%lx\n", regs->rip, regs->cs, regs->rflags);
    serial_printf("RAX: 0x%lx, RBX: 0x%lx, RCX: 0x%lx, RDX: 0x%lx\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    serial_printf("RSI: 0x%lx, RDI: 0x%lx, RBP: 0x%lx, RSP: 0x%lx\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);
    serial_printf("R8: 0x%lx, R9: 0x%lx, R10: 0x%lx, R11: 0x%lx\n", regs->r8, regs->r9, regs->r10, regs->r11);
    serial_printf("R12: 0x%lx, R13: 0x%lx, R14: 0x%lx, R15: 0x%lx\n", regs->r12, regs->r13, regs->r14, regs->r15);
    serial_printf("DS: 0x%lx\n", regs->ds);
    
    if (regs->int_no == 14) {
        uint64_t cr2;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        serial_printf("Page Fault Address (CR2): 0x%lx\n", cr2);
    }
    serial_printf("==================================================\n");

    terminal_writeln("System Panic! See serial log for details.");
    for (;;);
}

/* Forward declaration from process.c */
struct registers* scheduler_schedule(struct registers* regs);

/* Called by common IRQ stub */
struct registers* irq_handler(struct registers* regs) {
    int irq = regs->int_no - 32;

    if (irq >= 0 && irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](regs);
    }

    /* Send EOI to LAPIC */
    extern void lapic_eoi(void);
    lapic_eoi();

    // If timer interrupt, call scheduler
    if (irq == 0) {
        return scheduler_schedule(regs);
    }

    return regs;
}

void isr_install(void) {
    uint8_t flags = IDT_FLAG_PRESENT | IDT_FLAG_INT32 | IDT_FLAG_RING0;
    uint16_t sel = 0x08; // kernel code segment

    idt_set_gate(0,  (uint64_t)isr0,  sel, flags);
    idt_set_gate(1,  (uint64_t)isr1,  sel, flags);
    idt_set_gate(2,  (uint64_t)isr2,  sel, flags);
    idt_set_gate(3,  (uint64_t)isr3,  sel, flags);
    idt_set_gate(4,  (uint64_t)isr4,  sel, flags);
    idt_set_gate(5,  (uint64_t)isr5,  sel, flags);
    idt_set_gate(6,  (uint64_t)isr6,  sel, flags);
    idt_set_gate(7,  (uint64_t)isr7,  sel, flags);
    idt_set_gate(8,  (uint64_t)isr8,  sel, flags);
    idt_set_gate(9,  (uint64_t)isr9,  sel, flags);
    idt_set_gate(10, (uint64_t)isr10, sel, flags);
    idt_set_gate(11, (uint64_t)isr11, sel, flags);
    idt_set_gate(12, (uint64_t)isr12, sel, flags);
    idt_set_gate(13, (uint64_t)isr13, sel, flags);
    idt_set_gate(14, (uint64_t)isr14, sel, flags);
    idt_set_gate(15, (uint64_t)isr15, sel, flags);
    idt_set_gate(16, (uint64_t)isr16, sel, flags);
    idt_set_gate(17, (uint64_t)isr17, sel, flags);
    idt_set_gate(18, (uint64_t)isr18, sel, flags);
    idt_set_gate(19, (uint64_t)isr19, sel, flags);
    idt_set_gate(20, (uint64_t)isr20, sel, flags);
    idt_set_gate(21, (uint64_t)isr21, sel, flags);
    idt_set_gate(22, (uint64_t)isr22, sel, flags);
    idt_set_gate(23, (uint64_t)isr23, sel, flags);
    idt_set_gate(24, (uint64_t)isr24, sel, flags);
    idt_set_gate(25, (uint64_t)isr25, sel, flags);
    idt_set_gate(26, (uint64_t)isr26, sel, flags);
    idt_set_gate(27, (uint64_t)isr27, sel, flags);
    idt_set_gate(28, (uint64_t)isr28, sel, flags);
    idt_set_gate(29, (uint64_t)isr29, sel, flags);
    idt_set_gate(30, (uint64_t)isr30, sel, flags);
    idt_set_gate(31, (uint64_t)isr31, sel, flags);

    // Syscall: Vector 0x80, User Mode (Ring 3)
    idt_set_gate(0x80, (uint64_t)isr80, sel, flags | 0x60);
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

    idt_set_gate(32, (uint64_t)irq0,  sel, flags);
    idt_set_gate(33, (uint64_t)irq1,  sel, flags); // keyboard
    idt_set_gate(34, (uint64_t)irq2,  sel, flags);
    idt_set_gate(35, (uint64_t)irq3,  sel, flags);
    idt_set_gate(36, (uint64_t)irq4,  sel, flags);
    idt_set_gate(37, (uint64_t)irq5,  sel, flags);
    idt_set_gate(38, (uint64_t)irq6,  sel, flags);
    idt_set_gate(39, (uint64_t)irq7,  sel, flags);
    idt_set_gate(40, (uint64_t)irq8,  sel, flags);
    idt_set_gate(41, (uint64_t)irq9,  sel, flags);
    idt_set_gate(42, (uint64_t)irq10, sel, flags);
    idt_set_gate(43, (uint64_t)irq11, sel, flags);
    idt_set_gate(44, (uint64_t)irq12, sel, flags);
    idt_set_gate(45, (uint64_t)irq13, sel, flags);
    idt_set_gate(46, (uint64_t)irq14, sel, flags);
    idt_set_gate(47, (uint64_t)irq15, sel, flags);
}