// Processes (ADR-013): ring-3 programs in their own address space.
// Phase 2: one process at a time, but the full per-process page-table
// machinery. The process PD copies the kernel's identity PD (supervisor)
// and sets U/S only on the user region entries.

#include "kernel.h"

#define USER_ENTRY    0x200000
#define FAULT_ENTRY   0x220000
#define USER_STACK    0x250000          /* grows down */
#define FAULT_STACK   0x240000
#define USER_PD_ENTRY 1                 /* 2 MiB entry: 0x200000-0x400000 */

static uint64_t proc_cr3;
static int proc_built;
uint64_t proc_kernel_rsp;   /* parked stack (below the interrupt-frame zone) */
uint64_t proc_resume_addr;  /* REPL resume point — captured as a VALUE: the
                               chain's return addresses live near stack_top
                               and ring-3 interrupt frames (pushed at RSP0)
                               overwrite them — only the captured value
                               survives */
struct proc_resume_regs {
    uint64_t rbp, rbx, r12, r13, r14, r15;
} proc_resume_regs;         /* callee-saved regs of the REPL chain: the user
                               program clobbers them (esp. rbp) and the
                               interrupt frame only preserves the kernel's
                               values from int-time, not the chain's */

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

    /* Park the stack 4 KiB below the interrupt-frame zone: ring-3
     * interrupts push their frames at RSP0 (= stack_top) and would
     * otherwise clobber a chain running near the top. */
    __asm__ volatile("sub $0x1000, %%rsp" : : : "memory");
    __asm__ volatile("mov %%rsp, %0" : "=r"(proc_kernel_rsp) :: "memory");
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
     * address). Everything else gets destroyed by ring-3 execution and
     * interrupt frames before we ever return. */
    __asm__ volatile(
        "movq 8(%%rbp), %0\n\t"
        "movq 0(%%rbp), %1\n\t"
        "movq %%rbx, %2\n\t"
        "movq %%r12, %3\n\t"
        "movq %%r13, %4\n\t"
        "movq %%r14, %5\n\t"
        "movq %%r15, %6\n\t"
        : "=r"(proc_resume_addr), "=r"(proc_resume_regs.rbp),
          "=m"(proc_resume_regs.rbx), "=m"(proc_resume_regs.r12),
          "=m"(proc_resume_regs.r13), "=m"(proc_resume_regs.r14),
          "=m"(proc_resume_regs.r15));
    proc_enter(USER_ENTRY, USER_STACK);
}

void proc_run_fault(void)
{
    __asm__ volatile(
        "movq 8(%%rbp), %0\n\t"
        "movq 0(%%rbp), %1\n\t"
        "movq %%rbx, %2\n\t"
        "movq %%r12, %3\n\t"
        "movq %%r13, %4\n\t"
        "movq %%r14, %5\n\t"
        "movq %%r15, %6\n\t"
        : "=r"(proc_resume_addr), "=r"(proc_resume_regs.rbp),
          "=m"(proc_resume_regs.rbx), "=m"(proc_resume_regs.r12),
          "=m"(proc_resume_regs.r13), "=m"(proc_resume_regs.r14),
          "=m"(proc_resume_regs.r15));
    proc_enter(FAULT_ENTRY, FAULT_STACK);
}
