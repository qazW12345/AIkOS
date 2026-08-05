# AIkOS Roadmap

The project plan. Every phase has an **exit criterion** — a phase is done when its test demonstrably passes, nothing less.

**Current phase: Phase 2 — design done** (implementation next; v0.4.0 target)

## Phases

| # | Phase | Goal | Exit criterion | Status |
|---|---|---|---|---|
| 0 | Proof of Life | Boot in QEMU; banner on VGA + serial console | `test.sh` green (headless QEMU boot, serial log contains `AIkOS v0.1.0`); VGA banner verified via screendump; tagged release v0.1.0 | ✅ **DONE — v0.1.0 (2026-08-05)** |
| 1 | The Machine Wakes | Interrupts, timer ticks, PS/2 keyboard, kernel-mode REPL | test.sh v2 green: REPL responds to help/echo/ticks over `-serial stdio`; ticks increments; keyboard scancodes via monitor `sendkey`; `panic` produces exception dump; Phase 0 regression green; tagged release v0.2.0 | ✅ **DONE — v0.2.0 (2026-08-05)** |
| 1.5 | The Senses (extension) | kprintf, RTC+time, CPUID, VGA scroll, keyboard REPL, CI | test.sh v3 green: time/cpuid commands respond; keyboard-typed command works (sendkey h/e/l/p/ret); all Phase 1 tests stay green; GitHub Actions run green; tagged release v0.3.0 | ✅ **DONE — v0.3.0 (2026-08-05)** — CI green on GitHub Actions |
| 2 | Two Worlds | Paging, user mode, syscalls | test.sh v4 green: ring-3 program runs and syscalls out (`SYSCALL 1 (write)` + user text over serial); user fault kills the task without a kernel panic (REPL responsive after); Phase 1.5 regression green; tagged release v0.4.0 | ⬜ design done (2026-08-05) |
| 3 | Memory & Files | Allocators, filesystem, ELF loader, first userland apps | Boots from disk image; runs /bin apps | ⬜ |
| 4 | A Face | Framebuffer GUI, windows, compositor | Windows draw, drag, close — verified via QEMU screendump | ⬜ |
| 5 | The Wire | NIC driver, ARP/IP/TCP, socket API | A fetch-style app works over QEMU virtual network | ⬜ |
| 6 | Own Tongue | Compiler for our own language | Compile + run a program on AIkOS itself | ⬜ |
| 7 | Real Metal | Boot from USB on real hardware | Boots on a physical machine | ⬜ |

## Non-goals (written down so nobody drifts — these are *chosen* exclusions, not forgotten items)

- **Not a daily-driver OS** — but this is a **consequence, not a ceiling**: no third-party ecosystem will ever exist for AIkOS (nobody else will write software for it). Within that reality, **completeness is the horizon** (ADR-002): the goal is an OS as complete and *self-sufficient* as possible — its own shell, apps, tooling, compiler. Not a Windows/Linux replacement — but not a toy either.
- **No POSIX compatibility.** We are not cloning Linux.
- **No multi-user security model.** User mode (Phase 2) exists to learn the mechanics, not to protect users.
- **No networking before Phase 5.**
- **No broad real-hardware driver support before Phase 7.** Emulator-first, always.

## Principles

- **From scratch** (ADR-003): the OS itself — bootloader, kernel, drivers, filesystem, GUI, language — is written from zero. The build toolchain (compiler, assembler, emulator) is borrowed bootstrap only, because you need a compiler before you can write a compiler.
- **Completeness** (ADR-002): not a daily driver because no ecosystem will exist for it; complete = self-sufficient — our own shell, apps, tooling, compiler.
- **Exit criteria are real** (ADR-004): a phase ships when its test demonstrably passes — and it ships as a tagged GitHub release (`v0.1.0`, `v0.2.0`, ...).
- **Docs are code** (ADR-001): versioned, backed up, and the handoff mechanism between sessions.

## How the roadmap changes

If reality disagrees with this roadmap, we don't silently edit it — we write an ADR explaining the change, then update the roadmap to match. The roadmap describes *where we are going*; ADRs describe *how we got here and why*.

## Notes

- Every phase gets its own mini design doc in `Design/` before implementation starts (written for the ambiguous parts only — if there's no trade-off to weigh, a design doc is overhead).
- Phase 0 decisions made (2026-08-05): **ADR-005** (toolchain: clang/LLVM + NASM + bash build script), **ADR-006** (boot: custom boot sector, long mode in kernel entry). Design: `Design/Phase-0-Proof-of-Life.md`.
- Phase 1 decisions made (2026-08-05): **ADR-007** (PIC + PIT 100 Hz), **ADR-008** (scancode set 1, serial polling, REPL scope + `panic` command), **ADR-009** (panic-and-halt exception policy). Design: `Design/Phase-1-The-Machine-Wakes.md`.
- Phase 1.5 decisions made (2026-08-05): **ADR-010** (scope: kprintf, RTC, CPUID, VGA scroll, keyboard REPL; set-2 and serial IRQ deferred), **ADR-011** (CI adoption, supersedes ADR-004's deferral). Design: `Design/Phase-1.5-The-Senses.md`.
- Phase 2 decisions made (2026-08-05): **ADR-012** (memory: E820 + bitmap allocator + identity-mapped kernel; buddy + higher-half deferred), **ADR-013** (privilege separation: int 0x80, TSS, disk-blob user programs, user faults kill the task — supersedes ADR-009's ring-3 clause). Design: `Design/Phase-2-Two-Worlds.md`.
