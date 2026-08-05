// AIkOS user program #1 — the syscall round-trip (ADR-013).
// Freestanding, linked at 0x200000, loaded by the boot sector.
// Runs in ring 3: reaches the kernel ONLY through int 0x80.

#include <stdint.h>

static uint64_t sys_write(const char *s, uint64_t len)
{
    uint64_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(1), "D"(s), "S"(len) : "memory");
    return ret;
}

static void sys_exit(void)
{
    __asm__ volatile("int $0x80" : : "a"(2) : "memory");
    for (;;)                            /* never reached */
        __asm__ volatile("hlt");
}

void _start(void)
{
    sys_write("hello from ring 3", 18);
    sys_exit();
}
