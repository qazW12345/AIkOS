# ADR-019: Role-specialized worker team

- **Status:** Accepted (2026-08-06)
- **Supersedes:** the "two workers" clause of ADR-018 (kanban orchestration)
- **Superseded by:** ADR-020 (implementer roster + provider governance)

## Context

ADR-018 established the kanban board with two interchangeable implementer
profiles (nemotron, gemini). After a 4-card live trial on 2026-08-06 (memmap,
contracts validator, research brief, docs audit — 3 landed, 1 provider-quota
block), the team was reorganized by ROLE rather than by capability, and the
lean-profile pass stripped every worker to a minimal harness. Question: what
are the roles, and which guarantees keep the lanes safe?

## Decision

1. **Four worker roles.** Profile names follow `model_role` (lowercase — Hermes
   validates `^[a-z0-9_-]{0,63}$`):
   - **nemotron_implementer** (nemotron-3-ultra-550b-a55b:free) — implementer.
   - **gemini_implementer** (gemini-3.5-flash-lite) — implementer.
   - **deepseek_reviewer** (deepseek-v4-flash-free) — reviewer, read-only.
   - **mimo_researcher** (mimo-v2.5-free) — researcher, source-first.
2. **Implementers are interchangeable** on Tier-1/2 work, split by availability
   not capability (ADR-018 intent kept). The main model (AIko) remains the
   orchestrator, reviewer-gate, and merge gate; boot-path / ring-3 / debugging
   (Tier 3) never leaves the main model.
3. **The reviewer is read-only and never the gate.** Checklist-driven; findings
   are proposals with `file:line` citations; verdicts are evidence, not
   authority. Proven value: the 2026-08-06 docs audit found real drift the
   maintainer missed (root README stale, REPL `version` printed v0.4.0, TaskLog
   describing deleted profiles).
4. **The researcher is source-first.** Briefs quote verbatim with URLs; no
   memory-based summaries; deliverables land in `Documentation/Research/`.
5. **No third implementer at this codebase size** (~21 kernel files, 5
   serialized collision points). The ceiling is file ownership, not budget.
6. **Lean profiles are the contract.** Role-minimal toolsets (web lives only in
   the researcher), no personalities, no MCP servers, no skills catalog, no
   cron, memory off — smaller prompts = fewer tokens per call.
7. **Provider governance.** Free Zen models may train on data (no secrets in
   cards); gemini traffic flows through the budget-governor proxy
   (`gemini_budget_proxy.py`, 50% of paid limits). Implementer cards may
   commit+push their own task branch; PRs and merges stay with AIko.

## Consequences

**Positive:** cards carry role-appropriate briefs; the reviewer's line-cited
findings feed a fix pass per card without blocking the lane; researcher briefs
are self-citing; the Team-Expansion-Plan is implemented, and future model
choices (MiMo-V2.5 etc.) slot into the existing roles without redesign.
**Negative:** role asymmetry means per-role calibration (reviewer/researcher
are new capabilities, not just new names); the lean harness drops safety nets
(personalized SOUL, chat memory) that must not be re-added casually.
**Neutral:** the main model's workload shifts toward review + merge; worker
profiles can be re-pointed to other models without changing the roles.
