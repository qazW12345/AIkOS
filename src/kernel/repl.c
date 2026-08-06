// Kernel-mode REPL (ADR-008/010): shared SPSC input queue fed by polled
// Component: repl (kernel command line)
// Provides: repl_run (never returns), repl_input_putc (queue producer for
//           keyboard IRQ / serial IRQ when it exists)
// Depends on: serial (polled RX), printf (kprintf), vga, rtc, cpuid,
//             proc (run/runfault), hexdump
// Owns: the SPSC input queue; the line editor; command dispatch table

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

// Forward declarations for command handlers
static void cmd_help(const char *args);
static void cmd_echo(const char *args);
static void cmd_ticks(const char *args);
static void cmd_version(const char *args);
static void cmd_panic(const char *args);
static void cmd_time(const char *args);
static void cmd_cpuid(const char *args);
static void cmd_vga(const char *args);
static void cmd_run(const char *args);
static void cmd_runfault(const char *args);
static void cmd_hexdump(const char *args);
static void cmd_heap(const char *args);
static void cmd_heaptest(const char *args);

struct repl_cmd {
    const char *name;                  /* command word, e.g. "echo" */
    const char *usage;                 /* name + args hint for help, e.g. "echo <text>" */
    void (*handler)(const char *args); /* args = text after the name, leading spaces stripped, "" if none */
};

static const struct repl_cmd cmd_table[] = {
    { "help",        "help",                           cmd_help },
    { "echo",        "echo <text>",                    cmd_echo },
    { "ticks",       "ticks",                          cmd_ticks },
    { "version",     "version",                        cmd_version },
    { "panic",       "panic",                          cmd_panic },
    { "time",        "time",                           cmd_time },
    { "cpuid",       "cpuid",                          cmd_cpuid },
    { "vga",         "vga",                            cmd_vga },
    { "run",         "run",                            cmd_run },
    { "runfault",    "runfault",                       cmd_runfault },
    { "hexdump",     "hexdump <addr> <len>",           cmd_hexdump },
    { "heap",        "heap",                           cmd_heap },
    { "heaptest",    "heaptest",                       cmd_heaptest },
};

static int cmd_table_size(void)
{
    return sizeof(cmd_table) / sizeof(cmd_table[0]);
}

// Match command name at start of string, return pointer to args (after name + space)
// or end-of-string. Returns 0 if no match.
static const char *cmd_match(const char *cmd, const char *name)
{
    while (*name && *cmd && *name == *cmd) {
        name++;
        cmd++;
    }
    if (*name != '\0')
        return 0;
    if (*cmd == '\0')
        return cmd;
    if (*cmd == ' ')
        return cmd + 1;
    return 0;
}

static void cmd_help(const char *args)
{
    (void)args;
    kprintf("commands: ");
    int n = cmd_table_size();
    for (int i = 0; i < n; i++) {
        kprintf("%s", cmd_table[i].usage);
        if (i + 1 < n)
            kprintf(", ");
    }
    kprintf("\r\n");
}

static void cmd_echo(const char *args)
{
    while (*args == ' ')
        args++;
    if (*args != '\0')
        kprintf("%s\r\n", args);
}

static void cmd_ticks(const char *args)
{
    (void)args;
    kprintf("ticks: %lu\r\n", pit_get_ticks());
}

static void cmd_version(const char *args)
{
    (void)args;
    kprintf("AIkOS v0.4.0 - Two Worlds\r\n");
}

static void cmd_panic(const char *args)
{
    (void)args;
    kprintf("executing ud2\r\n");
    __asm__ volatile("ud2");
}

static void cmd_time(const char *args)
{
    (void)args;
    int s, mi, h, d, mo, y;
    rtc_read(&s, &mi, &h, &d, &mo, &y);
    kprintf("%04d-%02d-%02d %02d:%02d:%02d\r\n", y, mo, d, h, mi, s);
}

static void cmd_cpuid(const char *args)
{
    (void)args;
    cpuid_dump();
}

static void cmd_vga(const char *args)
{
    (void)args;
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
}

static void cmd_run(const char *args)
{
    (void)args;
    kprintf("entering ring 3...\r\n");
    proc_run();                     /* returns after sys_exit / fault */
    kprintf("back in kernel\r\n");
}

static void cmd_runfault(const char *args)
{
    (void)args;
    kprintf("entering ring 3 (faulting program)...\r\n");
    proc_run_fault();
    kprintf("back in kernel\r\n");
}

static void cmd_hexdump(const char *args)
{
    while (*args == ' ')
        args++;

    if (*args == '\0') {
        kprintf("hexdump: usage: hexdump <addr> <len>\r\n");
        return;
    }

    // find second argument
    const char *arg2 = args;
    while (*arg2 && *arg2 != ' ')
        arg2++;
    if (*arg2 == '\0') {
        kprintf("hexdump: usage: hexdump <addr> <len>\r\n");
        return;
    }

    // make a modifiable copy for parsing
    char buf[128];
    int i = 0;
    while (args + i < arg2 && i < 127) {
        buf[i] = args[i];
        i++;
    }
    buf[i] = '\0';

    const char *len_str = arg2 + 1;
    while (*len_str == ' ')
        len_str++;

    uint64_t addr, len;
    if (!parse_hex(buf, &addr)) {
        kprintf("hexdump: bad address\r\n");
    } else if (!parse_hex(len_str, &len)) {
        kprintf("hexdump: bad length\r\n");
    } else {
        hexdump(addr, len);
    }
}

