// PS/2 keyboard (ADR-008): IRQ1, scancode set 1 (8042-translated).
// Phase 1 role: scancode viewer — prints each scancode over serial.

#include "kernel.h"

void keyboard_irq(void)
{
    unsigned char scancode = inb(0x60);  // read only on IRQ (else garbage)
    serial_write_string("KB: 0x");
    serial_write_hex(scancode, 2);
    serial_write_string("\r\n");
}
