// Physical memory map command (ADR-014).
// Component: memmap
// Provides: memmap_cmd
// Depends on: kernel.h
// Owns: nothing persistent

#include "kernel.h"

struct e820_entry {
    uint64_t base, len;
    uint32_t type, attrs;
};

#define E820_COUNT (*(volatile uint16_t *)0x4FFC)
#define E820_TABLE ((struct e820_entry *)0x5000)
#define E820_TYPE_USABLE 1

void memmap_cmd(const char *args)
{
    (void)args;
    uint32_t count = E820_COUNT;
    if (count > 128)
        count = 128;

    kprintf("E820 memory map:\r\n");
    uint64_t total_usable = 0;
    for (uint32_t i = 0; i < count; i++) {
        struct e820_entry *e = &E820_TABLE[i];
        kprintf("base=0x%lx len=0x%lx type=%u\r\n", e->base, e->len, e->type);
        if (e->type == E820_TYPE_USABLE) {
            total_usable += e->len;
        }
    }
    kprintf("usable: 0x%lx bytes\r\n", total_usable);
}
