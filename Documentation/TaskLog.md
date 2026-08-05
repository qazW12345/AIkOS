# AIkOS Task Log — Session Handoff

The source of truth for "where are we". The newest entry describes the current state; a fresh session reads it first.

**Format:** date — what was done / what failed / what's next / build state / open questions.
**Rule:** newest entry on top. Write your entry before ending a session (soft rule — life happens, but diligence pays off).

---

## 2026-08-05 — Phase 1 designed (The Machine Wakes)

**Done:**
- Decisions accepted (all recommendations): **ADR-007** (interrupt architecture: PIC 8259A + PIT 100 Hz; APIC/HPET deferred), **ADR-008** (input: scancode set 1, serial RX polling, serial-only REPL + `panic` command; keymap & serial IRQ deferred), **ADR-009** (exception policy: panic-and-halt with register dump; revisit at Phase 2).
- **Design doc**: `Design/Phase-1-The-Machine-Wakes.md` — IDT/stub architecture, PIC/PIT/keyboard/serial details, `-mgeneral-regs-only`, memory layout, test plan, pitfalls.
- **Roadmap**: Phase 1 exit criterion sharpened into a scripted test (REPL + ticks + keyboard via sendkey + panic dump + Phase 0 regression → v0.2.0).

**Next:**
- Implement: `interrupt.asm`, `idt.c`, `pic.c`, `pit.c`, `keyboard.c`, `repl.c`; test.sh v2.
- On green: tag v0.2.0 + release (ADR-004).

**Build state:** v0.1.0 shipped; Phase 1 designed, not implemented.

**Open questions:** none blocking — implementation follows the design doc.

---

## 2026-08-05 — 🎉 PHASE 0 COMPLETE — v0.1.0 boots!

