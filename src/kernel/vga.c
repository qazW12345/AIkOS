// Minimal VGA text mode driver — 80x25, buffer at 0xB8000.
// Phase 0 scope: clear + write, no scrolling (banner fits on one screen).

#define VGA_MEM ((volatile unsigned short *)0xB8000)
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_ATTR 0x0F            // white on black

static int cursor_x = 0;
static int cursor_y = 0;

void vga_clear(void)
{
    int i;
    for (i = 0; i < VGA_COLS * VGA_ROWS; i++)
        VGA_MEM[i] = (unsigned short)(0x20 | (VGA_ATTR << 8));
    cursor_x = 0;
    cursor_y = 0;
}

static void vga_putc(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        VGA_MEM[cursor_y * VGA_COLS + cursor_x] =
            (unsigned short)((unsigned char)c | (VGA_ATTR << 8));
        cursor_x++;
    }
    if (cursor_x >= VGA_COLS) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_ROWS)
        cursor_y = VGA_ROWS - 1; // no scrolling in Phase 0; stay on last row
}

void vga_write_string(const char *str)
{
    while (*str)
        vga_putc(*str++);
}
