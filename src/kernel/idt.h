#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT entry (8 bytes each) */
struct idt_entry {
  uint16_t base_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed));

/* Pointer passed to lidt */
struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

/* Saved register state pushed by ISR/IRQ stubs */
struct regs {
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, useresp, ss;
};

/* Initialize IDT, remap PIC, start PIT, install keyboard IRQ */
void idt_init(void);

/* Register/unregister a handler for a given IRQ (0-15) */
typedef void (*irq_handler_t)(struct regs *r);
void irq_install_handler(int irq, irq_handler_t handler);
void irq_uninstall_handler(int irq);

/* Global tick counter incremented by PIT IRQ0 */
extern volatile uint32_t pit_ticks;

#endif