**Done:**
- **`test.sh` GREEN**: full boot chain verified — `SBMALCP123456789KAIkOS v0.1.0` + `Phase 0: long mode reached, halting.` on serial; VGA banner visually confirmed via screendump.
- Bugs slain (all in Guides/How-to-debug.md as war stories #1–5):
  1. 16-bit serial routine called from 32-bit PM (encoding eats next byte)
  2. QEMU 11 SeaBIOS hangs on AH=42h reads above 1MB → low-buffer + self-copy
  3. PD fill started at base `0x200000` instead of 0 → identity map shifted 2MiB → the "zero walk"
  4. UART is port-mapped I/O — C memory stores to 0x3F8 are silent → inline asm `outb`/`inb`
  5. SysV ABI: char arg in DIL, not AL (kputc)
- **Debug methodology proven**: serial milestone chars (one per boot stage) + `-d in_asm,cpu` traces + raw-byte serial dumps of buffers beat monitor forensics.
- `tools/ppm2png.py` added (screendump is PPM; Windows can't open it).
- `Guides/How-to-version-control.md` created (git rollback workflow, Q2 from Marcel).
- Tagged **v0.1.0**, GitHub release with disk.img artifact (ADR-004).

**Next:**
- Phase 1 (The Machine Wakes): interrupts (IDT), PIT timer, PS/2 keyboard, kernel REPL over serial.
- Phase 1 design doc first (per methodology).

**Build state:** v0.1.0 — boots, prints banner, halts. 881-byte kernel.

**Open questions:**
- None blocking. Phase 1 design decisions (IDT layout, PIC vs APIC, timer choice) land in the Phase 1 design doc.

---

## 2026-08-05 — Phase 0 decisions made; toolchain installed & verified

**Done:**
- **ADR-005** (toolchain): clang/LLVM + NASM + bash build script — all recommendations accepted by Marcel.
- **ADR-006** (boot path): custom boot sector; long mode in kernel entry; 64-bit from day one.
- **Design doc written and accepted**: `Design/Phase-0-Proof-of-Life.md` — boot sequence, memory map, disk layout, serial/VGA strategy, acceptance test, risks.
- **Roadmap**: Phase 0 exit criterion refined (test.sh green + screendump + tagged v0.1.0).
- **Toolchain installed & smoke-verified**: NASM 3.02 (per-user, `AppData\Local\bin\NASM`), clang 22.1.8 + lld + llvm-objcopy (`Program Files\LLVM\bin`), QEMU 11.0.50 (`Program Files\qemu`). Freestanding `x86_64-elf` compile → ELF64 object ✅; `nasm -f elf64` ✅; QEMU launches ✅.
- Install paths recorded in `Guides/How-to-build.md` (PATH caveat: resolve absolute paths in scripts).

**Next:**
- Write `build.sh` + `test.sh` + sources: `src/boot/boot.asm`, `src/kernel/entry.asm`, `src/kernel/kmain.c`, `serial.c`, `vga.c`, `linker.ld`.
- First boot attempt in QEMU — serial log is the lifeline.
- On green: tag v0.1.0 + GitHub release with disk.img (ADR-004).

**Build state:** documentation + verified toolchain; zero kernel code yet.

**Open questions:** none blocking — implementation follows the design doc.

---

## 2026-08-05 — GitHub workflow decided; README added; from-scratch principle

**Done:**
- **ADR-003 (from-scratch principle)**: the OS itself is written from zero — bootloader, kernel, drivers, filesystem, GUI, language. Build toolchain is borrowed bootstrap only. Standards count as documentation, not code. Phase 6 (own compiler) is the culmination. Tilts Phase 0 toward a custom bootloader.
- **ADR-004 (GitHub workflow)**: Releases per phase exit (tag + artifacts, phase not done until tagged); no Issues/PRs for now (TaskLog/Roadmap are the tracker — no dual source of truth); CI deferred until Phase 0 stabilizes; no Wiki.
- **Root README.md** created — short human overview, GitHub-ready, structured like a normal public project.
- Roadmap gained a Principles section; journal entry 4.

**Next:**
- Phase 0 toolchain: NASM + QEMU via winget, compiler TBD.
- Phase 0 design doc (boot path decision: custom bootloader favored by ADR-003).

**Build state:** docs only. 4 commits pushed (will be 5 after this entry).

**Open questions:**
- Compiler: clang/LLVM vs MSYS2 gcc vs OSDev cross-toolchain zip.
- Boot path: custom bootloader (leaning, per ADR-003) vs Multiboot2/GRUB.

---

## 2026-08-05 — GitHub backup live

**Done:**
- Private repo created: **https://github.com/qazW12345/AIkOS** (name: AIkOS, private).
- Remote `origin` configured; token stored in Git Credential Manager (encrypted, never in docs/memory).
- All 3 commits pushed; `git ls-remote` verified silent auth — no prompts on future pushes.
- Path taken: `gh` CLI device flow failed twice in this terminal (browser confirmed, CLI never received token — PTY/polling quirk). Worked around with API + `git credential approve`. gh CLI is installed but NOT authenticated — irrelevant for now; revisit if we need gh features.

**Next:**
- Phase 0 toolchain: NASM + QEMU via winget, compiler TBD.
- Write `Design/Phase-0-Proof-of-Life.md`, build first bootable kernel.

**Build state:** docs only. 3 commits, pushed to origin/main (HEAD 6bbcf5b).

**Open questions:**
- Compiler: clang/LLVM vs MSYS2 gcc vs OSDev cross-toolchain zip.
- Boot path: Multiboot2/GRUB vs custom bootloader.
- Commit identity currently `AIko <aiko@aikos.local>` — swap to Marcel's name if desired (one command).

---

## 2026-08-05 — Project intent refined; GitHub backup next

**Done:**
- Marcel clarified project intent → **ADR-002**: "not a daily-driver OS" is a consequence of the no-ecosystem reality, not a cap on ambition. Completeness is the horizon; "complete" = self-sufficient (own shell, apps, tooling, compiler).
- Roadmap non-goal updated to match; Journal entry 2 written.

**Next:**
- GitHub backup setup: visibility decision (public vs private) pending Marcel; auth via `gh` CLI device flow (needs one browser step from him) or personal access token.
- Push repo to GitHub as the first off-machine backup.
- Then Phase 0 toolchain (NASM + QEMU + compiler).

**Build state:** docs only. Three commits after this entry.

**Open questions:**
- GitHub repo visibility: public vs private (both free).
- Auth method: `gh` CLI login vs PAT.
- Remote name / repo name: `AIkOS` assumed.

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
