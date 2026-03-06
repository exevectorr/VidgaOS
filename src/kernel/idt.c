#include "idt.h"
#include "io.h"

/* ── IDT table ─────────────────────────────────────────────── */
static struct idt_entry idt[256];
struct idt_ptr idtp;

/* Defined in idt_asm.s */
extern void idt_load(void);

/* ISR stubs (exceptions 0-31) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* IRQ stubs (hardware interrupts, mapped to IDT 32-47) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* ── IDT helpers ───────────────────────────────────────────── */
static void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt[num].base_low = base & 0xFFFF;
  idt[num].base_high = (base >> 16) & 0xFFFF;
  idt[num].selector = sel;
  idt[num].zero = 0;
  idt[num].flags = flags;
}

/* ── ISR (exceptions) ──────────────────────────────────────── */
void isr_handler(struct regs *r) {
  /* For now, just acknowledge. Could print fault info later. */
  (void)r;
}

/* ── IRQ (hardware interrupts) ─────────────────────────────── */
static irq_handler_t irq_handlers[16] = {0};

void irq_install_handler(int irq, irq_handler_t handler) {
  if (irq >= 0 && irq < 16)
    irq_handlers[irq] = handler;
}

void irq_uninstall_handler(int irq) {
  if (irq >= 0 && irq < 16)
    irq_handlers[irq] = 0;
}

void irq_handler(struct regs *r) {
  irq_handler_t h = 0;
  int irq = r->int_no - 32;

  if (irq >= 0 && irq < 16)
    h = irq_handlers[irq];

  if (h)
    h(r);

  /* Send EOI to slave PIC if IRQ >= 8 */
  if (irq >= 8)
    outb(0xA0, 0x20);
  /* Always send EOI to master PIC */
  outb(0x20, 0x20);
}

/* ── PIC remapping ─────────────────────────────────────────── */
static void pic_remap(void) {
  /* Save masks */
  uint8_t mask1 = inb(0x21);
  uint8_t mask2 = inb(0xA1);

  /* Start init sequence (ICW1) */
  outb(0x20, 0x11);
  outb(0xA0, 0x11);

  /* ICW2: remap IRQ 0-7 → IDT 32-39, IRQ 8-15 → IDT 40-47 */
  outb(0x21, 0x20);
  outb(0xA1, 0x28);

  /* ICW3: tell master about slave on IRQ2, tell slave its cascade */
  outb(0x21, 0x04);
  outb(0xA1, 0x02);

  /* ICW4: 8086 mode */
  outb(0x21, 0x01);
  outb(0xA1, 0x01);

  /* Restore masks */
  outb(0x21, mask1);
  outb(0xA1, mask2);
}

/* ── PIT (Programmable Interval Timer) ─────────────────────── */
volatile uint32_t pit_ticks = 0;

static void pit_irq_handler(struct regs *r) {
  (void)r;
  pit_ticks++;
}

static void pit_init(void) {
  /* ~100 Hz: divisor = 1193180 / 100 = 11932 */
  uint16_t divisor = 11932;
  outb(0x43, 0x36);                  /* Channel 0, lo/hi, rate generator */
  outb(0x40, divisor & 0xFF);        /* Low byte */
  outb(0x40, (divisor >> 8) & 0xFF); /* High byte */

  irq_install_handler(0, pit_irq_handler);
}

/* ── Full IDT initialization ───────────────────────────────── */
void idt_init(void) {
  idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
  idtp.base = (uint32_t)&idt;

  /* Zero out the table */
  for (int i = 0; i < 256; i++) {
    idt_set_gate(i, 0, 0, 0);
  }

  /* Install ISR gates (exceptions 0-31), selector 0x08 = kernel code seg */
  idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
  idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
  idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
  idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
  idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
  idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
  idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
  idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
  idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
  idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
  idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
  idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
  idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
  idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
  idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
  idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
  idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
  idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
  idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
  idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
  idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
  idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
  idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
  idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
  idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
  idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
  idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
  idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
  idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
  idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
  idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
  idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

  /* Remap PIC before installing IRQ gates */
  pic_remap();

  /* Install IRQ gates (hardware interrupts 32-47) */
  idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
  idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
  idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
  idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
  idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
  idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
  idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
  idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
  idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
  idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
  idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
  idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
  idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
  idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
  idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
  idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

  /* Load IDT */
  idt_load();

  /* Start PIT timer */
  pit_init();
}
