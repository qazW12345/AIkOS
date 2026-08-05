// PIT 8254 driver (ADR-007): 100 Hz square wave on IRQ0.
// Component: pit (8254 timer)
// Provides: pit_init, pit_tick (IRQ0 hook called by idt), pit_get_ticks
// Depends on: kernel.h (port I/O); nothing else
// Owns: PIT channel-0 registers 0x40-0x43; IRQ0; the 100 Hz tick counter

#include "kernel.h"

#define PIT_CMD 0x43
#define PIT_CH0 0x40

static volatile uint64_t ticks;

void pit_init(void)
{
    uint16_t divisor = 11931;    // 1193182 / 100 = 100 Hz
    outb(PIT_CMD, 0x36);         // channel 0, lobyte/hibyte, mode 3, binary
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, divisor >> 8);
}

void pit_tick(void)
{
    ticks++;
}

uint64_t pit_get_ticks(void)
{
    return ticks;
}
