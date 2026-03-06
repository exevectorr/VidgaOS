#ifndef KEYBOARD_H
#define KEYBOARD_H

char translate_scancode(unsigned char sc);
void init_keyboard(void);

/* Blocking read — returns next character from interrupt buffer */
char keyboard_getchar(void);

#endif
