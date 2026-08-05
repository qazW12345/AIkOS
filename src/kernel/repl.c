// Kernel-mode REPL over serial (ADR-008): polled RX, line editor, commands.

#include "kernel.h"

#define LINE_MAX 128

static char line[LINE_MAX];
static int line_len;

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static void repl_exec(char *cmd)
{
    if (str_eq(cmd, "help")) {
        serial_write_string("commands: help, echo <text>, ticks, version, panic\r\n");
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' &&
               cmd[4] == ' ' && cmd[5] != '\0') {
        serial_write_string(cmd + 5);
        serial_write_string("\r\n");
    } else if (str_eq(cmd, "ticks")) {
        serial_write_string("ticks: ");
        serial_write_dec(pit_get_ticks());
        serial_write_string("\r\n");
    } else if (str_eq(cmd, "version")) {
        serial_write_string("AIkOS v0.2.0 - The Machine Wakes\r\n");
    } else if (str_eq(cmd, "panic")) {
        serial_write_string("executing ud2\r\n");
        __asm__ volatile("ud2");
    } else {
        serial_write_string("unknown command (try help)\r\n");
    }
}

void repl_run(void)
{
    serial_write_string("AIkOS> ");
    for (;;) {
        if (serial_rx_ready()) {
            char c = serial_read_char();
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
    }
}
