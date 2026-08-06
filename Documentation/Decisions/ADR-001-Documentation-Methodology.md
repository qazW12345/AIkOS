# ADR-001: Adopt a structured documentation methodology for AIkOS

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing

## Context

AIkOS is planned as a multi-year hobby OS project. Two realities shaped this decision:

1. **No single upfront design doc can capture the project.** The full complexity of what's to do will only reveal itself over time. We need documentation that is *clear* (readable by both human and model sessions), *extendable* (new discoveries slot in without making a mess), and *detailed* (the Documentation folder must work as a handoff location — a future session or a future, better model should be able to resume autonomously).
2. **Existing practice is well-established.** Research (2026-08-05) covered: Diátaxis (four documentation kinds: tutorials, how-to, reference, explanation — blurring them causes problems), Architecture Decision Records (append-only decision capture with context/consequences; supersede, never edit), Google's design-doc practice (goals AND non-goals, trade-offs foregrounded, 10–20 pages max, mini docs for incremental work, "amendments with links" over rewrites), docs-as-code (docs in git, reviewed like code, definition-of-done includes docs), and AGENTS.md (agent entry-point convention, progressive disclosure). Counter-evidence noted: context files that are bloated or stale *hurt* agent performance (~20% cost increase in one study) — the entry-point doc must stay lean and current, acting as a router rather than a repository.

## Decision

Adopt the following documentation structure for the AIkOS repository:

```
AIkOS/
├── AGENTS.md                  thin agent entry point (points into Documentation/)
└── Documentation/
    ├── README.md              summary + router: state, topic→file map, read order (kept lean, <~150 lines)
    │                          (2026-08-06: consolidated into the ROOT README.md — single readme)
    ├── Roadmap.md             phases with exit criteria + non-goals
    ├── TaskLog.md             session handoff log, newest first
    ├── Journal.md             human-readable narrative diary (non-technical audience)
    ├── Design/                one mini design doc per phase (written before implementation)
    ├── Decisions/             ADR log, append-only, numbered ADR-NNN-*.md
    └── Guides/                how-to build/run/debug + hard-won environment knowledge
```

Working rules:
- **Docs are code**: git from day one, docs versioned and committed with the project.
- **Decisions → ADRs**: significant decisions get a record (Context / Decision / Consequences, alternatives considered). Never edit an accepted ADR — supersede it. Trivial/reversible choices get no ADR.
- **Session-end ritual (soft)**: update TaskLog before ending a session. Not enforced — flexibility over procedure — but recognized as the habit that makes handoffs work.
- **Guides capture hard-won knowledge**: debugging victories and environment quirks (e.g. the `python3`→Microsoft Store trap on this machine) are recorded so they're never re-derived.
- **Journal is for humans**: plain language, concrete examples, no unexplained jargon — a non-technical intelligent reader can follow the project's story.

## Consequences

**Positive:**
- Future sessions resume in minutes instead of re-discovering state.
- Decisions carry reasoning trails; dead ends and alternatives stay visible.
- Hard-won knowledge is preserved — no re-inventing wheels.
- The methodology itself is amendable: changes to this system become new ADRs.

**Negative / costs:**
- Documentation takes time that could be spent coding (accepted — this project's product is partly the knowledge itself).
- Risk of staleness → mitigated by the session-end ritual and the "fix the document when it disagrees with reality" rule.
- Risk of bloat in the router doc → mitigated by the lean README rule and topic-map indirection.
