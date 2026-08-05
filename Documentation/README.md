# AIkOS Documentation — Summary & Router

> **Status:** Phase 0 SHIPPED (v0.1.0, 2026-08-05) — boots in QEMU, banner on VGA + serial. Phase 1 (interrupts, timer, keyboard) not started.
> **Last updated:** 2026-08-05

## What is AIkOS?

A from-scratch hobby operating system for x86-64, built as a multi-year project with heavy documentation so that any future session (or future model) can pick up where the last one stopped.

## Where we are

- **Phase 0 (Proof of Life): DONE — v0.1.0** (2026-08-05). Boots in QEMU via our own boot sector, reaches 64-bit long mode, prints the banner to VGA + serial, halts. `test.sh` green; release tagged.
- Full phase list with exit criteria: `Roadmap.md`.

## Topic map — read this to find things

| Topic | Where |
|---|---|
| Where work stopped / what to do next | `TaskLog.md` (newest entry) |
| Big picture, phases, exit criteria, non-goals | `Roadmap.md` |
| Human-readable story of the project | `Journal.md` |
| Why a decision was made (reasoning trail) | `Decisions/` (ADRs, numbered, append-only) |
| How to build / run / debug (exact commands) | `Guides/` |
| Phase designs (written before each phase) | `Design/` (one file per phase) |
| Environment quirks (python3 trap, WSL dead, etc.) | `Guides/How-to-build.md` → Known environment quirks |

## Read order for a fresh session

1. This file (you are here).
2. `TaskLog.md` — newest entry = current state.
3. `Roadmap.md` — where the project is going.
4. Follow the topic map for whatever you're about to work on.

## Conventions (the rules we set for ourselves)

- **Docs are code**: everything lives in git, versioned with the project (ADR-001).
- **Design decisions → ADRs** in `Decisions/`, append-only. Never edit an accepted ADR; supersede it.
- **Session end → TaskLog entry.** Soft rule, not enforced — but diligence pays off (ADR-001).
- **TaskLog is for the next session; Journal is for humans.**
- **Guides capture hard-won knowledge** so we never re-invent the wheel.
- If a document disagrees with reality, fix the document — that's part of the work.
