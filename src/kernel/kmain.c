// AIkOS kernel — C entry point.
// Component: kmain (boot orchestration)
// Provides: kmain() — called by entry.asm after long-mode setup
// Depends on: serial, vga, idt, pic, pit, mm, tss, buddy (the init chain)
// Owns: boot-time init order; the boot banner
// Phase 0: banner. Phase 1: IDT, PIC, PIT. Phase 1.5: REPL never returns.
// Phase 2 (ADR-012/013): physical memory + TSS for the ring-3 world.
// Phase 3 (ADR-017): buddy allocator + kernel heap.

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
    buddy_init();
    tss_init();
    __asm__ volatile("sti");

    repl_run();   /* never returns */
}
