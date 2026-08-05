// Minimal 16550 UART driver for COM1 (0x3F8) — the kernel's debug lifeline.
// Phase 0 scope: init + output only.
//
// NOTE: the 16550 is PORT-mapped I/O — it requires the `out`/`in`
// instructions. A plain memory store to 0x3F8 (*(volatile ...) = c) silently
// writes to RAM and produces nothing on the wire. War story #4,
// see Guides/How-to-debug.md.

#define COM1 0x3F8

static inline void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %w1" : : "a"(val), "d"(port));
}

static inline unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile("inb %w1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

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
