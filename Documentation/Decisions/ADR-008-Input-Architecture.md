# ADR-008: Input architecture — scancode set 1, serial polling, REPL scope

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing (keyboard keymap / serial IRQ may supersede in later phases)

## Context

Phase 1 input has three axes: keyboard scancodes, serial RX, and the REPL's input scope.

- **Scancodes**: the PS/2 controller (8042) translates the keyboard's native set-2 codes to set-1 by default. Set 1 = zero translation work (hardware does it — fair game per ADR-003's "hardware is physics"); set 2 = we disable translation and build our own make/break state machine and table (purer from-scratch, but a rabbit hole of multi-byte and break-sequence edge cases).
- **Serial RX**: polling the UART's RX-ready bit vs wiring IRQ4 through the PIC. The 16550 interrupt path (IER/IIR/FIFO thresholds, spurious cases) is the fiddliest part of this phase for zero user-visible difference in a REPL.
- **REPL scope**: the roadmap exit criterion is "type into the REPL over serial, see echo" — serial-only input with the keyboard as a scancode viewer keeps Phase 1 to three subsystems instead of four.

## Decision

- **Keyboard: scancode set 1** (8042 default translation). IRQ1 handler reads port 0x60 and prints the scancode over serial (`KB: 0x1E` style). No keymap, no shift states, no REPL feed in Phase 1. No 8042 init — QEMU's PS/2 is ready by default (real-hardware init is a Phase 7 concern).
- **Serial RX: polled** in the REPL loop (LSR bit 0 → RBR read). No UART interrupts in Phase 1.
- **REPL: serial-only input, kernel-mode.** Commands: `help`, `echo <text>`, `ticks`, `version`, and `panic` (executes `ud2` — deliberately triggers the exception path so it is testable).
- Set 2 translation and serial IRQs are noted as potential Phase 1.5 refinements.

## Consequences

**Positive:**
- Scope control: the interrupt machinery is exercised by timer + keyboard; the REPL stays simple.
- Trivial automated testing: `-serial stdio` lets test.sh pipe commands into the VM and grep responses; keyboard tested via the monitor's `sendkey`.
- The `panic` command makes the exception path (ADR-009) testable end to end.

**Negative / costs:**
- The keyboard doesn't drive anything yet (becomes natural early Phase 2 work).
- Polling burns CPU while idle — irrelevant in a single-threaded kernel REPL.
