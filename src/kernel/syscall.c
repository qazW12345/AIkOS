// Syscall dispatcher (ADR-013). Gate: int 0x80, DPL 3 (idt.c).
// ABI: number in eax, args in rdi/rsi/rdx/r10 (SysV-style).
// Phase 2 syscalls: 1 = write(ptr, len), 2 = exit().

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
    default:
        kprintf("SYSCALL %lu (unknown)\r\n", f->rax);
        f->rax = (uint64_t)-1;
        break;
    }
}
