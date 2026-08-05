// CPUID (ADR-010): vendor, family/model/stepping, feature flags.
// Component: cpuid (CPU identification)
// Provides: cpuid_dump
// Depends on: printf (kprintf)
// Owns: (stateless — reads the CPU)

#include "kernel.h"

static void cpuid_leaf(uint32_t leaf, uint32_t *a, uint32_t *b,
                       uint32_t *c, uint32_t *d)
{
    __asm__ volatile("cpuid"
                     : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                     : "a"(leaf));
}

void cpuid_dump(void)
{
    uint32_t a, b, c, d;
    char vendor[13];

    cpuid_leaf(0, &a, &b, &c, &d);
    *(uint32_t *)&vendor[0] = b;         // vendor = EBX EDX ECX, in order
    *(uint32_t *)&vendor[4] = d;
    *(uint32_t *)&vendor[8] = c;
    vendor[12] = '\0';
    kprintf("cpuid: vendor '%s', max leaf %d\r\n", vendor, a);

    cpuid_leaf(1, &a, &b, &c, &d);
    kprintf("cpuid: family %d, model %d, stepping %d\r\n",
            ((a >> 8) & 0xF) + ((a >> 20) & 0xFF), (a >> 4) & 0xF, a & 0xF);
    kprintf("cpuid: %s%s%s%s%s%s\r\n",
            d & (1 << 23) ? "mmx " : "",
            d & (1 << 25) ? "sse " : "",
            d & (1 << 26) ? "sse2 " : "",
            d & (1 << 6)  ? "pae " : "",
            c & (1 << 21) ? "x2apic " : "",
            c & (1 << 27) ? "osxsave " : "");

    cpuid_leaf(0x80000001, &a, &b, &c, &d);
    kprintf("cpuid: %s%s\r\n",
            d & (1 << 20) ? "nx " : "",
            d & (1 << 29) ? "lm " : "");
}
