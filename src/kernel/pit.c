// PIT 8254 driver (ADR-007): 100 Hz square wave on IRQ0.

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
