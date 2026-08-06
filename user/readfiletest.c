// AIkOS user program: readfiletest — tests read_file(fd, buf, len) syscall.
// Component: user_readfiletest
// Provides: _start
// Depends on: syscall (int 0x80)
// Owns: readfiletest application logic
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

static uint64_t sys_read_file(uint64_t fd, void *buf, uint64_t len)
{
    uint64_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(6), "D"(fd), "S"(buf), "d"(len) : "memory");
    return ret;
}

void _start(void)
{
    uint64_t fd = sys_open("/bin/hello.elf");
    uint8_t buf[4];
    sys_read_file(fd, buf, 4);
    // Write "magic:ELF" in one syscall to avoid kernel adding CRLF between them
    char out[9];
    out[0] = 'm';
    out[1] = 'a';
    out[2] = 'g';
    out[3] = 'i';
    out[4] = 'c';
    out[5] = ':';
    out[6] = (char)buf[1];  // 'E'
    out[7] = (char)buf[2];  // 'L'
    out[8] = (char)buf[3];  // 'F'
    sys_write(out, 9);
    sys_write("\r\n", 2);
    sys_close(fd);
    sys_exit();
}