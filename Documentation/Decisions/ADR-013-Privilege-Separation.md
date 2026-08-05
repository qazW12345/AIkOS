# ADR-013: Privilege separation — user mode, syscalls, fault policy

- **Status:** Accepted (2026-08-05)
- **Supersedes:** ADR-009's ring-3 clause (user faults kill the task, not the kernel); ADR-008's serial-only input note (unaffected, noted for completeness)
- **Superseded by:** nothing

## Context

Phase 2's core: programs run in ring 3, in their own address spaces, reaching the kernel only through syscalls. Decisions: the syscall mechanism, the delivery of user programs, and what happens when a user program misbehaves.

## Decision

1. **`int 0x80` syscall gate** — a DPL-3 interrupt gate at vector 0x80 riding the existing isr_common machinery (frame save/restore already battle-tested from Phase 1). The CPU performs the ring-3→ring-0 stack switch automatically via the TSS. **`syscall`/`sysret` deferred** — recorded optimization (no automatic stack switch, LSTAR/STAR MSR setup, new failure surface for zero measurable gain at this scale).
2. **One minimal TSS** — RSP0 = the existing kernel stack; `ltr` after patching the GDT TSS descriptor. Single kernel stack (no preemptive multitasking until Phase 3 — that's when per-process kernel stacks arrive).
3. **GDT additions** — user code (DPL 3, long mode) at `0x18`, user data at `0x20`, TSS descriptor at `0x28`; ring transition via `iretq`.
4. **User programs as separate disk blobs** — build.sh appends `user.bin` (LBA 65) and `userfault.bin` (LBA 81) after the kernel; the boot sector DAP-reads them to `0x200000`/`0x220000` (the same mechanism that loads the kernel). Programs are physically separate from the kernel from day one — the honest precursor to Phase 3's filesystem. REPL gains `run` and `runfault` commands.
5. **User fault policy: kill the task, kernel lives** — a ring-3 exception (detected via CS.RPL in the frame) prints `USER FAULT (vector N)` with the register frame and terminates the process; the kernel returns to the REPL. Kernel-mode exceptions keep the ADR-009 panic-and-halt (unchanged).
6. **Syscall ABI** — number in `eax`, args in `rdi/rsi/rdx/r10` (SysV-style, C-friendly). Phase 2 syscalls: `1 = write` (string pointer + length to serial), `2 = exit`. User pointer validation deferred to Phase 3 (single trusted process for now — documented).

## Consequences

**Positive:** tiny delta on proven machinery (one DPL-3 gate + dispatcher); the fault test becomes a feature — test.sh proves a misbehaving program cannot kill the OS; ADR-009's recorded revisit point ("when processes exist, Phase 2") is honored.
**Negative:** `int 0x80` is slower than `syscall` (irrelevant); no user pointer validation yet (documented, single-process trust); single kernel stack means a stuck kernel-mode handler still halts everything (unchanged from Phase 1).
