#include "keyboard.h"
#include "../kb_layouts/en_us.h"
#include "../kernel/idt.h"
#include "../kernel/io.h"

static int shift = 0;
static int caps = 0;

/* ── Ring buffer for interrupt-driven keyboard ─────────────── */
#define KB_BUF_SIZE 128
static char kb_buf[KB_BUF_SIZE];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;

static void kb_buf_push(char c) {
  int next = (kb_head + 1) % KB_BUF_SIZE;
  if (next != kb_tail) { /* drop if full */
    kb_buf[kb_head] = c;
    kb_head = next;
  }
}

char translate_scancode(unsigned char sc) {
  if (sc & 0x80) {
    unsigned char make = sc & 0x7F;

    if (make == 0x2A || make == 0x36)
      shift = 0;
    return 0;
  }

  if (sc == enterKey) {
    return '\n';
  }

  if (sc == 0x0E) {
    return '\b';
  }

  switch (sc) {
  case 0x2A:
  case 0x36:
    shift = 1;
    return 0;

  case 0x3A:
    caps = !caps;
    return 0;
  }

  char c = shift ? sc_shift[sc] : sc_normal[sc];
  if (!c)
    return 0;

  if (caps && c >= 'a' && c <= 'z')
    c -= 32;
  else if (caps && c >= 'A' && c <= 'Z' && !shift)
    c += 32;

  return c;
}

/* ── IRQ1 handler — called from IDT ──────────────────────── */
void keyboard_irq_handler(struct regs *r) {
  (void)r;
  unsigned char sc = inb(0x60);
  char c = translate_scancode(sc);
  if (c != 0) {
    kb_buf_push(c);
  }
}

/* ── Blocking read — waits for a character ────────────────── */
char keyboard_getchar(void) {
  while (kb_tail == kb_head) {
    /* Halt until next interrupt to save CPU */
    asm volatile("hlt");
  }
  char c = kb_buf[kb_tail];
  kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
  return c;
}

void init_keyboard(void) {
  shift = 0;
  caps = 0;
  kb_head = 0;
  kb_tail = 0;

  /* Install IRQ1 handler */
  irq_install_handler(1, keyboard_irq_handler);
}