# AIkOS

A from-scratch hobby operating system for x86-64. No Linux, no Windows, no borrowed OS code — everything from the bootloader up is written by us, from zero.

> **Status:** Phase 3 SHIPPED (v0.5.0, 2026-08-06) — buddy heap, AIkFS filesystem + ramdisk, ELF loader, /bin apps (36/36 suite green; 39 with the housekeeping checks). Phase 4 (A Face — framebuffer GUI) next. Team: AIko (orchestrator/reviewer) + Nemotron + Gemini workers via kanban (ADR-018).
> **Last updated:** 2026-08-06

## Why

Nobody else will ever write software for AIkOS, so "complete" means *self-sufficient*: our own kernel, shell, applications, filesystem, GUI — and eventually our own programming language. Not a Windows/Linux replacement; not a toy either. (ADR-002)

## Where we are

- **Phase 0 (Proof of Life): DONE — v0.1.0** (2026-08-05). Boots in QEMU via our own boot sector, reaches 64-bit long mode, prints the banner to VGA + serial, halts. `test.sh` green; release tagged.
- **Phase 1 (The Machine Wakes): DONE — v0.2.0** (2026-08-05). Interrupts, timer ticks, PS/2 keyboard, kernel-mode REPL, panic dumps.
- **Phase 1.5 (The Senses): DONE — v0.3.0** (2026-08-05). kprintf, RTC/time, CPUID, VGA scroll, keyboard REPL, CI green.
- **Phase 2 (Two Worlds): DONE — v0.4.0** (2026-08-05). Paging, user mode, syscalls (int 0x80), ring-3 programs.
- **Phase 3 (Memory & Files): DONE — v0.5.0** (2026-08-06). Buddy heap, AIkFS + ramdisk, ELF loader, /bin apps; test.sh v7 **36/36**; kanban orchestration (ADR-018).
- Full phase list with exit criteria: `Documentation/Roadmap.md`.

## Topic map — read this to find things

| Topic | Where |
|---|---|
| Where work stopped / what to do next | `Documentation/TaskLog.md` (newest entry) |
| Big picture, phases, exit criteria, non-goals | `Documentation/Roadmap.md` |
| Human-readable story of the project | `Documentation/Journal.md` |
| Why a decision was made (reasoning trail) | `Documentation/Decisions/` (ADRs, numbered, append-only) |
| How to build / run / debug (exact commands) | `Documentation/Guides/` |
| Phase designs (written before each phase) | `Documentation/Design/` (one file per phase) |
| Environment quirks (python3 trap, WSL dead, etc.) | `Documentation/Guides/How-to-build.md` → Known environment quirks |

## Read order for a fresh session

1. This file (you are here).
2. `Documentation/TaskLog.md` — newest entry = current state.
3. `Documentation/Roadmap.md` — where the project is going.
4. Follow the topic map for whatever you're about to work on.

## Conventions (the rules we set for ourselves)

- **Docs are code**: everything lives in git, versioned with the project (ADR-001).
- **Design decisions → ADRs** in `Documentation/Decisions/`, append-only. Never edit an accepted ADR; supersede it.
- **Session end → TaskLog entry.** Soft rule, not enforced — but diligence pays off (ADR-001).
- **TaskLog is for the next session; Journal is for humans.**
- **Guides capture hard-won knowledge** so we never re-invent the wheel.
- If a document disagrees with reality, fix the document — that's part of the work.
