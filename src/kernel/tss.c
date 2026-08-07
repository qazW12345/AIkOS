// Task state segment (ADR-013): RSP0 = kernel stack, used by the CPU for
// Component: tss (task state segment)
// Provides: tss_init
// Depends on: entry.asm (gdt64 symbol, GDT slot 0x28 — the 16-byte 64-bit
//             TSS descriptor), linker.ld (stack_top)
// Owns: the TSS struct in .bss; RSP0 = kernel stack top
// the ring-3 -> ring-0 stack switch on int 0x80 and on user interrupts.
// Minimal: one TSS, single kernel stack (no preemptive multitasking yet —
// per-process kernel stacks arrive with the Phase 3 scheduler).

#include "kernel.h"

struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

static struct tss tss;

/* Dedicated kernel stack for ring-3 -> ring-0 transitions (interrupts,
 * syscalls, faults). RSP0 MUST NOT be the main kernel stack: the REPL chain
 * (kmain -> repl_run -> repl_handle -> cmd_run -> proc_run) runs near the top
 * of stack_top, and a ring-3 interrupt frame pushed at RSP0 would overwrite
 * the chain's frames, destroying the resume state (war story: EXCEPTION 6 at
 * rip=0x3 after "back in kernel" — fixed 2026-08-07, ADR-021/EXCEPTION-6 card).
 * Each privilege level gets its own stack; RSP0 is used ONLY for ring changes. */
static uint8_t int_stack[4096] __attribute__((aligned(16)));

extern char stack_top[];                /* linker.ld, top of the 16 KiB stack */
extern unsigned char gdt64[];           /* entry.asm */

/* TSS descriptor (16 bytes, 64-bit format) at gdt64 + 0x28 */
static void gdt_set_tss(uint64_t base, uint32_t limit)
{
    volatile uint8_t *d = gdt64 + 0x28;
    d[0] = (uint8_t)(limit & 0xFF);
    d[1] = (uint8_t)((limit >> 8) & 0xFF);
    d[2] = (uint8_t)(base & 0xFF);
    d[3] = (uint8_t)((base >> 8) & 0xFF);
    d[4] = (uint8_t)((base >> 16) & 0xFF);
    d[5] = 0x89;                        /* present, 64-bit TSS available */
    d[6] = 0x00;                        /* limit 19:16 + flags (G=0) */
    d[7] = (uint8_t)((base >> 24) & 0xFF);
    d[8] = (uint8_t)((base >> 32) & 0xFF);
    d[9] = (uint8_t)((base >> 40) & 0xFF);
    d[10] = (uint8_t)((base >> 48) & 0xFF);
    d[11] = (uint8_t)((base >> 56) & 0xFF);
    d[12] = d[13] = d[14] = d[15] = 0;
}

void tss_init(void)
{
    tss.rsp0 = (uint64_t)int_stack + sizeof(int_stack); /* top of the
                                                           dedicated stack */
    tss.iomap_base = (uint16_t)sizeof(struct tss);
    gdt_set_tss((uint64_t)&tss, (uint32_t)sizeof(struct tss) - 1);
    __asm__ volatile("ltr %w0" : : "r"((uint16_t)0x28));
}
