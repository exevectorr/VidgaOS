#include "vga.h"
#include "../kernel/io.h"

char *VGA = (char *)0xb8000;
int vgaBuffPos = 0;

static void update_cursor(void) {
    int pos = vgaBuffPos / 2;  // Convert byte offset to character position
    outb(0x3D4, 0x0F);  // Low byte of cursor position
    outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E);  // High byte of cursor position
    outb(0x3D5, (pos >> 8) & 0xFF);
}

static void scroll_screen(int color) {
    // Shift all content up by one line (160 bytes = 80 chars * 2)
    for (int i = 0; i < (VGA_WIDTH * VGA_HEIGHT * 2) - 160; i++) {
        VGA[i] = VGA[i + 160];
    }
    
    // Clear last line with default color
    for (int i = (VGA_WIDTH * VGA_HEIGHT * 2) - 160; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        VGA[i] = 0;
        VGA[i + 1] = color;
    }
}

void vga_init(void) {
    // Enable small text cursor (just 1-2 pixels at bottom)
    outb(0x3D4, 0x0A);  // Cursor start register
    outb(0x3D5, 0x0D);  // Start at line 13
    outb(0x3D4, 0x0B);  // Cursor end register
    outb(0x3D5, 0x0E);  // End at line 14 (only 2 pixels tall)
}

void handle_next_line(void) {
    // Move to start of next line
    int current_line = vgaBuffPos / 160;
    vgaBuffPos = (current_line + 1) * 160;
    update_cursor();
}

void print_msg(char* msg, VGA_COLOR color)
{
    int i = 0;

    while (msg[i] != '\0')
    {
        if (msg[i] == '\n')
        {
            handle_next_line();
            i++;
            continue;
        }
        if (msg[i] == '\b')
        {
            VGA[vgaBuffPos-2] = ' ';
            VGA[vgaBuffPos-1] = color;
            vgaBuffPos-=2;
            i++;
            update_cursor();
            continue;
        }
        
        // Check if we need to wrap to next line
        if ((vgaBuffPos / 160) >= VGA_HEIGHT || vgaBuffPos >= VGA_WIDTH * VGA_HEIGHT * 2) {
            scroll_screen(color);
            vgaBuffPos -= 160;
        }
        
        VGA[vgaBuffPos] = msg[i];
        VGA[vgaBuffPos + 1] = color;

        ++i;
        vgaBuffPos += 2;
    }
    
    update_cursor();
}

void clear_screen(VGA_COLOR color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        VGA[i] = 0;
        VGA[i + 1] = color;
    }
    vgaBuffPos = 0;
    update_cursor();
}

int vga_color(VGA_COLOR fg, VGA_COLOR bg) {
    return (bg << 4) | (fg & 0x0F);
}

void print_msg_num(int num, int color) {
    char buf[16];
    int idx = 15;
    int is_negative = 0;
    
    buf[15] = '\0';
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    if (num == 0) {
        buf[14] = '0';
        idx = 14;
    } else {
        while (num > 0 && idx > 0) {
            buf[--idx] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    if (is_negative && idx > 0) {
        buf[--idx] = '-';
    }
    
    print_msg(&buf[idx], color);
}

void print_msg_char(char c, int color) {
    char buf[2] = {c, '\0'};
    print_msg(buf, color);
}
