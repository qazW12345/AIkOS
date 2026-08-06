// hexdump — kernel-internal memory dump tool (debug workhorse for Phase 3).
// Component: hexdump (memory dump tool)
// Provides: hexdump(addr, len)
// Depends on: serial (serial_putc), printf (kprintf)
// Owns: (stateless — reads identity-mapped memory; len capped at 256)
// Output format: one row per 16 bytes, CRLF-terminated
// <8 lowercase hex digits of address>  <16 bytes as %02x single-space separated, with an extra space after the 8th byte>  |<16 ascii chars>|
// ASCII column: chars 0x20..0x7E printable, everything else '.'. Address increments 16 per row.

#include "kernel.h"

static void hex_byte(uint8_t b)
{
    char hi = (b >> 4) & 0xF;
    char lo = b & 0xF;
    serial_putc(hi < 10 ? '0' + hi : 'a' + hi - 10);
    serial_putc(lo < 10 ? '0' + lo : 'a' + lo - 10);
}

static void hex_addr(uint64_t addr)
{
    int i;
    for (i = 7; i >= 0; i--) {
        int n = (int)((addr >> (4 * i)) & 0xF);
        serial_putc(n < 10 ? '0' + n : 'a' + n - 10);
    }
}

void hexdump(uint64_t addr, uint64_t len)
{
    uint64_t row_start;
    uint64_t row_end;
    uint64_t j;

    if (len > 256) {
        kprintf("hexdump: len capped at 256\r\n");
        len = 256;
    }

    for (row_start = addr; row_start < addr + len; row_start += 16) {
        // print address
        hex_addr(row_start);
        kprintf("  ");

        row_end = row_start + 16;
        if (row_end > addr + len)
            row_end = addr + len;

        // print hex bytes
        for (j = row_start; j < row_end; j++) {
            hex_byte(*(volatile uint8_t *)j);
            serial_putc(' ');
            if (j == row_start + 7)
                serial_putc(' ');
        }

        // pad to 16 bytes if short row
        for (j = row_end; j < row_start + 16; j++) {
            kprintf("   ");
            if (j == row_start + 7)
                serial_putc(' ');
        }

        kprintf(" |");

        // print ascii
        for (j = row_start; j < row_end; j++) {
            uint8_t c = *(volatile uint8_t *)j;
            serial_putc((c >= 0x20 && c <= 0x7E) ? c : '.');
        }

        // pad ascii if short row
        for (j = row_end; j < row_start + 16; j++) {
            serial_putc(' ');
        }

        kprintf("|\r\n");
    }
}