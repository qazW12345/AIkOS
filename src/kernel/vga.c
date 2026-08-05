// Minimal VGA text mode driver — 80x25, buffer at 0xB8000.
// Phase 1.5 (ADR-010): console scrolling — rows shift up, bottom row blanks.

#include "kernel.h"

#define VGA_MEM ((volatile unsigned short *)0xB8000)
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_ATTR 0x0F            // white on black

static int cursor_x = 0;
static int cursor_y = 0;

static void vga_scroll(void)
{
    int i;
    for (i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++)
        VGA_MEM[i] = VGA_MEM[i + VGA_COLS];
    for (i = (VGA_ROWS - 1) * VGA_COLS; i < VGA_ROWS * VGA_COLS; i++)
        VGA_MEM[i] = (unsigned short)(0x20 | (VGA_ATTR << 8));
}

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
    if (cursor_y >= VGA_ROWS) {
        vga_scroll();
        cursor_y = VGA_ROWS - 1;
    }
}

void vga_write_string(const char *str)
{
    while (*str)
        vga_putc(*str++);
}
