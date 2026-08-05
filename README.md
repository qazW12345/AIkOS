# AIkOS

A from-scratch hobby operating system for x86-64. No Linux, no Windows, no borrowed OS code — everything from the bootloader up is written by us, from zero.

**Status:** documentation phase — the full project methodology is in place. Phase 0 (first boot) not started.

## Why

Nobody else will ever write software for AIkOS, so "complete" means *self-sufficient*: our own kernel, shell, applications, filesystem, GUI — and eventually our own programming language. Not a Windows/Linux replacement; not a toy either. (ADR-002)

## From scratch

The OS itself — bootloader, kernel, drivers, filesystem, GUI, language — is written from zero. The only borrowed pieces are the build tools (compiler, assembler, emulator), and only because you need a compiler before you can write a compiler. Standards are read, not copied: implementing a spec is from-scratch. (ADR-003)

## Roadmap (condensed)

| Phase | Goal |
|---|---|
| 0 | Boot in QEMU — banner on screen |
| 1 | Interrupts, timer, keyboard, kernel REPL |
| 2 | Memory, user mode, syscalls |
| 3 | Filesystem, first programs |
| 4 | GUI |
| 5 | Networking |
| 6 | Own programming language |
| 7 | Real hardware |

Each phase ships as a tagged release when its exit test passes. (ADR-004)

## Documentation

- `Documentation/README.md` — the router: full doc map, current state, read order
- `Documentation/Journal.md` — the human-readable story of the project
- `Documentation/Decisions/` — ADRs: why every decision was made
- `AGENTS.md` — entry point for AI agents working on the project
