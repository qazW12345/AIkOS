# How to delegate to subagents (Nemotron 3 Ultra 550B)

**Status:** Active policy — 2026-08-05
**Scope:** AIkOS tasks delegated via Hermes `delegate_task` (background subagents)
**Model:** NVIDIA Nemotron 3 Ultra 550B-A55B via the OpenRouter free endpoint (`nvidia/nemotron-3-ultra-550b-a55b:free`), wired as `delegation.model` / `delegation.provider: openrouter` in Hermes config; API key via `OPENROUTER_API_KEY` env.

## 1. Capability profile (researched 2026-08-05)

Sources: NVIDIA research page + developer blog (2026-06-04), OpenRouter model page, Artificial Analysis.

- **Architecture:** 550B total / 55B active parameters (MoE); hybrid Transformer-Mamba layers; LatentMoE expert routing; multi-token prediction; NVFP4 quantized checkpoints.
- **Context:** 1,000,000-token input window; 65,536-token max output.
- **Release & license:** 2026-06-04; fully open (weights, data, recipes) under OpenMDW-1.1.
- **Benchmarks (vendor + independent):**
  - Agent productivity (PinchBench): **91%** — ties the best-in-class (Kimi K2.6 91%)
  - Instruction following (IFBench): **82%** — best of the compared set
  - Long-context retrieval (Ruler @1M): **95%** — best of the compared set
  - Coding (Terminal-Bench 2.0): **54%** — below Kimi K2.6 (67%) and GLM 5.1 (64%)
  - Long-horizon planning (EnterpriseOps-Gym): **33%** — below GLM 5.1 (40%)
  - SWE-bench Verified: **65–70.4%** consistently across harnesses — *including Hermes Agent* (NVIDIA explicitly supports Hermes with this model)
  - Artificial Analysis Intelligence Index: 38 (above median)
- **Positioning:** excellent at multi-step agent work, tool-calling discipline, long-context recall, and instruction following; mid-pack at raw coding; weaker at very long planning horizons. Free endpoint = zero token cost.

## 2. What to delegate — reliability tiers for AIkOS

**Tier 1 — delegate freely (read-only / low-risk):**
- Log and test-output analysis: boot milestone chains (`SBM E U F A L C P 1…K`), test.sh result greps, QEMU serial captures
- Spec research with source links (OSDev wiki, Intel SDM behavior, tool docs)
- Documentation drafting from bullet points: ADR summaries, TaskLog/Journal entries, Roadmap table edits, guide updates
- Boilerplate generation following existing patterns: linker scripts, keymaps, test additions, build.sh sections

**Tier 2 — delegate with mandatory diff review:**
- Single-file mechanical codegen with exact specifications
- Isolated refactors (renames, declaration moves, comment fixes)
- Test script additions/extensions

**Tier 3 — never delegate (stay on the main model):**
- Anything touching paging, rings, TSS, syscalls, IDT, memory management — war stories #1–9 territory
- Multi-file architectural changes
- Debugging sessions (require full session history, war-story context, and iterative tool loops)
- Any task whose failure cost exceeds the delegation savings — kernel edits are expensive failures

## 3. Instruction template (briefs must be self-contained)

Subagents have **no memory** of this conversation. Every brief must include:

