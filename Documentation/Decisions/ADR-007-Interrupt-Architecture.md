# ADR-007: Interrupt architecture — PIC 8259A + PIT 8254

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing (APIC/SMP will supersede when they arrive)

## Context

Phase 1 (The Machine Wakes) needs interrupt routing and a tick source. Options considered:

- **PIC (8259A)**: the classic cascaded pair; simple ICW init; remaps IRQs to chosen vectors; rock-solid in QEMU and on real hardware; everything we need for a single-core kernel.
- **APIC (local + I/O)**: what modern hardware actually uses; needed for SMP and per-CPU routing — none of which Phase 1 needs; MSR setup + redirection tables add real complexity for zero current benefit.
- **Timer sources**: PIT 8254 (canonical, divisor math, IRQ0), local APIC timer (requires APIC), HPET (most complex init, ACPI discovery) — for a tick counter, PIT is the right-sized tool.

## Decision

- **Interrupt controller: two cascaded 8259A PICs.** IRQ0–7 → vectors 0x20–0x27, IRQ8–15 → 0x28–0x2F (standard remap). Full ICW1–4 init sequence; mask everything except IRQ0 (timer) and IRQ1 (keyboard) in Phase 1; EOI discipline via OCW2, with spurious-IRQ7/15 handling (check ISR before EOI).
- **Timer: PIT channel 0, mode 3 (square wave), 100 Hz** (divisor 11931 = 1193182/100). IRQ0 increments a `volatile uint64_t` tick counter visible to the REPL.
- **APIC / HPET deferred** — each becomes its own ADR when actually needed (SMP, or per-CPU timers).

## Consequences

**Positive:**
- Canonical, boring, reliable — every reference implements exactly this; QEMU behaves perfectly.
- Minimal moving parts: the interrupt path stays debuggable (Phase 0's lesson: complexity is the enemy).

**Negative / costs:**
- Legacy hardware; the PIC has quirks (spurious IRQ7) we must code around.
- Revisit required when SMP arrives — recorded here so it isn't forgotten.
