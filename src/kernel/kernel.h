// AIkOS kernel — shared declarations (freestanding, no libc).

#ifndef AIKOS_KERNEL_H
#define AIKOS_KERNEL_H
// kernel.h — the component index (ADR-014). Every component declares its
// public API here; the per-component contract (Provides / Depends on / Owns)
// lives in each file's header block. Read a contract before its code.

#include <stdint.h>
#include <stddef.h>

/* interrupt frame — layout must match interrupt.asm's common entry exactly */
struct isr_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

/* entry.asm: kernel trampoline that returns to the REPL call chain */
void user_return(void);

/* linker.ld: top of the 16 KiB kernel stack (TSS RSP0, frame hygiene) */
extern char stack_top[];

/* port I/O — inline here: several drivers need it (serial.c, pic.c, ...) */
static inline void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %w1" : : "a"(val), "d"(port));
}

static inline unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile("inb %w1, %0" : "=a"(ret) : "d"(port));
    return ret;
}

/* printf.c */
void kprintf(const char *fmt, ...);

/* serial.c */
void serial_init(void);
void serial_putc(char c);
void serial_write_string(const char *str);
void serial_write_hex(uint64_t v, int digits);
void serial_write_dec(uint64_t v);
int serial_rx_ready(void);
char serial_read_char(void);

/* vga.c */
void vga_clear(void);
void vga_write_string(const char *str);

/* idt.c */
void idt_init(void);

/* pic.c */
void pic_init(void);
void pic_eoi(int irq);
int pic_is_spurious(int irq);

/* pit.c */
void pit_init(void);
void pit_tick(void);
uint64_t pit_get_ticks(void);

/* keyboard.c */
void keyboard_irq(void);

/* rtc.c */
void rtc_read(int *sec, int *min, int *hour, int *day, int *month, int *year);

/* cpuid.c */
void cpuid_dump(void);

/* repl.c */
void repl_run(void);
void repl_input_putc(char c);

/* mm.c — physical memory (ADR-012) */
void pmm_init(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
void *pmm_alloc_contiguous(uint32_t pages);

/* buddy.c — kernel heap (ADR-017) */
void buddy_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void heap_stats(uint64_t *free_pages, int *largest_order, uint64_t *allocated_blocks);

/* tss.c — task state segment (ADR-013) */
void tss_init(void);

/* syscall.c — int 0x80 dispatcher (ADR-013) */
void syscall_dispatch(struct isr_frame *f);

/* proc.c — ring-3 processes (ADR-013) */
void proc_run(void);
void proc_run_fault(void);
void proc_run_elf(uint64_t entry, uint64_t stack);   /* elf.c loads segments (ADR-016) */
extern uint64_t proc_kernel_rsp;   /* parked stack (below interrupt-frame zone) */
extern uint64_t proc_resume_addr;  /* REPL resume point (captured value) */

/* hexdump.c — memory dump tool */
void hexdump(uint64_t addr, uint64_t len);

/* fs.c — AIkFS driver (ADR-015) */
void fs_init(void);
void fs_ls(void);
int fs_read(const char *name, uint8_t **out, uint32_t *out_size);
void fs_info(void);

#endif
