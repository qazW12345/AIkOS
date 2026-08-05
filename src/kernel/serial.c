// Minimal 16550 UART driver for COM1 (0x3F8) — the kernel's debug lifeline.
// Phase 1: polled RX (REPL input) + hex/dec printers added.
//
// NOTE: the 16550 is PORT-mapped I/O — it requires the `out`/`in`
// instructions. A plain memory store to 0x3F8 (*(volatile ...) = c) silently
// writes to RAM and produces nothing on the wire. War story #4,
// see Guides/How-to-debug.md.

#include "kernel.h"

#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00);        // disable interrupts
    outb(COM1 + 3, 0x80);        // DLAB on
    outb(COM1 + 0, 0x01);        // divisor low  (115200 baud)
    outb(COM1 + 1, 0x00);        // divisor high
    outb(COM1 + 3, 0x03);        // 8N1, DLAB off
    outb(COM1 + 2, 0xC7);        // FIFO enable, clear, 14-byte threshold
    outb(COM1 + 4, 0x0B);        // RTS/DSR set
}

void serial_putc(char c)
{
    while ((inb(COM1 + 5) & 0x20) == 0)  // wait for THR empty
        ;
    outb(COM1, (unsigned char)c);
}

void serial_write_string(const char *str)
{
    while (*str)
        serial_putc(*str++);
}

int serial_rx_ready(void)
{
    return (inb(COM1 + 5) & 1) != 0;
}

char serial_read_char(void)
{
    return (char)inb(COM1);
}

void serial_write_hex(uint64_t v, int digits)
{
    int i;
    for (i = digits - 1; i >= 0; i--) {
        int n = (int)((v >> (4 * i)) & 0xF);
        serial_putc((char)(n < 10 ? '0' + n : 'a' + n - 10));
    }
}

void serial_write_dec(uint64_t v)
{
    char buf[21];
    int i = 0;
    if (v == 0) {
        serial_putc('0');
        return;
    }
    while (v > 0) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i > 0)
        serial_putc(buf[--i]);
}
