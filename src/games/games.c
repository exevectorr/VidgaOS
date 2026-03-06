#include "games.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"
#include "../kernel/idt.h"
#include "../kernel/rand.h"

/* ── Helper: read a number from keyboard input ───────────── */
static int read_number(int design) {
  char buf[8];
  int idx = 0;

  while (1) {
    char c = keyboard_getchar();

    if (c == '\n' || c == '\r') {
      /* Print newline and return the number */
      print_msg("\n", design);
      buf[idx] = '\0';
      break;
    } else if (c == '\b' || c == 127) {
      if (idx > 0) {
        idx--;
        print_msg("\b \b", design);
      }
    } else if (c >= '0' && c <= '9' && idx < 6) {
      buf[idx++] = c;
      char str[2] = {c, 0};
      print_msg(str, design);
    }
  }

  /* Convert string to int */
  int result = 0;
  for (int i = 0; i < idx; i++) {
    result = result * 10 + (buf[i] - '0');
  }
  return result;
}

/* ── Dice game ────────────────────────────────────────────── */
void game_dice(int design) {
  /* Re-seed from current ticks for extra entropy each play */
  srand(pit_ticks);

  int roll1 = rand_range(1, 6);
  int roll2 = rand_range(1, 6);

  print_msg("Rolling two dice...\n", design);
  print_msg("Die 1: ", design);
  print_msg_num(roll1, design);
  print_msg("\n", design);

  print_msg("Die 2: ", design);
  print_msg_num(roll2, design);
  print_msg("\n", design);

  print_msg("Total: ", design);
  print_msg_num(roll1 + roll2, design);
  print_msg("\n", design);
}

/* ── Number guessing game (fully interactive) ─────────────── */
void game_number_guess(int design) {
  srand(pit_ticks);
  int target = rand_range(1, 100);
  int guesses = 0;
  int highlight = vga_color(VGA_YELLOW, VGA_BLUE);
  int success = vga_color(VGA_LIGHT_GREEN, VGA_BLUE);
  int hint = vga_color(VGA_LIGHT_CYAN, VGA_BLUE);

  print_msg("=== Number Guessing Game ===\n", highlight);
  print_msg("I'm thinking of a number between 1 and 100.\n", design);
  print_msg("Type your guess and press Enter.\n\n", design);

  while (1) {
    print_msg("Your guess: ", design);
    int guess = read_number(design);
    guesses++;

    if (guess < 1 || guess > 100) {
      print_msg("Please enter a number between 1 and 100.\n", hint);
      guesses--;
      continue;
    }

    if (guess < target) {
      print_msg("Too low! Try higher.\n", hint);
    } else if (guess > target) {
      print_msg("Too high! Try lower.\n", hint);
    } else {
      print_msg("Correct! You got it in ", success);
      print_msg_num(guesses, success);
      print_msg(" guesses!\n", success);
      break;
    }
  }
}

/* ── Game list ────────────────────────────────────────────── */
void game_list(int design) {
  (void)design;  // Suppress unused parameter warning
  int color = vga_color(VGA_CYAN, VGA_BLUE);
  print_msg("Available Games:\n", color);
  print_msg("  game play dice       - Roll two dice\n", color);
  print_msg("  game play guess      - Number guessing game\n", color);
}
