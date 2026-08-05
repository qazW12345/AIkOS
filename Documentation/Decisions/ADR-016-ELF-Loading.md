# ADR-016: ELF static loading — parse ELF64, load segments, run

- **Status:** Accepted (2026-08-05)
- **Supersedes:** ADR-013's flat-blob program loading (the blobs remain for the t7/t8 regression only; new programs are ELF)
- **Superseded by:** nothing (relocations/ASLR/PIE are Phase 3.x+ candidates)

## Context

Phase 2 loads programs as flat binary blobs at fixed addresses (ADR-013). Phase 3
filesystem delivery needs a real program format: named files in `/bin` that the
kernel loads and runs. clang already emits ELF64 objects for user programs (build.sh
objcopies them flat) — the loader can consume the ELF directly. The format question:
how much ELF machinery does v1 need?

## Decision

1. **Static ET_EXEC loading.** `elf.c` validates magic (`\x7fELF`), ELF64 LE, type
   `ET_EXEC`, machine x86-64; walks the program headers; for each `PT_LOAD` copies
   `p_filesz` bytes from `p_offset` to `p_vaddr` and zero-fills to `p_memsz`;
   entry = `e_entry`. No relocations, no `PT_INTERP`, no `PT_PHDR`, no symbol table
   in v1.
2. **Load region constraint.** `p_vaddr` must land in `[0x200000, 0x400000)` (the
   already-mapped 2 MiB U/S user region) — the loader validates and refuses otherwise
   (no new virtual-memory machinery in v1; vm expansion stays a Phase 3.x candidate).
3. **Reuse the proc machinery.** `proc_run_elf(entry, stack)` drives the existing
   per-process page tables, resume capture, and user_return trampoline (ADR-013);
   the flat-blob `proc_run`/`proc_run_fault` are untouched and keep the t7/t8
   regression green.
4. **No crt0.** User programs keep the freestanding pattern: `_start` first, the
   user linker script pins the base at 0x200000, syscalls via int 0x80.

## Consequences

**Positive:** real program format with headers we can validate; segment zero-fill
gives proper BSS for free; build.sh stops objcopying user programs flat; the path is
open to more ELF features later (shared libs, PIE) without changing the loader shape.
**Negative:** v1 refuses non-ET_EXEC (no PIE); fixed 2 MiB region bounds program
size; no symbols means no kernel-side debugging names for userland (hexdump/objdump
still work offline).
**Neutral:** the format work is one-time; the loader is ~150 lines of careful parsing.
