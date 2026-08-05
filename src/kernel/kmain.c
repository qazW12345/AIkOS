// AIkOS kernel — C entry point (Phase 0: Proof of Life).
// Freestanding, no libc. Compiled with clang --target=x86_64-elf.

void serial_init(void);
void serial_write_string(const char *str);
void vga_clear(void);
void vga_write_string(const char *str);

void kmain(void)
{
    serial_init();
    vga_clear();

    vga_write_string("AIkOS v0.1.0 - Proof of Life");
    serial_write_string("AIkOS v0.1.0\r\n");
    serial_write_string("Phase 0: long mode reached, halting.\r\n");

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}
