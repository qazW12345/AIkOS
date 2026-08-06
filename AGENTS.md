# AIkOS — Agent Instructions

AIkOS is a from-scratch hobby operating system. This file is the entry point for any agent session working on the project.

## Before touching anything

1. Read `README.md` (root) — the router: current state, topic map, read order.
2. Read `Documentation/TaskLog.md` — the newest entry tells you exactly where work stopped.
3. Check `Documentation/Guides/` before rediscovering anything — hard-won knowledge lives there.
4. Orchestrating workers (Nemotron + Gemini profiles): read `Documentation/Guides/How-to-use-kanban.md` (the board, AIko's lifecycle, file-ownership rules) and `Documentation/Guides/How-to-delegate-to-subagents.md` (tier rules, brief template, privacy rules, model division).
5. Component contracts (ADR-014): every kernel file's header block states `Component / Provides / Depends on / Owns` — read the contract before the code, and cite contracts (not the whole tree) in any brief.

## After finishing work

- Update `Documentation/TaskLog.md` (session handoff entry).
- Update `Documentation/Journal.md` if something noteworthy happened (it's the human-readable story).
- Significant decisions become ADRs in `Documentation/Decisions/` — never rewrite an ADR, supersede it with a new one.

## Golden rules

- Docs are code: they live in git, versioned with the project.
- The task log is the handoff mechanism — its newest entry is the source of truth for "where are we".
- If reality disagrees with a document, change the document (or write an ADR) — never leave silent drift.

## Housekeeping: the archive/ folder

Old/redundant files are NOT deleted on sight — they move to `archive/` (gitignored),
then a weekly review confirms and deletes them:

1. Found something stale (debug logs, CI zips, accidental artifacts)? `git mv` or
   plain `mv` it into `archive/` (keep names unchanged — the review needs to see
   what it is).
2. The weekly `archive-review` cron lists the folder with ages/sizes and flags
   files older than 30 days — that's the inspection trigger.
3. On review: confirmed redundant → delete; might be useful → keep; needed again
   → move back. Note the verdict in the weekly cron reply.
4. Never delete from `archive/` without the review step; never put git-tracked
   files there (they belong in git history, not the archive).
