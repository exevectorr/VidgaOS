#include "shell.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"
#include "../kernel/io.h"
#include "../kernel/vfs.h"
#include "commands.h"

static char input_buffer[256];
static int buffer_idx = 0;
static int current_design = 0x0F;

static void print_char(char c, int design) {
  char str[2] = {c, 0};
  print_msg(str, design);
}

static void print_prompt(int design) {
  vfs_cwd_t *cwd = vfs_get_cwd();
  print_msg("root", design);
  print_msg(cwd->path, design);
  print_msg("> ", design);
}

void shell_init(int design) {
  current_design = design;
  buffer_idx = 0;
  print_msg("VidgaOS Shell\n", design);
  print_prompt(design);
}

void shell_process_input(char c, int design) {
  if (c == '\n' || c == '\r') {
    print_msg("\n", design);
    input_buffer[buffer_idx] = '\0';

    if (buffer_idx > 0) {
      execute_command(input_buffer, design);
    }

    buffer_idx = 0;
    print_prompt(design);
  } else if (c == '\b' || c == 127) {
    if (buffer_idx > 0) {
      buffer_idx--;
      print_msg("\b \b", design);
    }
  } else if (buffer_idx < 255 && c >= 32) {
    input_buffer[buffer_idx++] = c;
    print_char(c, design);
  }
}

void shell_run(int design) {
  shell_init(design);

  while (1) {
    /* Use interrupt-driven blocking read instead of polling */
    char c = keyboard_getchar();
    if (c != 0) {
      shell_process_input(c, design);
    }
  }
}
