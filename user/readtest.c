// AIkOS user program: readtest — tests read syscall (fd 0 = serial stdin).
// Component: user_readtest
// Provides: _start
// Depends on: syscall (int 0x80)
// Owns: readtest application logic
#include <stdint.h>

static uint64_t sys_read(uint64_t fd, void *buf, uint64_t len)
{
    uint64_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(3), "D"(fd), "S"(buf), "d"(len) : "memory");
    return ret;
}

static uint64_t sys_write(const char *s, uint64_t len)
{
    uint64_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(1), "D"(s), "S"(len) : "memory");
    return ret;
}

static void sys_exit(void)
{
    __asm__ volatile("int $0x80" : : "a"(2) : "memory");
    for (;;)
        __asm__ volatile("hlt");
}

void _start(void)
{
    char c = 0;
    while (sys_read(0, &c, 1) != 1) {
        // busy loop
    }
    char msg[8];
    msg[0] = 'r';
    msg[1] = 'e';
    msg[2] = 'a';
    msg[3] = 'd';
    msg[4] = ':';
    msg[5] = c;
    msg[6] = '\r';
    msg[7] = '\n';
    sys_write(msg, 8);
    sys_exit();
}
