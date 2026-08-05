// CMOS RTC driver (ADR-010): wall clock for the REPL.
// Registers (BCD): 0=sec, 2=min, 4=hour, 7=day, 8=month, 9=year, 0x32=century.
// Update-in-progress guard: wait UIP clear, read, re-read seconds, retry.

#include "kernel.h"

#define CMOS_CMD 0x70
#define CMOS_DATA 0x71

static unsigned char cmos_read(unsigned char reg)
{
    outb(CMOS_CMD, reg);
    return inb(CMOS_DATA);
}

static int bcd2bin(unsigned char v)
{
    return (int)((v & 0x0F) + (v >> 4) * 10);
}

void rtc_read(int *sec, int *min, int *hour, int *day, int *month, int *year)
{
    unsigned char s1, s2;

    for (;;) {
        while (cmos_read(0x0A) & 0x80)   // UIP: update in progress
            ;
        s1 = cmos_read(0);
        *min = cmos_read(2);
        *hour = cmos_read(4);
        *day = cmos_read(7);
        *month = cmos_read(8);
        *year = cmos_read(9);
        s2 = cmos_read(0);
        if (s1 == s2)                    // stable read
            break;
    }

    *sec = bcd2bin(s1);
    *min = bcd2bin((unsigned char)*min);
    *hour = bcd2bin((unsigned char)*hour);
    *day = bcd2bin((unsigned char)*day);
    *month = bcd2bin((unsigned char)*month);
    *year = bcd2bin((unsigned char)*year);

    int century = bcd2bin(cmos_read(0x32));
    if (century < 19 || century > 25)    // absent/garbage century -> 20xx
        century = 20;
    *year += century * 100;
}
