// AIkOS kernel — shared declarations (freestanding, no libc).

#ifndef AIKOS_KERNEL_H
#define AIKOS_KERNEL_H

#include <stdint.h>

/* port I/O — inline here: several drivers need it (serial.c, pic.c, ...) */
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

/* serial.c */
void serial_init(void);
void serial_putc(char c);
void serial_write_string(const char *str);
void serial_write_hex(uint64_t v, int digits);
void serial_write_dec(uint64_t v);
int serial_rx_ready(void);
char serial_read_char(void);

/* vga.c */
void vga_clear(void);
void vga_write_string(const char *str);

/* idt.c */
void idt_init(void);

/* pic.c */
void pic_init(void);
void pic_eoi(int irq);
int pic_is_spurious(int irq);

/* pit.c */
void pit_init(void);
void pit_tick(void);
uint64_t pit_get_ticks(void);

/* keyboard.c */
void keyboard_irq(void);

/* repl.c */
void repl_run(void);

#endif
