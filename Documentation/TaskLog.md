# AIkOS Task Log — Session Handoff

The source of truth for "where are we". The newest entry describes the current state; a fresh session reads it first.

**Format:** date — what was done / what failed / what's next / build state / open questions.
**Rule:** newest entry on top. Write your entry before ending a session (soft rule — life happens, but diligence pays off).

---

## 2026-08-05 — Documentation methodology established

**Done:**
- Researched large-project documentation methodologies: Diátaxis (4 doc types), ADRs (append-only decision records), Google design docs (goals/non-goals, mini docs per phase), docs-as-code (git-versioned docs), AGENTS.md (agent entry-point convention).
- Agreed the structure with Marcel: README router, Roadmap, TaskLog, Journal, Design/ (per-phase), Decisions/ (ADR log), Guides/ (hard-won knowledge), AGENTS.md at repo root.
- Created the full documentation skeleton; git repository initialized (first commit).
- Wrote ADR-001 (this documentation methodology itself) and Journal entry 1.

**Failed:** nothing — no code exists yet to fail.

**Next:**
- Decide Phase 0 toolchain: NASM + QEMU via winget, compiler TBD (see open questions).
- Install toolchain, verify with a hello-world build.
- Write `Design/Phase-0-Proof-of-Life.md`.
- Build Phase 0: bootable kernel in QEMU.

**Build state:** no code exists yet. Documentation skeleton committed.

**Open questions:**
- Compiler: clang/LLVM (cross-compiles out of the box) vs MSYS2 gcc vs OSDev cross-toolchain zip?
- Boot path: Multiboot2 via GRUB (well-trodden, more tooling) vs custom bootloader (more learning, more pain)?
- WSL is unusable (no distro, virtualization disabled in firmware) — confirmed dead end, do not revisit without a BIOS change.
