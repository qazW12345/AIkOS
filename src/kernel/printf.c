// kprintf — kernel print API (ADR-010). Streams to serial, no buffer.
// Component: printf (kprintf engine)
// Provides: kprintf (varargs)
// Depends on: serial (serial_putc) — serial-only output
// Owns: the format grammar (%c %s %d %u %x %ld %lu %lx %p %%, 0+width pad)
// Format: %c %s %d %u %x (32-bit) | %ld %lu %lx (64-bit) | %p (16 hex) | %%
// Optional '0' + width zero-pads %d/%u/%x (e.g. %02x, %02d).
// NOTE: 32-bit and 64-bit args MUST be read with the right va_arg type —
// reading an int arg as uint64_t is garbage (war story: %d vs %ld).

#include "kernel.h"
#include <stdarg.h>

static int digit_count(uint64_t v)
{
    int n = 1;
    while (v >= 10) {
        v /= 10;
        n++;
    }
    return n;
}

void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            serial_putc(*fmt);
            continue;
        }
        fmt++;
        int width = 0;
        if (*fmt == '0')
            fmt++;
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');
        int lng = 0;
        if (*fmt == 'l') {
            lng = 1;
            fmt++;
        }
        switch (*fmt) {
        case 'c':
            serial_putc((char)va_arg(ap, int));
            break;
        case 's':
            serial_write_string(va_arg(ap, const char *));
            break;
        case 'd': {
            int64_t sv = lng ? va_arg(ap, int64_t) : va_arg(ap, int);
            uint64_t uv;
            if (sv < 0) {
                serial_putc('-');
                uv = (uint64_t)(-sv);
            } else {
                uv = (uint64_t)sv;
            }
            while (digit_count(uv) < width)
                serial_putc('0'), width--;
            serial_write_dec(uv);
            break;
        }
        case 'u': {
            uint64_t uv = lng ? va_arg(ap, uint64_t) : va_arg(ap, unsigned int);
            while (digit_count(uv) < width)
                serial_putc('0'), width--;
            serial_write_dec(uv);
            break;
        }
        case 'x': {
            uint64_t uv = lng ? va_arg(ap, uint64_t) : va_arg(ap, unsigned int);
            serial_write_hex(uv, width ? width : (lng ? 16 : 8));
            break;
        }
        case 'p':
            serial_write_hex((uint64_t)va_arg(ap, void *), 16);
            break;
        case '%':
            serial_putc('%');
            break;
        default:
            serial_putc('%');
            serial_putc(*fmt);
            break;
        }
    }
    va_end(ap);
}
