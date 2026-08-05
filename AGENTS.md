# AIkOS — Agent Instructions

AIkOS is a from-scratch hobby operating system. This file is the entry point for any agent session working on the project.

## Before touching anything

1. Read `Documentation/README.md` — the router: current state, topic map, read order.
2. Read `Documentation/TaskLog.md` — the newest entry tells you exactly where work stopped.
3. Check `Documentation/Guides/` before rediscovering anything — hard-won knowledge lives there.
4. Delegating work to subagents (Nemotron 550B): read `Documentation/Guides/How-to-delegate-to-subagents.md` first — tier rules, brief template, privacy rules.
5. Component contracts (ADR-014): every kernel file's header block states `Component / Provides / Depends on / Owns` — read the contract before the code, and cite contracts (not the whole tree) in any brief.

## After finishing work

- Update `Documentation/TaskLog.md` (session handoff entry).
- Update `Documentation/Journal.md` if something noteworthy happened (it's the human-readable story).
- Significant decisions become ADRs in `Documentation/Decisions/` — never rewrite an ADR, supersede it with a new one.

## Golden rules

- Docs are code: they live in git, versioned with the project.
- The task log is the handoff mechanism — its newest entry is the source of truth for "where are we".
- If reality disagrees with a document, change the document (or write an ADR) — never leave silent drift.
