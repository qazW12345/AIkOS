// AIkOS kernel — C entry point.
// Phase 0: banner. Phase 1: IDT, PIC, PIT, then the REPL (never returns).

#include "kernel.h"

void kmain(void)
{
    serial_init();
    vga_clear();

    vga_write_string("AIkOS v0.2.0 - The Machine Wakes");
    serial_write_string("AIkOS v0.2.0\r\n");
    serial_write_string("The Machine Wakes\r\n");

    idt_init();
    pic_init();
    pit_init();
    __asm__ volatile("sti");

    repl_run();   /* never returns */
}
