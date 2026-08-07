# ADR-021: Ring-3 Resume Integrity — Dedicated RSP0 Stack + Call-Site Restore

Status: Accepted
Supersedes: n/a

## Context

The ring-3 return path (`user_return` trampoline, ADR-013) resumed the REPL
call chain after a user program exited (`syscall 2`) or faulted. It relied on
two fragile assumptions, both broken in practice:

1. **RSP0 = stack_top.** Ring-3 interrupt frames (syscall/fault/IRQ) are
   pushed at RSP0. The REPL chain (`kmain → repl_run → repl_handle → dispatch
   → cmd_* → proc_run*`) runs near the top of the 16 KiB kernel stack, so a
   frame pushed at stack_top overwrote the chain's saved rbp / return
   addresses. The old code "parked" the stack 4 KiB below stack_top
   (`sub $0x1000` → `proc_kernel_rsp`) to dodge the clobber.

2. **Fixed park offset.** The parked-stack offset was calibrated for the
   call-chain depth at the time it was written. Commit `4a586102` (REPL
   command-table refactor) added two frames of indirection
   (`repl_handle → dispatch → cmd_runfault → proc_run_fault`); the park
   offset no longer matched the real call-site rsp, and the resumed epilogue
   (`pop rbp` / tail-call `ret`) read garbage → EXCEPTION 6 at rip=0x3 after
   "back in kernel" (bisected 2026-08-07; parent `b80ac47` passes strict
   t8, `4a586102` is the first bad commit).

3. **Pending PIT tick at the iretq.** The rewritten frame kept the USER's
   rflags (IF=1). The iretq that lands in `user_return` restores those
   rflags, so a PIT tick pending during the fault/syscall handler is
   delivered in the iretq's shadow — before `cli` can run — with rsp =
   stack_top (the rewritten `f->rsp`). Its frame lands exactly on the
   chain's saved-rbp / return-address slots. This is a phase-locked race:
   deterministic for a given boot/input timing, which is why a debug build
   whose extra serial output shifted the PIT phase appeared to "fix" it.

## Decision

Three-part fix, each addressing one failure mode:

1. **Dedicated interrupt stack.** `tss.rsp0` now points to the top of a new
   `int_stack[4096]` (tss.c) instead of the main stack's `stack_top`. Ring-3
   interrupt frames live on their own stack and can never touch the REPL
   chain.

2. **Call-site rsp capture.** Each `proc_run*` captures the caller's rsp at
   the call site (`leaq 16(%rbp)` — frame base + 16) into `proc_resume_rsp`
   (proc.c), replacing the fixed park offset. `user_return` restores
   `rsp = proc_resume_rsp` (entry.asm), so the resumed code runs from the
   exact stack position its epilogue expects, regardless of call-chain
   depth. `proc_kernel_rsp` is removed.

3. **IF cleared in the rewritten frame.** Both rewrite sites (syscall.c
   exit case 2, idt.c user-fault branch) now do `f->rflags &= ~0x200` so the
   iretq lands in `user_return` with interrupts disabled. `cli` in
   `user_return` is kept as defense-in-depth; `sti` re-enables interrupts
   only after the chain slots are consumed by the resumed code.

## Consequences

- All ring-3 return paths (t7 run / t8 runfault / t15 runelf / t18-t21
  syscall tests) resume cleanly: 47/47 suite, zero EXCEPTION/PAGE FAULT in
  all build/*.out outputs (strict audit, not the grep-based assertions that
  printed "back in kernel" before the fault).
- The tests' "kernel survives" assertions are still grep-based; the strict
  check (no EXCEPTION + fresh `AIkOS>` prompt after "back in kernel") is
  the real gate and must be added to test.sh as hardening (follow-up:
  t_4187cc33).
- The 16 KiB main stack is no longer shared with ring-3 interrupt frames;
  per-process kernel stacks remain a Phase 3 scheduler item (unchanged).
