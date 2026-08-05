// AIkOS user program #2 — deliberately faults (ADR-013 test vehicle).
// Executing a privileged instruction from ring 3 raises #GP(0); the kernel
// must kill this task and survive (USER FAULT path), never panic.

#include <stdint.h>

void _start(void)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(0x1000ull));  /* #GP at ring 3 */
    for (;;)                            /* never reached */
        __asm__ volatile("hlt");
}
