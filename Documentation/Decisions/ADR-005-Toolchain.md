# ADR-005: Toolchain — clang/LLVM, NASM, bash build script

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing

## Context

Phase 0 needs a compiler, assembler, linker and binary tools on Windows. ADR-003 permits a borrowed build toolchain (bootstrap — you need a compiler before you can write a compiler). Candidates evaluated:

- **clang/LLVM** (winget): ELF cross-target out of the box (`clang --target=x86_64-elf`); bundles LLD (linker) and llvm-objcopy (flat binary extraction); modern, actively maintained; one admin prompt at install.
- **OSDev x86_64-elf-gcc zip**: the canonical hobby-OS toolchain, zero-install; but third-party prebuilt (trust + version rot, GCC 12–13 era).
- **MSYS2/MinGW gcc**: COFF/PE-native — the wrong object format for kernels; would require fighting the toolchain.

## Decision

- **Compiler/linker/binutils: clang/LLVM** (`winget install LLVM.LLVM`), invoked as `clang --target=x86_64-elf -ffreestanding -nostdlib`, linked with `-fuse-ld=lld`, binaries extracted with `llvm-objcopy -O binary`. Fallback if the install fails: OSDev x86_64-elf-gcc zip.
- **Assembler: NASM** (winget) — the hobby-OS standard for boot code.
- **Build system: plain bash script** (`build.sh`: nasm → clang → ld.lld → disk image). Revisit (Make/CMake) around Phase 2 when the build graph grows.
- **Baseline kernel flags:** `-ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -O2 -Wall -Wextra -std=c11` (details and rationale in the Phase 0 design doc).

## Consequences

**Positive:**
- One modern toolchain serves all phases; ELF-native end to end (objects, link, binary).
- No trust issues from third-party prebuilt binaries; automatic updates via winget.
- `-mno-red-zone` from day one prevents a silent disaster when interrupt handlers arrive in Phase 1.

**Negative / costs:**
- One UAC prompt at install; potential compiler-rt builtin attention in later phases (documented when hit).
- The bash build script won't scale forever — revisit flagged around Phase 2.
