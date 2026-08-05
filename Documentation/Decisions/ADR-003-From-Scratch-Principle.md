# ADR-003: The from-scratch principle

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing (complements ADR-002)
- **Superseded by:** nothing

## Context

Marcel: *"ideally as much as possible of AIkOS would be created from scratch to make it NOT a copy of windows/linux etc."*

The existing non-goals already exclude POSIX compatibility and "Linux clone" ambitions. This ADR makes the positive principle explicit. One practical constraint shapes it: an OS cannot be written with nothing — building any OS requires a compiler, assembler, linker, and an emulator to test it. Writing those *before* an OS exists is a chicken-and-egg problem (Terry Davis's HolyC compiler only became possible because TempleOS itself already ran).

## Decision

- **The OS itself is from scratch**: bootloader, kernel, drivers, filesystem, GUI, applications — no code borrowed from Linux, BSD, Windows, or any other operating system, and no compatibility layers.
- **The build toolchain is borrowed bootstrap, not compromise**: compiler, assembler, linker, QEMU, git. The toolchain builds the OS; it is not part of the OS. This is the standard hobby-OS interpretation (TempleOS itself was built with existing tools; HolyC came later).
- **Standards are documentation, not code**: implementing from the spec (Intel SDM, ACPI, FAT32, Multiboot, ...) counts as from-scratch. Reading is not copying.
- **Hardware is physics**: ports, MMIO, interrupt controllers are used as they are — from-scratch governs our *code*, not the silicon.
- **Phase 6 (Own Tongue) is the culmination**: when AIkOS runs its own compiler, written in its own language, on itself, the bootstrap loop closes.

## Consequences

**Positive:**
- Distinct identity — by construction, AIkOS cannot be mistaken for a derivative.
- No licensing entanglement; every line is ours to do with as we please.
- The principle is enforceable: each phase's review asks "did we borrow?" — and the answer must be "no" (or an ADR).
- Tilt: the Phase 0 boot-path question now leans **custom bootloader over GRUB** (final call in the Phase 0 design doc).

**Negative / costs:**
- Everything is slower; some wheels genuinely do not need reinventing (e.g., we may still design our own filesystem rather than implement FAT32 — that is a Phase 3 design decision, weighed against compatibility needs like QEMU disk images).
- Risk of ignorance dressed as purity — mitigated by the Guides rule: hard-won knowledge is documented, and "from scratch" means our *code*, not our *understanding*.
- The toolchain exception must stay honest: if we ever need a feature the toolchain lacks, we build the toolchain *feature*, not silently import one.
