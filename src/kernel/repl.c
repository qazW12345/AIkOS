// Kernel-mode REPL (ADR-008/010): shared SPSC input queue fed by polled
// Component: repl (kernel command line)
// Provides: repl_run (never returns), repl_input_putc (queue producer for
//           keyboard IRQ / serial IRQ when it exists)
// Depends on: serial (polled RX), printf (kprintf), vga, rtc, cpuid,
//             proc (run/runfault), hexdump
// Owns: the SPSC input queue; the line editor; command dispatch
// serial AND the keyboard IRQ; line editor + commands.

#include "kernel.h"

#define LINE_MAX 128
#define INBUF 128

static char line[LINE_MAX];
static int line_len;

// SPSC ring: producers = serial polling loop + keyboard IRQ; consumer =
// the REPL loop. head/tail volatile; no locks needed (single producer per
// direction, Phase 1.5 scope — a scheduler will need real locks, Phase 2).
static char inbuf[INBUF];
static volatile int in_head;
static volatile int in_tail;

void repl_input_putc(char c)
{
    int next = (in_head + 1) % INBUF;
    if (next != in_tail) {
        inbuf[in_head] = c;
        in_head = next;
    }
}

static int repl_input_getc(char *c)
{
    if (in_head == in_tail)
        return 0;
    *c = inbuf[in_tail];
    in_tail = (in_tail + 1) % INBUF;
    return 1;
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int hex_val(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return c - 'A' + 10;
}

static int parse_hex(const char *s, uint64_t *out)
{
    uint64_t val = 0;
    int any = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;

    if (!is_hex_digit(*s))
        return 0;

    while (is_hex_digit(*s)) {
        val = (val << 4) | (uint64_t)hex_val(*s);
        s++;
        any = 1;
    }

    if (*s != '\0')
        return 0;

    *out = val;
    return any;
}

static void repl_exec(char *cmd)
{
    if (str_eq(cmd, "help")) {
        kprintf("commands: help, echo <text>, ticks, version, panic, time, cpuid, vga, run, runfault, hexdump <addr> <len>\r\n");
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' &&
               cmd[4] == ' ' && cmd[5] != '\0') {
        kprintf("%s\r\n", cmd + 5);
    } else if (str_eq(cmd, "ticks")) {
        kprintf("ticks: %lu\r\n", pit_get_ticks());
    } else if (str_eq(cmd, "version")) {
        kprintf("AIkOS v0.4.0 - Two Worlds\r\n");
    } else if (str_eq(cmd, "panic")) {
        kprintf("executing ud2\r\n");
        __asm__ volatile("ud2");
    } else if (str_eq(cmd, "time")) {
        int s, mi, h, d, mo, y;
        rtc_read(&s, &mi, &h, &d, &mo, &y);
        kprintf("%04d-%02d-%02d %02d:%02d:%02d\r\n", y, mo, d, h, mi, s);
    } else if (str_eq(cmd, "cpuid")) {
        cpuid_dump();
    } else if (str_eq(cmd, "vga")) {
        char buf[12];
        int i;
        for (i = 0; i < 30; i++) {
            char tmp[4];
            int n = i, p = 0, t = 0;
            buf[p++] = 'L'; buf[p++] = 'i'; buf[p++] = 'n'; buf[p++] = 'e';
            buf[p++] = ' ';
            if (n == 0)
                tmp[t++] = '0';
            while (n) {
                tmp[t++] = (char)('0' + n % 10);
                n /= 10;
            }
            while (t)
                buf[p++] = tmp[--t];
            buf[p++] = '\n';
            buf[p] = '\0';
            vga_write_string(buf);
        }
        kprintf("vga: 30 lines written\r\n");
    } else if (str_eq(cmd, "run")) {
        kprintf("entering ring 3...\r\n");
        proc_run();                     /* returns after sys_exit / fault */
        kprintf("back in kernel\r\n");
    } else if (str_eq(cmd, "runfault")) {
        kprintf("entering ring 3 (faulting program)...\r\n");
        proc_run_fault();
        kprintf("back in kernel\r\n");
    } else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'x' && cmd[3] == 'd' &&
               cmd[4] == 'u' && cmd[5] == 'm' && cmd[6] == 'p' && cmd[7] == ' ') {
        const char *args = cmd + 8;
        uint64_t addr, len;
        char *arg2;

        // skip leading spaces
        while (*args == ' ')
            args++;

        if (*args == '\0') {
            kprintf("hexdump: usage: hexdump <addr> <len>\r\n");
        } else {
            // find second argument
            arg2 = (char *)args;
            while (*arg2 && *arg2 != ' ')
                arg2++;
            if (*arg2 == '\0') {
                kprintf("hexdump: usage: hexdump <addr> <len>\r\n");
            } else {
                *arg2 = '\0';
                arg2++;
                while (*arg2 == ' ')
                    arg2++;

                if (!parse_hex(args, &addr)) {
                    kprintf("hexdump: bad address\r\n");
                } else if (!parse_hex(arg2, &len)) {
                    kprintf("hexdump: bad length\r\n");
                } else {
                    hexdump(addr, len);
                }
            }
        }
    } else {
        kprintf("unknown command (try help)\r\n");
    }
}

static void repl_handle(char c)
{
    if (c == '\r' || c == '\n') {
        serial_write_string("\r\n");
        line[line_len] = '\0';
        repl_exec(line);
        line_len = 0;
        serial_write_string("AIkOS> ");
    } else if (c == '\b' || c == 0x7F) {
        if (line_len > 0) {
            line_len--;
            serial_write_string("\b \b");
        }
    } else if (line_len < LINE_MAX - 1) {
        line[line_len++] = c;
        serial_putc(c);
    }
}

void repl_run(void)
{
    serial_write_string("AIkOS> ");
    for (;;) {
        char c;
        if (serial_rx_ready())
            repl_input_putc(serial_read_char());
        if (repl_input_getc(&c))
            repl_handle(c);
    }
}
