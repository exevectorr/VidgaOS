#define VGA_HEIGHT 25
#define VGA_WIDTH 80

typedef enum VGA_COLOR {
    VGA_BLACK         = 0x0,
    VGA_BLUE          = 0x1,
    VGA_GREEN         = 0x2,
    VGA_CYAN          = 0x3,
    VGA_RED           = 0x4,
    VGA_MAGENTA       = 0x5,
    VGA_BROWN         = 0x6,
    VGA_LIGHT_GRAY    = 0x7,
    VGA_DARK_GRAY     = 0x8,
    VGA_LIGHT_BLUE    = 0x9,
    VGA_LIGHT_GREEN   = 0xA,
    VGA_LIGHT_CYAN    = 0xB,
    VGA_LIGHT_RED     = 0xC,
    VGA_LIGHT_MAGENTA = 0xD,
    VGA_YELLOW        = 0xE,
    VGA_WHITE         = 0xF
} VGA_COLOR;

void clear_screen(VGA_COLOR color);
int vga_color(VGA_COLOR fg, VGA_COLOR bg);
void print_msg(char* msg, VGA_COLOR color);
void print_msg_char(char c, int color);
void print_msg_num(int num, int color);
void handle_next_line(void);
void vga_init(void);