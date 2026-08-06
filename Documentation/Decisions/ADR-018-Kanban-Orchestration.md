# ADR-018: Kanban orchestration + three-way division of labor

- **Status:** Accepted (2026-08-06)
- **Supersedes:** the ad-hoc direct-dispatch pattern for multi-step work (delegate_task remains for one-shot slices)
- **Superseded by:** nothing

## Context

Phase 3 proved parallel subagents work (PR lanes, worktrees, branch protection) but
orchestration was chat-driven: AIko hand-wrote every brief, dispatched, reviewed,
merged. With a second worker profile joining, the coordination layer needs to be
explicit and durable — a board the whole team reads, with assignments, dependencies,
and an audit trail. Question: how do the three agents divide work, and what are the
guarantees that keep parallel edits safe?

## Decision

1. **Hermes kanban is the coordination layer.** One card per unit of work; the
   gateway dispatcher spawns the assigned profile; cards carry goal, file set,
   branch discipline, and evidence contract (the old brief, now on the card).
   Dependencies via `kanban link`; failure_limit 2 auto-blocks; stale claims
   reclaimed after 4 h. Full operating guide: Guides/How-to-use-kanban.md.
2. **Division of labor, informed by model research (2026-08-06):**
   - **AIko (deepseek-v4-flash)** — orchestrator + reviewer + merge gate; war-story
     territory stays on the main model (boot path, ring-3, memory management,
     debugging, architecture — the Tier-3 rule is unchanged by the kanban).
   - **Nemotron (nvidia/nemotron-3-ultra-550b-a55b:free)** — SWE-bench 65–70%,
     Terminal-Bench 54%, 1M context; proven on kernel components with contracts
     (Tier 2.5, zero self-inflicted regressions across Phase 3).
   - **Gemini (gemini-3.5-flash-lite)** — Google's fastest 3.5-tier model: 1M
     context / 64K output, 350 tok/s, $0.30/$2.50 per 1M tokens; SWE-bench Pro
     54.2% (≈ GPT-5.4 mini), Terminal-Bench 54%, OSWorld 74% (best-in-class
     agentic computer use), long-context 8-needle 72.2% @128k. Agentic/coding
     capability is comparable to Nemotron's at far lower latency/cost, so the
     two workers are **interchangeable on Tier-1/2 work — split by availability,
     not capability**. Both are read-only on the repo (cards carry branch rules);
     both endpoints log prompts (no secrets in card bodies).
3. **The file-ownership rule is the parallelism guarantee.** Cards declare their
   file set; shared files (kernel.h, repl.c, kmain.c, test.sh, build.sh) are
   serialized; workers never merge — AIko reviews + runs the suite + merges.
4. **CI gains per-branch concurrency control** so three actors pushing don't
   queue stale runs (build.yml `concurrency` group, cancel-in-progress).

## Consequences

**Positive:** the board is the single source of truth (who owns what, what's
blocked, what merged); the audit trail (comments/events) feeds the policy
validation record; workers are swappable; parallel lanes stay safe via file
ownership + the review gate.
**Negative:** board discipline is a new habit (cards must carry the file set or the
safety net has a hole); two-model asymmetry means empirical calibration needed for
Gemini's first kernel tasks (Phase 3-style review fixes expected).
**Neutral:** delegate_task still exists for one-shot slices; the kanban adds
durability — cards survive restarts, unlike process-local children.
