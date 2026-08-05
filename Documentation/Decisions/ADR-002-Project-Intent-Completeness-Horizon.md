# ADR-002: Refine project intent — completeness as the horizon

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing (refines the Roadmap's non-goals, which ADR-001 established the process for)
- **Superseded by:** nothing

## Context

The original Roadmap non-goal read: "Not a daily-driver OS. Not a Windows/Linux replacement. Curiosity and learning are the product."

Marcel clarified the intent behind that statement: the reason AIkOS will never be a daily-driver OS is **not a chosen cap on ambition** — it's that **no third-party ecosystem will ever exist for it**. Nobody else will write software for AIkOS; Windows has millions of programmers building for it, and we have ourselves. Within that reality, the aspiration is to make AIkOS **as complete as possible**.

## Decision

- Reframe the non-goal: "not a daily-driver OS" is a **consequence of the ecosystem reality**, not a ceiling we chose.
- **Completeness is the project horizon.** "Complete" means *self-sufficient*: AIkOS should eventually provide everything a user needs without third-party software — its own shell, its own applications, its own tooling, its own compiler (the roadmap's phases already point this way; this ADR makes the intent explicit).
- Going forward, scope decisions should weigh "does this move us toward completeness?" — and "not a daily driver" must never be used as an excuse to cut corners.

## Consequences

**Positive:**
- Ambition preserved and made explicit; the project has a positive horizon, not just a list of exclusions.
- Future sessions know the intent: completeness is the target, ecosystem absence is the constraint.
- The roadmap's later phases (own language, own apps) get their philosophical justification recorded.

**Negative / costs:**
- "As complete as possible" is unbounded — mitigated by the phased roadmap and exit criteria; resources remain single-project-scale.
- Risk of scope creep disguised as completeness — the exit-criteria discipline (ADR-001) is the counterweight.
- This intent may need revisiting if the ecosystem reality changes (e.g., contributors appear) — then a new ADR supersedes this one.
