# How AIko uses the Kanban (orchestration guide)

> **Status:** current (2026-08-06). The coordination layer for the four-person team:
> Marcel (owner), AIko (main agent — orchestrator + reviewer), Nemotron worker
> (profile `nemotron`), Gemini worker (profile `gemini`).

## What the kanban is

Hermes's built-in durable work queue (SQLite board, `hermes kanban`). The gateway
runs a **dispatcher** (`kanban.dispatch_in_gateway: true`) that claims ready cards
every 60 s and spawns the **assigned profile** as a worker session. Workers get the
`kanban_*` toolset (show/complete/block/comment/heartbeat) and a pinned board via
`HERMES_KANBAN_TASK`/`HERMES_KANBAN_BOARD`. Profiles: `nemotron`
(nvidia/nemotron-3-ultra-550b-a55b:free via OpenRouter) and `gemini`
(gemini-3.5-flash-lite via Google AI Studio). The generic `delegate_task` fallback
runs gemma-4-31b-it (Google) — prefer the kanban for anything multi-step.

## The lifecycle AIko runs

```
1. CREATE     user request → cards (kanban create). One unit of work per card.
             auto_decompose: true splits big cards (3 per tick).
2. ASSIGN     kanban assign <id> <nemotron|gemini> — or leave unassigned for
             AIko's own hands (boot path, ring-3, review, docs).
3. DISPATCH   the gateway dispatcher claims + spawns the worker automatically.
4. WORK       the worker edits in ITS workspace (see card body for branch
             instructions), runs its verification, kanban complete <id>.
5. REVIEW     AIko: kanban show <id> → diff review → own suite run → merge PR
             → kanban comment <id> with the result → (card already done).
6. DEPEND     parent→child via kanban link (chunk B depends on A → B links A).
```

## The kanban ↔ git mapping (the four-person safety net)

| Kanban state | Git state | Who |
|---|---|---|
| todo / ready | nothing | AIko |
| claimed (in progress) | worktree `AIkOS.<task>` on branch `feat/<task>` | worker |
| blocked | nothing (card explains why) | AIko |
| done | PR merged to protected `main` | AIko (gate) |

**Rules that make 3 actors safe on one repo (from the Phase 3 war diary):**

1. **Every card that touches the repo declares its FILE SET in the body** — the
   parallelism rule. Two cards owning the same file = a merge conflict; cards
   touching `src/kernel/kernel.h`, `src/kernel/repl.c`, `src/kernel/kmain.c`,
   `test.sh`, `build.sh` are **serialized** (only one in flight).
2. **Every card body carries the branch + worktree discipline**: "work in
   `E:\Hermes_Agent\projects\AIkOS.<task>` (git worktree), branch `feat/<task>`,
   commit + push to YOUR branch only, never touch main (protected)."
3. **Workers never merge.** AIko reviews, runs the suite, merges the PR, deletes
   the branch — the non-negotiable gate.
4. **Card bodies carry the evidence contract**: real RESULT line from ./test.sh,
   no fabricated output, no placeholders (XXX/TODO), report surprises.
5. **Dependencies are explicit** (`kanban link`) — a card that needs another's
   merged output links it, so the dispatcher/assigner never starts it early.

## Conventions

- **Card title:** `[AIkOS] <verb phrase>` — e.g. `[AIkOS] chunk 4: A Face framebuffer init`.
- **Card body = the old delegation brief** (goal, contracts to cite, repo traps:
  python-not-python3, CRLF, FIFO ≤15 B input, lowercase hex, no secrets — the
  Google/NVIDIA endpoints both log).
- **Comments** carry the review verdict + evidence (like the policy §5 records).
- **Failure handling:** a worker failing twice auto-blocks the card
  (`kanban.failure_limit: 2`); stale claims are reclaimed after 4 h
  (`dispatch_stale_timeout_seconds: 14400`). AIko unblocks after diagnosing.
- **Swarm** (`kanban swarm`) is the parallel fan-out mode (workers → verifier →
  synthesizer) — use for genuinely parallel independent edges, not for
  dependent kernel chunks.

## AIko's daily commands

```bash
hermes kanban ls                     # the board
hermes kanban show <id>              # card + comments + events
hermes kanban create --assignee nemotron --title "..." --body "..." # (or --assignee gemini)
hermes kanban link <child> <parent>  # dependency
hermes kanban comment <id> "..."     # review verdict / context
hermes kanban block|unblock <id>     # problems / resolved
hermes kanban complete <id>          # only after the PR merged
hermes kanban tail <id>              # live worker log
```

## What AIko keeps for itself (never cards)

Boot path / entry.asm surgery, ring-3 machinery (proc.c), memory management
design, debugging sessions, the merge gate, suite runs, ADR/design decisions.
War-story territory stays on the main model — the kanban doesn't change the
Tier-3 rule, it just formalizes how the other tiers are queued.

## Verification cards (the review loop, 2026-08-06)

The board has NO native `review` stage — the status enum accepts `review` but
the dispatcher/dashboard treat it as a forward-compat value (canonical statuses:
triage, todo, ready, running, blocked, done, archived). The native ways to run
verification:

1. **Implementer hand-back:** on finishing, the worker calls
   `kanban_block(reason="review-required: …", kind="needs_input")` — the P5
   human-in-the-loop pattern (surfaces to AIko). This is the correct protocol;
   do NOT have workers mark cards done before review.
2. **Verification card (P2 pipeline):** create `[REVIEW] <subject>` assigned to
   `deepseek_reviewer` with `--parent <implementer-card>`. Gated: it promotes to
   `ready` when the implementer card reaches `done` — right for POST-merge
   verification.
3. **Pre-merge review:** when the review must run BEFORE the merge (syscall
   series rule — bugs must not compound), do NOT gate the review card on the
   implementer card: `hermes kanban unlink <implementer> <review>` (unlinking a
   parentless ready card keeps it ready — the dispatcher re-gates a forced
   promote, so `promote --force` does NOT survive the tick). Keep the reference
   in the card title instead (`[REVIEW] … (t_<id>)`).
4. **Reviewer contract:** checklist-driven (ABI/spec/bounds/test-evidence/
   contracts), findings cite file:line, verdict is a proposal — never the gate.
   AIko verifies findings + runs the suite + merges.
