#include "commands.h"
#include "../drivers/vga.h"
#include "../kernel/io.h"
#include "../kernel/fat32.h"
#include "../kernel/vfs.h"
#include "../games/games.h"

static int strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return *a - *b;
}

static int strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

void execute_command(char *cmd, int design) {
    if (cmd[0] == '\0') {
        return;
    }
    
    if (strncmp(cmd, "echo ", 5) == 0) {
        print_msg(cmd + 5, design);
        print_msg("\n", design);
    } else if (strcmp(cmd, "clear") == 0) {
        int color = vga_color(VGA_WHITE, VGA_BLUE);
        clear_screen(color);
    } else if (strcmp(cmd, "pwd") == 0) {
        int color = vga_color(VGA_LIGHT_CYAN, VGA_BLUE);
        print_msg("Current directory: ", color);
        print_msg(vfs_get_cwd()->path, color);
        print_msg("\n", color);
    } else if (strcmp(cmd, "ls") == 0) {
        vfs_ls(".");
    } else if (strcmp(cmd, "dir") == 0) {
        vfs_ls("/");
    } else if (strncmp(cmd, "cd ", 3) == 0) {
        const char *path = cmd + 3;
        vfs_cd(path);
    } else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char *dirname = cmd + 6;
        vfs_mkdir(dirname);
    } else if (strncmp(cmd, "touch ", 6) == 0) {
        const char *filename = cmd + 6;
        vfs_create_file(filename);
    } else if (strncmp(cmd, "rm ", 3) == 0) {
        const char *path = cmd + 3;
        vfs_delete(path);
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        const char *filename = cmd + 4;
        fat32_file_t file;
        uint8_t buffer[512];
        
        if (fat32_open(filename, &file)) {
            int total_read = 0;
            int bytes_read;
            
            while ((bytes_read = fat32_read(&file, buffer, 512)) > 0) {
                for (int i = 0; i < bytes_read; i++) {
                    if (buffer[i] >= 32 && buffer[i] < 127) {
                        print_msg_char(buffer[i], design);
                    } else if (buffer[i] == '\n') {
                        print_msg("\n", design);
                    } else if (buffer[i] == '\t') {
                        print_msg("  ", design);
                    }
                }
                total_read += bytes_read;
            }
            
            fat32_close(&file);
            
            if (total_read == 0) {
                int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
                print_msg("Error: Could not read file\n", color);
            }
        } else {
            int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
            print_msg("Error: File not found: ", color);
            print_msg((char *)filename, color);
            print_msg("\n", color);
        }
    } else if (strcmp(cmd, "help") == 0) {
        print_msg("Commands:\n", design);
        print_msg("  pwd              - Print working directory\n", design);
        print_msg("  cd <path>        - Change directory (supports cd .., cd /abs/path)\n", design);
        print_msg("  ls               - List files in current directory\n", design);
        print_msg("  dir              - List files in root directory\n", design);
        print_msg("  mkdir <dir>      - Create directory\n", design);
        print_msg("  touch <file>     - Create empty file\n", design);
        print_msg("  rm <path>        - Remove file or directory\n", design);
        print_msg("  cat <file>       - Display file contents\n", design);
        print_msg("  echo <text>      - Print text\n", design);
        print_msg("  clear            - Clear screen\n", design);
        print_msg("  game list        - List available games\n", design);
        print_msg("  game play <game> - Play a game\n", design);
        print_msg("  help             - Show this help\n", design);
        print_msg("\nDefault directories:\n", design);
        print_msg("  /system  - System files (default location)\n", design);
        print_msg("  /personal - User files\n", design);
        print_msg("  /tmp     - Temporary files\n", design);
    } else if (strncmp(cmd, "game ", 5) == 0) {
        char *subcmd = cmd + 5;
        
        if (strcmp(subcmd, "list") == 0) {
            game_list(design);
        } else if (strncmp(subcmd, "play ", 5) == 0) {
            char *game_name = subcmd + 5;
            
            if (strcmp(game_name, "dice") == 0) {
                game_dice(design);
            } else if (strcmp(game_name, "guess") == 0) {
                game_number_guess(design);
            } else {
                int error_color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
                print_msg("Unknown game: ", error_color);
                print_msg(game_name, error_color);
                print_msg("\nUse 'game list' to see available games\n", error_color);
            }
        } else {
            int error_color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
            print_msg("Usage: game list | game play <game_name>\n", error_color);
        }
    } else {
        int error_color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Unknown command: ", error_color);
        print_msg(cmd, error_color);
        print_msg("\n", error_color);
    }
}
