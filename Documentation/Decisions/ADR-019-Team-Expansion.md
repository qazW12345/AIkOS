# ADR-019 — Team Expansion: Role-Specialized Workers

**Status:** Accepted, 2026-08-06
**Supersedes:** the "two workers" clause of ADR-018 (kanban orchestration)
**Context:** ADR-018 established the kanban board with two interchangeable
implementer profiles (nemotron, gemini). After a 4-card live trial on
2026-08-06 (memmap, contracts validator, research brief, docs audit — 3 landed
+ 1 provider-quota block), the team was reorganized by ROLE rather than by
capability, and the lean-profile pass stripped every worker to a minimal
harness.

## Decision

1. **Four worker roles** (profile names follow `model_role`, lowercase —
   Hermes validates `^[a-z0-9_-]{0,63}$`):

   | Profile | Model | Role |
   |---|---|---|
   | `nemotron_implementer` | nemotron-3-ultra-550b:free | implementer |
   | `gemini_implementer` | gemini-3.5-flash-lite | implementer |
   | `deepseek_reviewer` | deepseek-v4-flash-free | reviewer (read-only) |
   | `mimo_researcher` | mimo-v2.5-free | researcher (source-first) |

2. **Implementers are interchangeable** on Tier 1–2 work, split by availability
   not capability (ADR-018 intent kept). The main model (AIko) remains the
   orchestrator, reviewer-gate, and merge gate; boot-path / ring-3 / debugging
   (Tier 3) never leaves the main model.
3. **Reviewer = read-only, never the gate.** Checklist-driven; findings are
   proposals with `file:line` citations; verdicts are evidence, not authority.
   Proven value: the 2026-08-06 docs audit found real drift the maintainer
   missed (root README stale, REPL `version` printed v0.4.0, TaskLog describing
   deleted profiles).
4. **Researcher = source-first.** Briefs quote verbatim with URLs; no memory-
   based summaries; deliverables land in `Documentation/Research/`.
5. **No third implementer at this codebase size** (~21 kernel files, 5
   serialized collision points). The ceiling is file ownership, not budget.
6. **Lean profiles are the contract:** role-minimal toolsets (web lives only in
   the researcher), no personalities, no MCP servers, no skills catalog, no
   cron, memory off — smaller prompts = fewer tokens per call.
7. **Provider governance:** free Zen models may train on data (no secrets in
   cards); gemini traffic flows through the budget-governor proxy
   (`gemini_budget_proxy.py`, 50% of paid limits).

## Consequences

- Cards carry role-appropriate briefs; implementer cards now allow
  commit+push of the task branch (PRs + merges stay with AIko — the gate).
- Reviewer findings feed a fix pass per card, not a blocking mechanism.
- The Team-Expansion-Plan (`Documentation/Design/Team-Expansion-Plan.md`) is
  hereby implemented; remaining matrix options (MiMo-V2.5 etc.) slot into the
  existing roles without redesign.
