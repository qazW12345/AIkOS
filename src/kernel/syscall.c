// Syscall dispatcher (ADR-013). Gate: int 0x80, DPL 3 (idt.c).
// Component: syscall (int 0x80 dispatcher)
// Provides: syscall_dispatch(isr_frame*)
// Depends on: idt (gate 0x80), entry.asm (user_return), serial (write
//             syscall), kernel.h (stack_top for the frame rewrite)
// Owns: the syscall ABI (eax=number, rdi/rsi/rdx/r10); numbers 1=write,
//       2=exit, 3=read, 4=open
// ABI: number in eax, args in rdi/rsi/rdx/r10 (SysV-style).
// Syscalls: 1 = write(ptr, len), 2 = exit(), 3 = read(fd, ptr, len),
//           4 = open(path).

#include "kernel.h"

void syscall_dispatch(struct isr_frame *f)
{
    switch (f->rax) {
    case 1: {                           /* write */
        const char *s = (const char *)f->rdi;
        uint64_t len = f->rsi, i;
        kprintf("SYSCALL 1 (write) len=%lu\r\n", len);
        for (i = 0; i < len; i++)
            serial_putc(s[i]);
        serial_write_string("\r\n");
        f->rax = len;
        break;
    }
    case 2: {                           /* exit: never return to ring 3 */
        kprintf("SYSCALL 2 (exit)\r\nuser exited\r\n");
        /* Rewrite the whole frame tail for a same-ring return to the
         * kernel: iretq validates SS.RPL == CPL even without a ring
         * change — a leftover user SS (0x23, RPL 3) is #GP(0x20). */
        f->cs = 0x08;
        f->ss = 0x10;
        f->rsp = (uint64_t)stack_top;
        f->rip = (uint64_t)user_return; /* trampoline -> REPL call chain */
        break;
    }
    case 3: {                           /* read */
        uint64_t fd = f->rdi;
        char *buf = (char *)f->rsi;
        uint64_t len = f->rdx;
        if (fd != 0) {
            kprintf("SYSCALL 3 (read) invalid fd=%lu\r\n", fd);
            f->rax = 0xFFFFFFFF;
            break;
        }
        kprintf("SYSCALL 3 (read) len=%lu\r\n", len);
        uint64_t bytes_read = 0;
        while (bytes_read < len && serial_rx_ready()) {
            buf[bytes_read] = serial_read_char();
            bytes_read++;
        }
        f->rax = bytes_read;
        break;
    }
    case 4: {                           /* open */
        const char *path = (const char *)f->rdi;
        kprintf("SYSCALL 4 (open) path=%s\r\n", path ? path : "(null)");
        f->rax = fd_open(path);
        break;
    }
    default:
        kprintf("SYSCALL %lu (unknown)\r\n", f->rax);
        f->rax = (uint64_t)-1;
        break;
    }
}
