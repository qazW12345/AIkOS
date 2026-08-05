// AIkOS kernel — C entry point.
// Phase 0: banner. Phase 1: IDT, PIC, PIT. Phase 1.5: REPL never returns.
// Phase 2 (ADR-012/013): physical memory + TSS for the ring-3 world.

#include "kernel.h"

void kmain(void)
{
    serial_init();
    vga_clear();

    vga_write_string("AIkOS v0.4.0 - Two Worlds");
    serial_write_string("AIkOS v0.4.0\r\n");
    serial_write_string("Two Worlds\r\n");

    idt_init();
    pic_init();
    pit_init();
    pmm_init();
    tss_init();
    __asm__ volatile("sti");

    repl_run();   /* never returns */
}
