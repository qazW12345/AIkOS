# ADR-009: Exception policy — panic-and-halt with register dump

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing (will be revisited when processes exist, Phase 2+)

## Context

Phase 0's debugging war stories all share one shape: a kernel executing in a corrupted state, wasting hours. Once the IDT exists, every CPU exception (divide error, #GP, #PF, ...) becomes visible to us — and we must decide what the kernel does about it.

Options: **panic-and-halt** (print what we know, stop) vs **try-to-continue** (log and return to the interrupted code).

## Decision

- All 32 CPU exception vectors get a default handler that:
  1. prints the vector number and error code (where the CPU provides one; 0 elsewhere),
  2. dumps the general-purpose registers (the interrupted state, straight off the stack),
  3. prints CR2 when the exception is a page fault,
  4. then `cli; hlt` — the kernel stops, visibly, with evidence.
- No continuation after a fault. Safety-first: a faulting kernel must not stumble onward in an unknown state (Marcel's standing principle: blockers before exploits, safety before capability).

## Consequences

**Positive:**
- Every fault becomes a readable, recorded event — the debugging gift Phase 0 proved we need.
- No silent corruption; the REPL's `panic` command (ADR-008) makes the path testable.

**Negative / costs:**
- Zero resilience — one fault kills the kernel. Acceptable: there are no processes to protect yet.
- Revisit when user mode arrives (Phase 2): the policy then becomes "kill the task, keep the kernel" — this ADR will be superseded with the reasoning recorded.
