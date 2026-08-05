# ADR-012: Memory architecture — E820 discovery, bitmap allocator, identity-mapped kernel

- **Status:** Accepted (2026-08-05)
- **Supersedes:** Phase 0 design doc's hardcoded memory assumption (implicit)
- **Superseded by:** nothing (buddy allocator and higher-half kernel are recorded Phase 3+ candidates)

## Context

Phase 2 (Two Worlds) needs per-process address spaces and the ability to hand out physical memory. Three questions: how do we *know* how much RAM exists, how do we *allocate* pages, and where does the *kernel* live in the virtual address space once user address spaces exist?

## Decision

1. **E820 memory map** — the boot sector calls `int 15h AX=E820` (it already does int 13h; one more BIOS call), writes entries (24 bytes each) at `0x5000` with a count word at `0x4FFC`; the kernel parses it. Real hardware answer, QEMU-accurate, no recompilation per RAM size.
2. **Bitmap page allocator** — one bit per 4 KiB page; static 4 KiB bitmap in kernel .bss at `0xC000` (covers 128 MiB; QEMU runs 32 MiB). First-fit scan; `pmm_alloc_page`/`pmm_free_page`. Low memory `[0, 1 MiB)` reserved wholesale (BIOS/kernel structures); kernel, page tables, E820 area, and user blobs reserved explicitly. **Buddy allocator deferred** — recorded candidate for Phase 3 when contiguous DMA-style allocations matter.
3. **Identity-mapped kernel** — the kernel stays at `0x100000`, mapped supervisor-only into *every* process's page tables (each process's PD is a copy of the kernel PD with U/S set only on user regions). **Higher-half kernel deferred** — recorded candidate for Phase 3+ when a real multi-process userland arrives; relocating all kernel addressing now would be a rewrite for zero Phase 2 gain.

## Consequences

**Positive:** no pointer relocation anywhere; the allocator is trivially verifiable; per-process page tables are a small delta (per-process PD with U/S on user 2 MiB entries).
**Negative:** kernel occupies low addresses; 128 MiB bitmap ceiling (fine for QEMU; dynamic sizing is Phase 3 work).
