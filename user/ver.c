// AIkOS user program: ver — prints the OS version via int 0x80 write syscall.
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

void _start(void)
{
    sys_write("AIkOS v0.5.0\r\n", 14);
    sys_exit();
}