# Phase 1 — The Machine Wakes (design doc)

**Status:** Accepted — design for implementation (2026-08-05)
**References:** ADR-007 (interrupt architecture: PIC + PIT), ADR-008 (input: set-1 scancodes, serial polling, REPL scope), ADR-009 (exception policy: panic-and-halt); Roadmap Phase 1

## Context & scope

Phase 0 gave AIkOS a voice (banner) but no senses. Phase 1 adds the interrupt machinery and the first input paths: CPU exceptions, a 100 Hz timer tick, a PS/2 keyboard, and a kernel-mode REPL over serial. The exit criterion: **type into the REPL over serial, see echo** — sharpened into a scripted test below.

## Goals

1. IDT with 256 entries; all 32 exception vectors handled (panic-and-halt with register dump, ADR-009).
2. PIC remap (IRQ0–7 → 0x20–0x27, IRQ8–15 → 0x28–0x2F); timer + keyboard IRQs live.
3. PIT at 100 Hz; `volatile uint64_t ticks` counter.
4. PS/2 keyboard: IRQ1 reads scancodes (set 1), prints them over serial.
5. Kernel-mode REPL over serial (polled RX): `help`, `echo <text>`, `ticks`, `version`, `panic`.
6. test.sh v2 green (below); Phase 0 regression stays green; tag v0.2.0 + release.

## Non-goals (deliberately deferred)

- User mode, syscalls, TSS stack switching → Phase 2 (interrupts stay ring-0-only; no TSS needed)
- APIC/SMP, HPET → ADR-007 (their own ADRs when needed)
- UART interrupts (IRQ4) → ADR-008 (RX is polled)
- Keyboard keymap / shift states / REPL feed → ADR-008 (Phase 1.5 or Phase 2)
- Memory management beyond the Phase 0 identity map → Phase 2
- Real-hardware keyboard/8042 init → Phase 7 (QEMU's PS/2 is ready by default)

## Architecture

### IDT and stubs

- 256 entries (16 bytes each = 4 KiB) in kernel `.data`; `lidt` with limit = size−1.
- All entries: interrupt gates (type 0xE, DPL 0, present), selector 0x08 (kernel code).
- NASM macro generates one stub per vector: error-code vectors (8, 10–14, 17) keep the CPU-pushed error code; all others push 0; each stub pushes its vector number, then jumps to a common entry.
- Common entry (asm): saves all GPRs, passes a pointer to the saved state to a C handler (`isr_handler`), restores, `iretq`. The CPU-pushed frame (SS, RSP, RFLAGS, CS, RIP, [error code]) must stay untouched below the GPR save area — iretq pops exactly that.
- Vectors 32–255: default "unhandled interrupt" panic handler (also via stub, vector pushed).

### PIC (ADR-007)

- ICW1 (0x11, edge/ICW4) → ICW2 (master 0x20 / slave 0x28) → ICW3 (cascade: master bit 2, slave 2) → ICW4 (0x01, 8086 mode).
- Mask (OCW1): enable only IRQ0 and IRQ1 in Phase 1.
- EOI: OCW2 0x20 (master) / 0xA0 (slave) after handling; spurious IRQ7: check ISR (OCW3 read) before sending EOI.

### PIT (ADR-007)

- Channel 0, mode 3 (square wave), divisor 11931 → 100 Hz. `outb` to 0x43 (mode/command) then 0x40 (divisor LSB, MSB).
- IRQ0 handler: `ticks++`.

### Keyboard (ADR-008)

- IRQ1 handler: read port 0x60 → scancode (set 1, 8042-translated). Print `KB: 0xXX` over serial.
- Read 0x60 **only** inside IRQ1 (unconditional reads return garbage).

### Serial RX + REPL (ADR-008)

- Polled: LSR (0x3FD) bit 0 → RBR (0x3F8) read in the REPL loop.
- Line editor: echo each char, backspace removes, Enter dispatches. `\r` and `\n` both accepted (serial terminals send `\r`).
- Commands: `help` (list), `echo <text>` (echo the rest), `ticks` (print counter), `version`, `panic` (execute `ud2` → exercises ADR-009).
- kmain: after Phase 0 banner, run `repl_run()` as the main loop (interrupts already on).

## Compile flags

- Add **`-mgeneral-regs-only`** to kernel CFLAGS: interrupt handlers must not touch XMM/FPU registers (we don't save them). This is the cheap insurance that prevents clobbering interrupted code's SIMD state.

## Memory layout additions

| Region | Use |
|---|---|
| kernel `.data` | IDT (4 KiB, filled at init) |
| kernel `.bss` | `ticks` counter; REPL line buffer (128 B) |
| everything else | unchanged from Phase 0 |

No new fixed addresses — the kernel image just grows (budget: 64 sectors ≈ 32 KiB, current 1 KiB — ample).

## Test plan (test.sh v2)

1. **Regression (unchanged):** headless boot, `-serial file:`, grep banner. Must stay green.
2. **REPL test:** `printf 'help\necho hello\nticks\nticks\n' | qemu -serial stdio -display none -no-reboot -m 32M`; grep stdout for: `help` output listing commands, `hello` echoed, two different `ticks` values (counter increments).
3. **Keyboard test:** `(sleep 5; echo "sendkey a"; sleep 1; echo "sendkey b"; sleep 1; echo quit) | qemu -serial file:keyboard.log -monitor stdio`; grep log for `KB: 0x1E` and `KB: 0x30` (set-1 make codes for `a`/`b`).
4. **Exception test:** REPL `panic` → grep serial for `EXCEPTION` + vector 6 (`#UD`) + register dump; VM halts (timeout expected).

**Exit criterion:** all four green → tag **v0.2.0** + GitHub release (ADR-004).

## Risks & pitfalls

- **PIC init order** (ICW1–4) — wrong sequence = no interrupts at all; verify with IRQ0 ticking before anything else.
- **EOI discipline** — EOI after handling; spurious IRQ7/15 must not EOI blindly (ISR check) or interrupts wedge.
- **lidt limit** = size−1 (classic off-by-one).
- **iretq frame** — common entry must preserve the CPU-pushed frame exactly; errors here produce bizarre faults *inside* handlers.
- **-mgeneral-regs-only** — without it, clang may emit XMM ops in handlers and silently corrupt interrupted code.
- **PIT divisor** 1193182/100 = 11931; mode 3.
- **Keyboard** — read 0x60 only on IRQ1.
- **Serial terminals send `\r`** for Enter — accept both; echo `\r\n` for line endings.
- **-serial stdio and -monitor stdio conflict** — REPL test uses serial-stdio (no monitor); keyboard test uses serial-file + monitor-stdio.
- **Interrupts must be on** (`sti`) before the REPL loop — and the boot chain runs with IF=0; the switch-on point is kmain.

## Deliverables

```
src/kernel/interrupt.asm   (stubs + common entry)
src/kernel/idt.c           (IDT setup + isr_handler)
src/kernel/pic.c           (PIC init + EOI)
src/kernel/pit.c           (timer init + tick counter)
src/kernel/keyboard.c      (IRQ1 scancode reader)
src/kernel/repl.c          (serial line editor + commands)
test.sh (v2)               (regression + REPL + keyboard + exception)
```

Tag v0.2.0; release with disk.img (ADR-004).
