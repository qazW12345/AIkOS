// AIkOS user program: closetest — tests close(fd) syscall.
// Component: user_closetest
// Provides: _start
// Depends on: syscall (int 0x80)
// Owns: closetest application logic
#include <stdint.h>

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

static uint64_t sys_open(const char *path)
{
    uint64_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(4), "D"(path) : "memory");
    return ret;
}

static uint64_t sys_close(uint64_t fd)
{
    uint64_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(5), "D"(fd) : "memory");
    return ret;
}

void _start(void)
{
    uint64_t fd = sys_open("/bin/hello.elf");
    uint64_t rc = sys_close(fd);
    char buf[9];
    buf[0] = 'c';
    buf[1] = 'l';
    buf[2] = 'o';
    buf[3] = 's';
    buf[4] = 'e';
    buf[5] = ':';
    buf[6] = (char)('0' + rc);
    buf[7] = '\r';
    buf[8] = '\n';
    sys_write(buf, 9);
    sys_exit();
}