1. **Goal** — one sentence, unambiguous outcome
2. **Context** — exact paths, current state, expected format (what exists, what's wanted)
3. **House rules** — match existing style, KISS/DRY, no cleverness, no fabricated output, report blockers honestly
4. **Verification** — return verifiable handles (absolute paths, exit codes, grep evidence); never claim success without evidence
5. **Output** — concise structured summary; English unless told otherwise

```text
Goal: <one sentence>
Context: <exact file paths + current state + repo conventions to follow>
Contracts: cite the component contract blocks (ADR-014) of the files you
       touch — Provides / Depends on / Owns. Do not read the whole tree.
Rules: match the existing style; KISS/DRY; do NOT invent output — if something
       fails or is missing, report it plainly.
Verification: return absolute paths / exit codes / grep evidence for anything
       you claim to have done.
Output: concise structured summary.
```

Example (Tier 1):
```text
Goal: Verify the serial log at E:\...\build\user.out shows a full ring-3
      round-trip.
Context: hobby-OS test artifact; expected lines: 'SYSCALL 1 (write)',
      'hello from ring 3', 'user exited', 'back in kernel'.
Rules: read-only; do not modify anything.
Verification: quote the matching lines with their order.
Output: pass/fail per expected line.
```

## 4. Operational rules

- **Privacy — critical:** the OpenRouter free endpoint **logs all usage to NVIDIA** for product improvement (per their terms). Never include credentials, PATs, API keys, inbox addresses, or personal data in delegation briefs. AIkOS source and test output are fine (public hobby code).
- **Verification:** subagent summaries are self-reports, not facts — verify file writes, commits, and HTTP results before trusting (house rule, applies to all delegation).
- **Free tier realities:** expect occasional rate limits and availability blips; retry once, then fall back to the main model.
- **Concurrency:** max 3 parallel subagents (Hermes config).
- **Division of labor:** the main model keeps session context, memory, war stories, and architecture; subagents get isolated, fully-specified slices. This mirrors NVIDIA's own recommended pattern (orchestrator + efficient workers).

## 5. Validation record

- **2026-08-05 — Tier-1 gate PASSED.** Log-analysis benchmark (read `build/user.out`, report milestone chain + key strings): `SBMEUFALCP123456789KAIkOS v0.4.0`; `hello from ring 3` (line 6); `back in kernel` (line 9) — verified accurate by grep. 2 API calls, ~23 s, zero fabrication.
- **2026-08-05 — Tier-2 real-world task PASSED (with review fixes).** Task: implement the `hexdump <addr> <len>` REPL command + tests (commit `d87a2b3`, test.sh v5 **25/25 green**, CI green). 50 API calls, ~15 min. Result quality: **structurally correct on first pass** — spec-compliant output format (verified row: `00200000  55 48 89 e5 bf 23 10 20  00 be 12 00 00 00 b8 01  |UH...#. ........|`), correct error strings, good judgment (checked printf.c for `%02x` support, chose a local hex helper instead), build.sh integration correct. **Two review findings, both fixed by the reviewer:** (a) `ADR-XXX` placeholder left in a header comment; (b) the test piped an 18-byte burst (`hexdump 200000 10\n`) — over the 16550 RX FIFO limit (war story #6) — the brief omitted repo-specific traps. **Lessons → §6.**

- **2026-08-05 — Tier-2 task #2 (command table refactor) PASSED — zero reviewer fixes.** Task: replace repl.c's if/else dispatch with a `{name, usage, handler}` table + generated help + t10 unknown-command test (commit `4a58610`, test.sh v6 **26/26 green**, CI green). 22 API calls, ~9.5 min. The lessons from task #1 were applied: contracts cited *and its own contract block updated* ("command dispatch table"), FIFO-burst rule respected in test input, no placeholders, honest evidence. The hexdump handler even improved on the old code (copies the arg token instead of mutating the line buffer). **Lesson comparison: 50→22 API calls, 2 reviewer fixes → 0.**

## 6. Lessons for delegation briefs

- Briefs must include **repo-specific traps**, not just general context: the FIFO-burst rule (war story #6), CRLF in kernel strings, lowercase hex, the `python`/`python3` quirk. A one-line "read How-to-debug.md war stories #1–9 first" is cheap insurance.
- Placeholder tokens (`XXX`) are a common LLM artifact — grep for them in review.
- Subagent self-reports remain self-reports: the final timing patch it reported never landed on disk. **Always verify repo state with `git status`/`git diff` and run the suite yourself** (house rule).
