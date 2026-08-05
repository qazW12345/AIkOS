// IDT setup + interrupt/exception handling (ADR-007, ADR-009, ADR-013).

#include "kernel.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern uint64_t isr_addr_table[256];

static struct idt_entry idt[256];

static const char *const exception_names[32] = {
    "DIVIDE ERROR", "DEBUG", "NMI", "BREAKPOINT", "OVERFLOW",
    "BOUND RANGE", "INVALID OPCODE", "DEVICE NOT AVAILABLE", "DOUBLE FAULT",
    "COPROC SEGMENT OVERRUN", "INVALID TSS", "SEGMENT NOT PRESENT",
    "STACK FAULT", "GENERAL PROTECTION", "PAGE FAULT", "RESERVED",
    "X87 FLOAT EXCEPTION", "ALIGNMENT CHECK", "MACHINE CHECK",
    "SIMD FLOAT EXCEPTION", "VIRTUALIZATION", "RESERVED", "RESERVED",
    "RESERVED", "RESERVED", "RESERVED", "RESERVED", "RESERVED",
    "RESERVED", "RESERVED", "RESERVED", "RESERVED"
};

static void idt_set(int n, uint64_t addr, uint8_t attr)
{
    idt[n].offset_low = addr & 0xFFFF;
    idt[n].selector = 0x08;          // kernel code segment
    idt[n].ist = 0;
    idt[n].type_attr = attr;         // present, DPL in attr
    idt[n].offset_mid = (addr >> 16) & 0xFFFF;
    idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[n].zero = 0;
}

void idt_init(void)
{
    int i;
    struct idt_ptr ptr;

    for (i = 0; i < 256; i++)
        idt_set(i, isr_addr_table[i], 0x8E);   // interrupt gate, DPL 0
    idt_set(0x80, isr_addr_table[0x80], 0xEE); // int 0x80: DPL 3 (ADR-013)

    ptr.limit = (uint16_t)(sizeof(idt) - 1);
    ptr.base = (uint64_t)idt;
    __asm__ volatile("lidt %0" : : "m"(ptr));
}

static void dump_frame(struct isr_frame *f)
{
    kprintf("rax=%lx rbx=%lx rcx=%lx rdx=%lx\r\n", f->rax, f->rbx, f->rcx, f->rdx);
    kprintf("rsi=%lx rdi=%lx rbp=%lx rsp=%lx\r\n", f->rsi, f->rdi, f->rbp, f->rsp);
    kprintf("r8 =%lx r9 =%lx r10=%lx r11=%lx\r\n", f->r8, f->r9, f->r10, f->r11);
    kprintf("r12=%lx r13=%lx r14=%lx r15=%lx\r\n", f->r12, f->r13, f->r14, f->r15);
    kprintf("rip=%lx cs=%lx rflags=%lx ss=%lx\r\n", f->rip, f->cs, f->rflags, f->rsp);
}

static void panic_halt(void)
{
    for (;;)
        __asm__ volatile("cli; hlt");
}

void isr_handler(struct isr_frame *f)
{
    if (f->vector == 0x80) {             // syscall gate (ADR-013)
        syscall_dispatch(f);
        return;
    }
    if (f->vector < 32) {
        if ((f->cs & 3) == 3) {
            // user-mode fault (ADR-013): kill the task, kernel lives.
            kprintf("\r\nUSER FAULT %d (%s) error=%lx", (int)f->vector,
                    exception_names[f->vector], f->error_code);
            if (f->vector == 14) {
                uint64_t cr2;
                __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
                kprintf(" cr2=%lx", cr2);
            }
            kprintf("\r\n");
            dump_frame(f);
            kprintf("user program terminated\r\n");
            /* Full frame-tail rewrite for a same-ring return (ADR-013):
             * iretq validates SS.RPL == CPL — the user SS (0x23) must go. */
            f->cs = 0x08;
            f->ss = 0x10;
            f->rsp = (uint64_t)stack_top;
            f->rip = (uint64_t)user_return;
            return;
        }
        // CPU exception from the kernel -> panic-and-halt (ADR-009)
        kprintf("\r\nEXCEPTION %d (%s) error=%lx", (int)f->vector,
                exception_names[f->vector], f->error_code);
        if (f->vector == 14) {
            uint64_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            kprintf(" cr2=%lx", cr2);
        }
        kprintf("\r\n");
        dump_frame(f);
        panic_halt();
    }
    if (f->vector < 48) {
        // PIC IRQ (ADR-007): 32-47 map to IRQ0-15
        int irq = (int)f->vector - 32;
        if (pic_is_spurious(irq))
            return;
        if (irq == 0)
            pit_tick();
        else if (irq == 1)
            keyboard_irq();
        pic_eoi(irq);
        return;
    }
    kprintf("\r\nUNHANDLED INTERRUPT %d\r\n", (int)f->vector);
    dump_frame(f);
    panic_halt();
}
