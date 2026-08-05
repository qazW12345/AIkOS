# ADR-017: Buddy allocator + kernel heap (kmalloc/kfree)

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing (extends the ADR-012 bitmap allocator; picks up its recorded "buddy allocator deferred to Phase 3" candidate)
- **Superseded by:** nothing (slab/small-object allocator is a Phase 3.x candidate)

## Context

Phase 3 needs kernel-side allocations of *arbitrary size*: FS read buffers, whole-file
ELF images, directory scans. The Phase 2 pmm (ADR-012) hands out raw 4 KiB pages with
a bitmap — page-grained only, no size classes, no free-list efficiency. The design
doc for Phase 3 and ADR-012 both name the binary buddy as the next step; the question
is how much allocator machinery v1 needs.

## Decision

1. **Binary buddy allocator** over pmm pages (`buddy.c`): free lists per order
   (0..10), split-on-alloc, merge-on-free. `kmalloc(size)` rounds up to the next
   order (4 KiB granularity; no slab in v1 — FS buffers and ELF images fit this).
   `kfree(ptr)` walks back to the buddy and merges.
2. **Backed by pmm** — pages come from `pmm_alloc_page`/`pmm_free_page` (ADR-012);
   buddy state lives in kernel .bss (a static structure, no extra pages needed at
   this scale).
3. **Observability** — `heap` (total/free/allocated pages, largest free order) and
   `heaptest` (a deterministic alloc/free stress round printing `heaptest OK`)
   are REPL commands and test.sh checks — merge bugs are silent corruption, so the
   allocator must prove itself before FS/ELF trust it.
4. **Deferred:** small-object slab allocator (when kmalloc traffic justifies it),
   DMA-contiguity guarantees beyond page alignment (Phase 3.x).

## Consequences

**Positive:** bounded worst-case behavior (orders cap at 10), O(log n) split/merge,
deterministic and testable, and it fulfils the ADR-012 promise; the heap unblocks
the whole Phase 3 (FS reads, ELF images).
**Negative:** page-granular allocation wastes up to 4 KiB per small object (accepted
at this scale — no networking stacks yet); buddy metadata is static, so it can't
grow past its configured orders (fine for 32 MiB RAM).
**Neutral:** the pmm bitmap remains the physical allocator; buddy is the *virtual*
heap layer above it — a clean two-layer story for later phases.
