#include "./drivers/keyboard.h"
#include "./drivers/vga.h"
#include "./kernel/idt.h"
#include "./kernel/rand.h"
#include "./kernel/ata.h"
#include "./kernel/fat32.h"
#include "./kernel/vfs.h"
#include "./shell/shell.h"

void kernel_main() {
  vga_init();
  int design = vga_color(VGA_WHITE, VGA_BLUE);
  clear_screen(design);
  print_msg("VidgaOS running in 32bit mode.\n", design);

  /* Set up IDT, PIC, PIT — then enable interrupts */
  idt_init();
  init_keyboard();

  /* Initialize ATA and FAT32 filesystem */
  ata_init();
  fat32_init();
  
  /* Initialize VFS (Virtual File System) */
  vfs_init();
  
  /* Create system directories if they don't exist */
  vfs_mkdir("/system");
  vfs_mkdir("/personal");
  vfs_mkdir("/tmp");
  
  /* Change to default directory (/system) */
  vfs_cd("/system");

  /* Enable interrupts now that everything is installed */
  asm volatile("sti");

  /* Seed RNG from timer ticks (ticks accumulated during boot) */
  srand(pit_ticks);

  shell_run(design);
}
