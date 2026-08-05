# Phase 1.5 — The Senses (design doc)

**Status:** Accepted — design for implementation (2026-08-05)
**References:** ADR-010 (scope), ADR-011 (CI), ADR-008 (input architecture, superseded clause), Roadmap

## Context & scope

Between Phase 1 (v0.2.0) and Phase 2, a small extension layer: kernel printf, CMOS RTC + `time`, CPUID + `cpuid`, VGA console scrolling, and the keyboard graduating from scancode viewer to REPL input. Ships as **v0.3.0**.

## Goals

1. `kprintf` — the kernel's print API (`%c %s %d %u %x %lx %p %%` + zero-padded width).
2. RTC: `time` command (BCD, UIP guard, century-aware).
3. CPUID: `cpuid` command (vendor, family/model/stepping, feature flags).
4. VGA scrolling + `vga` command (30 lines) for verification.
5. Keyboard REPL: set-1 keymap (normal+shift), shift state machine, shared input queue feeding the line editor; scancode line keeps its `KB: 0x..` format (now with the mapped char).
6. CI (ADR-011): GitHub Actions build+test on every push; `env.sh` platform detection.
7. test.sh v3 green; tag v0.3.0 + release.

## Non-goals

- Scancode set 2, serial IRQ4 (ADR-010 deferrals)
- Caps/ctrl/alt handling, non-US layouts, key repeat
- Scheduling, locks (input queue is SPSC by design)

## Design per feature

### kprintf (`src/kernel/printf.c`, ~120 lines)

- Streams directly to serial (no buffer); `stdarg.h` varargs.
- Format: `%c %s %d %u %x` (32-bit ints), `%ld %lu %lx` (64-bit), `%p` (16 hex), `%%`; optional `0`+width (e.g. `%02x`, `%02d`).
- `%d` handles negatives; width zero-pads.
- Refactor: `idt.c` dump_frame/messages, `repl.c`, `keyboard.c` use kprintf.

### RTC (`src/kernel/rtc.c`)

- CMOS ports 0x70/0x71; regs 0=sec, 2=min, 4=hour, 7=day, 8=month, 9=year, 0x32=century.
- Read loop: wait UIP clear (reg 0x0A bit 7), read all fields, re-read seconds, retry until stable.
- BCD→binary; century sanity-checked (19–25) with fallback 20.
- REPL `time` → `%04d-%02d-%02d %02d:%02d:%02d`.

### CPUID (`src/kernel/cpuid.c`)

- Leaf 0: vendor (EBX,EDX,ECX order), max leaf. Leaf 1: family/model/stepping + feature bits (mmx/sse/sse2/pae/x2apic/osxsave). Leaf 0x80000001: nx, lm.
- REPL `cpuid` command.

### VGA scrolling (`src/kernel/vga.c`)

- On cursor overflow: shift 24 rows up (word memmove), blank last row, cursor to last row.
- REPL `vga` command: writes 30 numbered lines (exercises scroll); serial confirms.

### Keyboard REPL (`src/kernel/keyboard.c`, `repl.c`)

- `map_norm[128]` / `map_shift[128]` designated-initializer tables (set 1, 0x02–0x53).
- Shift keys: 0x2A/0x36 make → shift=1; break (0xAA/0x36|0x80) → shift=0.
- Mapped make codes feed `repl_input_putc(ch)`; unmapped (E0-prefixed, F-keys) ignored.
- Input queue (SPSC ring, 128 B) in repl.c: serial polling pushes, keyboard IRQ pushes, REPL loop consumes — one line-editor path for both sources.
- Scancode line: `KB: 0x1e 'a'` (make with char), `KB: 0x9e` (break).

### CI (`env.sh`, `.github/workflows/build.yml`)

- `env.sh`: `uname -s` → MINGW* (Windows absolute paths, `python`) vs Linux (`nasm clang ld.lld llvm-objcopy qemu-system-x86_64`, `python3`).
- build.sh/test.sh source it (test.sh drops its hardcoded QEMU; `python` → `$PYTHON`).
- Workflow: checkout → apt install nasm clang lld llvm qemu-system-x86 → `./test.sh`.

## Test plan (test.sh v3)

- t1 regression (grep now `AIkOS v0.3.0`), t2 REPL, t3 keyboard extended (sendkey a/b/h/e/l/p/ret → `commands: help` via keyboard-typed command), t4 panic — unchanged logic.
- **t5 time**: `time` → grep `20\d\d-\d\d-\d\d \d\d:\d\d:\d\d`.
- **t6 cpuid**: `cpuid` → grep `cpuid: vendor`.
- Input chunking ≤15 bytes with gaps (war story #6) for all serial-input tests.

**Exit criterion:** test.sh v3 13/13 green locally + GitHub Actions run green → tag **v0.3.0** + release (ADR-004).

## Risks & pitfalls

- **%d vs %ld varargs**: 32-bit args are `va_arg(int/unsigned)` — reading them as 64-bit is UB/garbage (split by `l` prefix).
- **RTC UIP**: read race → retry loop; century reg may be absent → fallback.
- **Keyboard**: only read 0x60 on IRQ1; E0 sequences unhandled (ignored by `sc < 128` guard).
- **SPSC queue**: head/tail `volatile`, single producer (IRQ) + single consumer (REPL) only — no locks.
- **CI**: ubuntu `llvm-objcopy` lives in the `llvm` package; grep `-P` exists on ubuntu grep.
