# Team Expansion Plan — reviewer + research roles (PROPOSAL, 2026-08-06)

> **Status:** proposal — not decided. Marcel is weighing candidate models
> (Mistral Medium, Xiaomi MiMo-V2.5, a second DeepSeek V4 Flash) before
> choosing. This document fixes the *architecture* so the *model choices*
> can slot in without redesign. When models are chosen, this becomes ADR-019.
> Companion: Guides/How-to-use-kanban.md (the board), ADR-018 (current team).

## 1. The case, in one paragraph

Phase 3 produced 8 real defects across 4 implementer tasks — every one caught by
**review against the spec**, none by the writer's own tests (the buddy coalesce
leak, alignment fragmentation, three ELF hardening gaps, dead code, casts).
Verification is cheaper than generation, so a dedicated reviewer lane needs less
model than an implementer lane; and a fresh-context reviewer has no anchoring
bias — it reads what is there, not what the writer intended. A research lane
keeps AIko's context clean and builds a citation-backed knowledge base. Both
new roles are **read-only**: zero file-collision risk, no merge-gate pressure —
they *relieve* the real bottleneck (AIko's review/merge capacity) instead of
pushing on it. **Explicit non-goal: a third implementer** — ~21 kernel files
with five shared collision points already serialize implementer lanes.

## 2. Role architecture

```
                ┌──────────────────────────────┐
                │   AIko (the gate)            │  deepseek-v4-flash
                │   orchestrate · review ·     │  final verdict, fixes,
                │   suite · merge · war stories│  merge — unchanged
                └──────────────┬───────────────┘
        ┌─────────────┬────────┴────────┬──────────────┐
        ▼             ▼                 ▼              ▼
   implementer    implementer      reviewer ★       research ★
   nemotron       gemini           (NEW, read-only)  (NEW, read-only)
   kernel comps   kernel comps     checklist review  source-first briefs
   (proven)       (proven)         of implementer    → Documentation/Research/
                                   cards, evidence   + Hermes/Obsidian notes
                                   per finding       (grounded-citations)
```

- **Reviewer lane:** implementer card done → reviewer card links to it
  (`kanban link`) → reviewer runs the checklist, comments line-cited findings →
  AIko verifies, fixes, merges. Reviewer NEVER merges and NEVER edits code.
- **Research lane:** standalone cards; output is a cited brief, not code.
- **Swarm note:** `kanban swarm` already encodes workers → verifier →
  synthesizer; the reviewer lane is that verifier made first-class.

## 3. Candidate model matrix (researched 2026-08-06, sources linked)

| Model | Context / output | Coding & agentic | Cost | Fit |
|---|---|---|---|---|
| **Gemini 3.5 Flash-Lite** (worker) | 1M / 64K | SWE-bench Pro 54.2 · TB 54 · OSWorld 74 | $0.30/$2.50 | implementer + **reviewer** (fast/cheap) + **research** (1M ctx) |
| **Nemotron 3 Ultra 550B** (worker) | 1M / 65K | SWE-bench 65–70.4 · TB 54 | free (rate-limited) | implementer |
| **MiMo-V2.5** (Xiaomi) ★candidate | **1M** / omnimodal | **SWE-bench Verified 78.6 (thinking)** — frontier-adjacent | half of frontier cost; open-weights; on OpenRouter | strongest *implementer* candidate; also research (1M + images/video) |
| **Mistral Medium** (medium-latest) ★candidate | 131–262K | "≥90% of Claude Sonnet 3.7" (Medium 3, 2025) | $0.4–1.65 in / $2–8.25 out | reviewer (EU-friendly, strong reasoning, mid cost) |
| **DeepSeek V4 Flash #2** ★candidate | same as main | empirically the strongest in this stack (found every Phase 3 bug in review) | per-token, cheap | **reviewer gold standard**; Tier-2.5 implementer relief |
| **Qwen3-4B** (local) | 262K | small-model tier | $0, private, ~29 tok/s | budget reviewer for non-critical cards; privacy-sensitive review |

Sources: gemini-3.5-flash-lite model card (deepmind.google), nemotron via OpenRouter,
MiMo-V2.5 (mimo.mi.com; openrouter.ai/xiaomi/mimo-v2.5; deepinfra), Mistral Medium 3
(mistral.ai/news/mistral-medium-3; requesty.ai pricing). Pin exact aliases at onboarding.

## 4. The review-card contract (the product — what makes a weak model sufficient)

Every review card MUST carry this checklist; findings MUST cite file:line and
explain the failure mode; "no issues" is only accepted with the checklist shown:

1. **Contract conformance** (ADR-014): Provides/Depends/Owns lines match the code.
2. **Spec invariants** from the linked design doc/ADR (e.g. heap accounting closed;
   FS offsets exact; ELF bounds overflow-safe — `p_memsz > END - p_vaddr`, not
   `vaddr + memsz`).
3. **War-story traps**: FIFO input ≤15 B bursts; CRLF strings; lowercase hex;
   python-not-python3; exec-bit on new scripts; no placeholders (XXX/TODO).
4. **Build discipline**: warning-free; no dead code; no debug leftovers.
5. **Test evidence**: real RESULT line; the suite was run, not assumed.

## 5. The research-card contract

- **Source-first**: fetch real pages, quote them, link them (grounded-citations
  skill). No summaries of memory — every claim traces to a URL.
- Output: cited brief → `Documentation/Research/<topic>.md` → feeds design docs;
  Hermes-level topics → Obsidian.
- AIko spot-checks: URLs resolve, quotes match (cheap verification — the
  delegation lesson: self-reports are not facts).

## 6. Wiring (when models are chosen)

1. Profiles: `reviewer` + `research` (model per decision; `hermes profile create`).
2. Kanban conventions: card-type prefixes (`[REVIEW]`, `[RESEARCH]`), review cards
   `link` their implementer card; READ-ONLY role note on the board.
3. ADR-019 (supersedes the "two workers" clause of ADR-018) with the chosen matrix.
4. No CI changes: read-only lanes can't collide; docs PRs already paths-filtered.
5. Calibration: first REAL review card + first REAL research card get the same
   validation treatment as every other onboarding (evidence, review, record).

## 7. Decision points for Marcel

- **Reviewer model**: DeepSeek V4 Flash #2 (gold standard) · Gemini (cheap/fast) ·
  Mistral Medium (EU/mid) · local Qwen (free/private) — or a tiered split
  (Qwen for routine, DeepSeek for kernel-critical).
- **Research model**: Gemini (1M cheap) · MiMo-V2.5 (1M omnimodal).
- **MiMo-V2.5 as implementer**: the one candidate that changes the non-goal —
  frontier-adjacent coding at half cost; adopt only when the file-ownership
  ceiling lifts (codebase grows) or as a third lane with strict serialization.
