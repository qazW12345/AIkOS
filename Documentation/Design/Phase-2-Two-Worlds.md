# Phase 2 — Two Worlds (design doc)

**Status:** Accepted — design for implementation (2026-08-05)
**References:** ADR-012 (memory architecture), ADR-013 (privilege separation), ADR-009 (superseded for ring 3), Phase 0 design (memory map), Phase 1.5 design (input queue)
**Version target:** v0.4.0

## Context & scope

Privilege separation: ring-3 programs in their own address spaces, hardware only through syscalls. Exit criterion (Roadmap): **a ring-3 process runs and syscalls out** — sharpened below into test.sh v4.

## Goals

1. E820 memory discovery + bitmap page allocator (ADR-012)
2. Per-process page tables; kernel identity-mapped, supervisor-only, in every address space
3. Ring-3 transition via `iretq`; user GDT segments; minimal TSS (RSP0 = kernel stack)
4. `int 0x80` syscall gate (DPL 3) + dispatcher; syscalls `write` (1) and `exit` (2)
5. User fault policy: kill task, kernel lives (ADR-013)
6. User programs as separate disk blobs loaded by the boot sector; REPL `run` / `runfault`
7. test.sh v4 green; tag v0.4.0 + release

## Non-goals

- Preemptive multitasking, scheduler, per-process kernel stacks (Phase 3)
- ELF loader, filesystem (Phase 3 — the disk blobs are a stopgap)
- Higher-half kernel, buddy allocator, `syscall` instruction (recorded deferrals, ADR-012/013)
- Copy-on-write, demand paging, user pointer validation (Phase 3 hardening)

## Memory map (QEMU 32 MiB)

| Range | Owner |
|---|---|
| `0x005000` | E820 table (24-byte entries; count word at `0x4FFC`) |
| `0x007C00` | boot sector |
| `0x009000–0x00C000` | kernel page tables (Phase 0) — reserved |
| `0x00C000–0x00D000` | physical bitmap (4 KiB → 128 MiB coverage) |
| `0x100000–0x108000` | kernel |
| `0x200000–0x220000` | `user.bin` (16-sector budget, LBA 65) |
| `0x220000–0x240000` | `userfault.bin` (16-sector budget, LBA 81) |
| `0x240000–0x250000` | user stacks (grow down from `0x250000` / `0x240000`) |
| rest | free (allocator) |

## Design per subsystem

### E820 (boot.asm)
`int 15h AX=E820` loop (standard: ES:DI buffer at `0x5000`, EBX continuation, 24-byte entries), count word at `0x4FFC`. Kernel parses: type-1 (usable) regions become free; everything else used.

### Bitmap allocator (mm.c, new)
- Static 4 KiB bitmap in kernel .bss (covers 128 MiB).
- `pmm_init()`: mark all pages used; free type-1 E820 regions; then reserve `[0, 1 MiB)`, kernel (`kernel_start`/`kernel_end` linker symbols), page tables, E820 area, and both user blobs.
- `void *pmm_alloc_page(void)` / `void pmm_free_page(void *)` — first-fit scan, returns 4 KiB-aligned physical addresses (identity-mapped, so directly writable).

### Page tables & processes (proc.c, new)
- `struct process { uint64_t cr3; uint64_t entry; uint64_t rsp; int state; }` — one process in Phase 2, but the full machinery: each process gets its own PML4/PDPT/PD allocated from the allocator.
- Process PD = copy of kernel PD (1 GiB identity, supervisor) **with U/S set on the 2 MiB entries covering `0x200000–0x400000`** (user code/stack region). Kernel region stays supervisor — user code cannot touch it.
- `proc_enter(p)`: build the iretq frame — `ss=0x20, rsp=0x250000, rflags=0x202 (IF), cs=0x18, rip=0x200000` — and iret. CR3 switched before the frame loads.

### GDT & TSS (entry.asm + tss.c, new)
- entry.asm GDT64 extended: null, kcode `0x08`, kdata `0x10`, **ucode `0x18`** (DPL 3, L), **udata `0x20`** (DPL 3), **TSS `0x28`** (64-bit available, base patched by C).
- tss.c: 104-byte TSS with `rsp0 = stack_top`; patch the GDT descriptor; `ltr 0x28`.

### Syscalls (syscall.c, new; idt.c)
- Vector 0x80: interrupt gate with `type_attr 0xEE` (present, DPL 3).
- `isr_handler`: `vector == 0x80` → `syscall_dispatch(f)`; `eax` = number; `write`: emit `rdi`/`rsi` bytes to serial; `exit`: mark process dead, return to REPL.
- Everything else in the handler unchanged.

### Fault policy (idt.c)
- Exception path: check `f->cs & 3` — **ring 3** → `USER FAULT (vector N, name)` + frame dump + terminate task + `AIkOS>` prompt returns. **ring 0** → existing panic-and-halt (ADR-009).

### REPL (repl.c)
- New commands: `run` (enter ring 3 at `0x200000`) and `runfault` (enter at `0x220000`). `help` updated.

### User programs (user/, new)
- `user/main.c`: freestanding; `sys_write`/`sys_exit` wrappers (`int 0x80` inline asm); prints `hello from ring 3` via syscall; exits.
- `user/fault.c`: executes `mov cr3, rax` from ring 3 → #GP (privileged instruction).
- `user/linker.ld`: base `0x200000` (fault: `0x220000`), stack in .bss top.
- build.sh: compile both (`--target=x86_64-elf`, freestanding, `-fno-pic -fno-pie`), assert ≤ 8 KiB, `dd` into the image at LBA 65 / 81. boot.asm: two more DAP reads (user, fault) with sector-count defines.

## Test plan (test.sh v4)

- **t7** `run`: grep `SYSCALL 1 (write)` + `hello from ring 3` + `user exited` — the syscall round-trip.
- **t8** `runfault`: grep `USER FAULT` + `GENERAL PROTECTION` + a fresh `AIkOS>` prompt *after* the fault — the kernel survives user misbehavior.
- t1–t6: unchanged regression (14 checks). Total ≈ 19–20 checks.

**Exit criterion:** test.sh v4 all green locally + CI green → tag **v0.4.0** + release with disk.img (ADR-004).

## Risks & pitfalls

- **iretq frame order** must be exactly `ss, rsp, rflags, cs, rip`; rflags IF set; user RSP 16-aligned.
- **Gate DPL**: forgetting `0xEE` on vector 0x80 = #GP on first user syscall.
- **TSS**: `ltr` before any ring-3 entry; RSP0 wrong → first syscall corrupts the kernel stack.
- **U/S discipline**: U/S off on user pages → user #PF; U/S on kernel pages → user owns the kernel.
- **E820**: 24-byte entries, count word placement fixed (`0x4FFC`); entries beyond the 4 KiB table are dropped.
- **Bitmap**: never free page 0; low memory reserved wholesale.
- The Phase 1.5 input queue is untouched (serial REPL still drives `run`; keyboard input works as before).
