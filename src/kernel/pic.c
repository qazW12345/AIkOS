// 8259A PIC driver (ADR-007): remap IRQs to 0x20-0x2F, mask, EOI.
// IRQ0 = PIT timer, IRQ1 = PS/2 keyboard (the only two unmasked in Phase 1).

#include "kernel.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

void pic_init(void)
{
    outb(PIC1_CMD, 0x11);        // ICW1: cascade, ICW4 expected
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, 0x20);       // ICW2: master vector offset
    outb(PIC2_DATA, 0x28);       // ICW2: slave vector offset
    outb(PIC1_DATA, 0x04);       // ICW3: slave on master IRQ2
    outb(PIC2_DATA, 0x02);       // ICW3: slave cascade id 2
    outb(PIC1_DATA, 0x01);       // ICW4: 8086 mode
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0xFC);       // OCW1: unmask IRQ0 + IRQ1 only
    outb(PIC2_DATA, 0xFF);       // OCW1: mask all slave IRQs
}

// Spurious IRQ7/15: the IRQ fired but no device asserted it (ISR bit clear).
// Return 1 when spurious — caller must NOT send EOI.
int pic_is_spurious(int irq)
{
    if (irq == 7) {
        outb(PIC1_CMD, 0x0B);    // OCW3: read ISR
        return !(inb(PIC1_CMD) & 0x80);
    }
    if (irq == 15) {
        outb(PIC2_CMD, 0x0B);
        return !(inb(PIC2_CMD) & 0x80);
    }
    return 0;
}

void pic_eoi(int irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, 0x20);    // OCW2: EOI, slave
    outb(PIC1_CMD, 0x20);        // OCW2: EOI, master
}
