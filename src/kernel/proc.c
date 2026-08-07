// Processes (ADR-013): ring-3 programs in their own address space.
// Component: proc (ring-3 processes)
// Provides: proc_run, proc_run_fault, proc_run_elf; globals proc_resume_addr,
//           proc_resume_regs (consumed by entry.asm's user_return trampoline)
// Depends on: mm (pmm_alloc_page), entry.asm (kernel PD @0xB000, user_return),
//             tss (dedicated RSP0 interrupt stack — ring-3 frames never
//             clobber the REPL chain near stack_top)
// Owns: per-process page tables; the user region 0x200000-0x400000 (U/S);
//       resume-capture state; the ring-3 entry/return machinery
// Phase 2: one process at a time, but the full per-process page-table
// machinery. The process PD copies the kernel's identity PD (supervisor)
// and sets U/S only on the user region entries.

#include "kernel.h"

#define USER_ENTRY    0x200000
#define FAULT_ENTRY   0x220000
#define FAULT_STACK   0x240000
#define USER_PD_ENTRY 1                 /* 2 MiB entry: 0x200000-0x400000 */

static uint64_t proc_cr3;
static int proc_built;
uint64_t proc_resume_addr;  /* REPL resume point — captured as a VALUE: the
                               chain's return addresses live near stack_top
                               and ring-3 interrupt frames (pushed at the
                               dedicated RSP0 stack) must never overwrite
                               them — only the captured value survives */
uint64_t proc_resume_rsp;   /* the caller's rsp at the `call proc_run*`
                               site (= callee rbp + 16). The resumed code
                               expects rsp here, NOT the frame base: callers
                               with stack locals (cmd_runelf's subq $0x20)
                               run their epilogue from the call-site rsp.
                               (EXCEPTION-6 fix, 2026-08-07) */
struct proc_resume_regs {
    uint64_t rbp, rbx, r12, r13, r14, r15;
} proc_resume_regs;         /* callee-saved regs of the REPL chain: the user
                               program clobbers them (esp. rbp) and the
                               interrupt frame only preserves the kernel's
                               values from int-time, not the chain's.
                               rbp = the caller's frame base (saved rbp),
                               restored so [rbp] and [rbp+8] (saved rbp and
                               return address) are reachable from the
                               call-site rsp */

static void proc_build_address_space(void)
{
    volatile uint64_t *pml4 = (volatile uint64_t *)pmm_alloc_page();
    volatile uint64_t *pdpt = (volatile uint64_t *)pmm_alloc_page();
    volatile uint64_t *pd = (volatile uint64_t *)pmm_alloc_page();
    volatile uint64_t *kpd = (volatile uint64_t *)0xB000;   /* kernel PD */
    int i;

    for (i = 0; i < 512; i++)
        pml4[i] = pdpt[i] = pd[i] = 0;

    /* NOTE: user access requires U/S at EVERY level — PML4, PDPT, and PD.
     * The PDE alone is not enough (war story #7). */
    pml4[0] = (uint64_t)pdpt | 0x7;     /* present, rw, USER */
    pdpt[0] = (uint64_t)pd | 0x7;
    for (i = 0; i < 512; i++) {
        uint64_t e = kpd[i];
        if (i == USER_PD_ENTRY)
            e |= 0x4;                   /* U/S: only the user region */
        pd[i] = e;
    }
    proc_cr3 = (uint64_t)pml4;
    proc_built = 1;
}

static void proc_enter(uint64_t entry, uint64_t stack)
{
    /* User selectors carry RPL=3 (0x1B/0x23): iretq to an outer ring
     * requires CS.RPL == new CPL — RPL=0 selectors (0x18/0x20) are #GP'd. */
    uint64_t ss = 0x23, rflags = 0x202, cs = 0x1B;

    if (!proc_built)
        proc_build_address_space();

    __asm__ volatile("mov %0, %%cr3" : : "r"(proc_cr3) : "memory");
    /* iretq frame: ss, rsp, rflags, cs, rip — pushed low to high */
    __asm__ volatile(
        "pushq %0\n\t"
        "pushq %1\n\t"
        "pushq %2\n\t"
        "pushq %3\n\t"
        "pushq %4\n\t"
        "iretq"
        : : "r"(ss), "r"(stack), "r"(rflags), "r"(cs), "r"(entry)
        : "memory");
}

void proc_run(void)
{
    /* Capture the REPL resume point + callee-saved registers (frame-pointer
     * ABI guarantees: [rbp] = caller's saved rbp, [rbp+8] = our return
     * address). The caller's rsp at the call site = rbp + 16 — the resumed
     * code runs from THERE, not from the frame base (callers may hold
     * stack locals below rbp). Everything else gets destroyed by ring-3
     * execution and interrupt frames before we ever return. */
    __asm__ volatile(
        "movq 8(%%rbp), %0\n\t"
        "leaq 16(%%rbp), %1\n\t"
        "movq 0(%%rbp), %2\n\t"
        "movq %%rbx, %3\n\t"
        "movq %%r12, %4\n\t"
        "movq %%r13, %5\n\t"
        "movq %%r14, %6\n\t"
        "movq %%r15, %7\n\t"
        : "=r"(proc_resume_addr), "=r"(proc_resume_rsp),
          "=r"(proc_resume_regs.rbp),
          "=m"(proc_resume_regs.rbx), "=m"(proc_resume_regs.r12),
          "=m"(proc_resume_regs.r13), "=m"(proc_resume_regs.r14),
          "=m"(proc_resume_regs.r15));
    proc_enter(USER_ENTRY, USER_STACK);
}

/* Run an ELF program (ADR-016): entry is the ELF e_entry (the loader in
 * elf.c has already copied the segments into the user region). The resume
 * capture is duplicated per-caller deliberately — the asm reads THIS
 * function's frame ([rbp]); a shared helper would capture its own frame. */
void proc_run_elf(uint64_t entry, uint64_t stack)
{
    __asm__ volatile(
        "movq 8(%%rbp), %0\n\t"
        "leaq 16(%%rbp), %1\n\t"
        "movq 0(%%rbp), %2\n\t"
        "movq %%rbx, %3\n\t"
        "movq %%r12, %4\n\t"
        "movq %%r13, %5\n\t"
        "movq %%r14, %6\n\t"
        "movq %%r15, %7\n\t"
        : "=r"(proc_resume_addr), "=r"(proc_resume_rsp),
          "=r"(proc_resume_regs.rbp),
          "=m"(proc_resume_regs.rbx), "=m"(proc_resume_regs.r12),
          "=m"(proc_resume_regs.r13), "=m"(proc_resume_regs.r14),
          "=m"(proc_resume_regs.r15));
    proc_enter(entry, stack);
}

void proc_run_fault(void)
{
    __asm__ volatile(
        "movq 8(%%rbp), %0\n\t"
        "leaq 16(%%rbp), %1\n\t"
        "movq 0(%%rbp), %2\n\t"
        "movq %%rbx, %3\n\t"
        "movq %%r12, %4\n\t"
        "movq %%r13, %5\n\t"
        "movq %%r14, %6\n\t"
        "movq %%r15, %7\n\t"
        : "=r"(proc_resume_addr), "=r"(proc_resume_rsp),
          "=r"(proc_resume_regs.rbp),
          "=m"(proc_resume_regs.rbx), "=m"(proc_resume_regs.r12),
          "=m"(proc_resume_regs.r13), "=m"(proc_resume_regs.r14),
          "=m"(proc_resume_regs.r15));
    proc_enter(FAULT_ENTRY, FAULT_STACK);
}