static void cmd_heap(const char *args)
{
    (void)args;
    uint64_t free_pages;
    int largest_order;
    uint64_t alloc_blocks;
    heap_stats(&free_pages, &largest_order, &alloc_blocks);
    kprintf("heap: free %lu pages, largest order %d, %lu blocks\r\n",
            free_pages, largest_order, alloc_blocks);
}

static void cmd_heaptest(const char *args)
{
    (void)args;
    int ok = 1;

    // (a) kmalloc sizes {16, 100, 4096, 5000, 200000}
    size_t sizes_a[] = {16, 100, 4096, 5000, 200000};
    void *bufs_a[5];
    for (int i = 0; i < 5; i++) {
        bufs_a[i] = kmalloc(sizes_a[i]);
        if (!bufs_a[i]) {
            kprintf("heaptest: kmalloc failed at step a[%d]\r\n", i);
            ok = 0;
            break;
        }
        // fill with (i + j) & 0xFF
        unsigned char *p = (unsigned char *)bufs_a[i];
        for (size_t j = 0; j < sizes_a[i]; j++)
            p[j] = (unsigned char)((i + j) & 0xFF);
        // verify
        for (size_t j = 0; j < sizes_a[i]; j++) {
            if (p[j] != (unsigned char)((i + j) & 0xFF)) {
                kprintf("heaptest: verify failed at step a[%d], j=%zu\r\n", i, j);
                ok = 0;
                break;
            }
        }
        if (!ok) break;
    }

    // (b) kfree buffers 0, 2, 4 (leaving gaps)
    if (ok) {
        kfree(bufs_a[0]);
        kfree(bufs_a[2]);
        kfree(bufs_a[4]);
    }

    // (c) kmalloc sizes {64, 8192, 65536}, fill + verify with fresh patterns
    size_t sizes_c[] = {64, 8192, 65536};
    void *bufs_c[3];
    if (ok) {
        for (int i = 0; i < 3; i++) {
            bufs_c[i] = kmalloc(sizes_c[i]);
            if (!bufs_c[i]) {
                kprintf("heaptest: kmalloc failed at step c[%d]\r\n", i);
                ok = 0;
                break;
            }
            unsigned char *p = (unsigned char *)bufs_c[i];
            for (size_t j = 0; j < sizes_c[i]; j++)
                p[j] = (unsigned char)((i * 17 + j * 3) & 0xFF);
            for (size_t j = 0; j < sizes_c[i]; j++) {
                if (p[j] != (unsigned char)((i * 17 + j * 3) & 0xFF)) {
                    kprintf("heaptest: verify failed at step c[%d], j=%zu\r\n", i, j);
                    ok = 0;
                    break;
                }
            }
            if (!ok) break;
        }
    }

    // (d) kfree everything
    if (ok) {
        kfree(bufs_a[1]);
        kfree(bufs_a[3]);
        kfree(bufs_c[0]);
        kfree(bufs_c[1]);
        kfree(bufs_c[2]);
    }

    // (d2) accounting: every page must be back in the free lists
    // (catches coalescing leaks AND alignment-fragmentation — a merged block
    //  that is never re-pushed disappears from the heap, and unaligned pulls
    //  that never re-merge undercount free pages; buffer-content checks see
    //  neither). With aligned pulls heaptest holds ~135 pages before step (e);
    //  require >= 100 free — the unaligned variant yields ~70 and must FAIL.
    if (ok) {
        uint64_t free_pages;
        int largest_order;
        uint64_t alloc_blocks;
        heap_stats(&free_pages, &largest_order, &alloc_blocks);
        if (alloc_blocks != 0 || free_pages < 100) {
            kprintf("heaptest: accounting FAIL (free %lu pages, %lu blocks)\r\n",
                    free_pages, alloc_blocks);
            ok = 0;
        }
    }

    // (e) kmalloc 1000000, fill + verify, kfree
    if (ok) {
        void *big = kmalloc(1000000);
        if (!big) {
            kprintf("heaptest: kmalloc failed at step e\r\n");
            ok = 0;
        } else {
            unsigned char *p = (unsigned char *)big;
            for (size_t j = 0; j < 1000000; j++)
                p[j] = (unsigned char)(j & 0xFF);
            for (size_t j = 0; j < 1000000; j++) {
                if (p[j] != (unsigned char)(j & 0xFF)) {
                    kprintf("heaptest: verify failed at step e, j=%zu\r\n", j);
                    ok = 0;
                    break;
                }
            }
            kfree(big);
        }
    }

    if (ok)
        kprintf("heaptest OK\r\n");
    else
        kprintf("heaptest FAIL\r\n");
}

static void repl_exec(char *cmd)
{
    int n = cmd_table_size();
    for (int i = 0; i < n; i++) {
        const char *args = cmd_match(cmd, cmd_table[i].name);
        if (args) {
            cmd_table[i].handler(args);
            return;
        }
    }
    kprintf("unknown command (try help)\r\n");
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