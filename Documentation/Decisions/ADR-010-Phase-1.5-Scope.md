# ADR-010: Phase 1.5 scope — The Senses (extension layer)

- **Status:** Accepted (2026-08-05)
- **Supersedes:** ADR-008's scancode-viewer clause (keyboard becomes REPL input)
- **Superseded by:** nothing

## Context

After Phase 1 (v0.2.0), an extension layer between Phase 1 and Phase 2 was evaluated. Candidates: keyboard REPL input, scancode set 2 (raw), IRQ-driven serial RX, kernel printf, CMOS RTC, CPUID, VGA scrolling, GitHub Actions CI. Marcel selected the bundle in order: **printf, RTC, CPUID, VGA scroll (quick wins) → keyboard REPL → CI** (CI has its own ADR-011).

## Decision

Phase 1.5 scope (ships as **v0.3.0**):

1. **Kernel printf** — `kprintf` with `%c %s %d %u %x %lx %p %%` + zero-padded width (`%02x`). Replaces the ad-hoc serial write chains.
2. **CMOS RTC** — date/time read with update-in-progress guard; REPL `time` command.
3. **CPUID** — vendor, family/model/stepping, feature flags (mmx, sse, sse2, pae, x2apic, osxsave, nx, lm); REPL `cpuid` command.
4. **VGA console scrolling** — rows shift up instead of "stuck on last row"; REPL `vga` command (30 lines) for verification.
5. **Keyboard REPL** — set-1 keymap (normal + shift tables), shift state machine, keys feed the line editor through a shared input queue (also fed by polled serial). **Supersedes ADR-008's scancode-viewer role** (the viewer print stays, now with the mapped char).
6. Deferred again: scancode set 2, serial IRQ4 (revisit only if input drops bite in real use).

## Consequences

**Positive:**
- The keyboard finally drives the OS; typing works end-to-end (make/break → keymap → queue → REPL).
- printf compounds forever: dump_frame, REPL, and every future phase get a real print API.
- RTC/CPUID give the REPL a sense of time and machine; VGA becomes a real console.

**Negative / costs:**
- Keyboard input is US-layout only, no caps/ctrl handling (Phase 1.5 scope; keymap is data, easily extended).
- The input queue is single-producer/single-consumer (IRQ-safe by design); a general scheduler will need real locks (Phase 2+).
