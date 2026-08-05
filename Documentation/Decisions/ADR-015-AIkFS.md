# ADR-015: AIkFS — a custom read-only filesystem, RAM-backed in v1

- **Status:** Accepted (2026-08-05)
- **Supersedes:** the Phase 2 implicit "user blobs at fixed LBAs" model (blobs stay for regression; new content moves into a filesystem)
- **Superseded by:** nothing (write support and ATA backing are recorded Phase 3.x candidates)

## Context

Phase 3 needs a filesystem: userland programs should live as *named files* the kernel
loads, not as flat blobs at fixed LBAs. Three candidate formats:

1. **FAT16** — host-interoperable, but the Windows toolchain has no mtools/mformat
   (build.sh would gain a fragile dependency), and FAT's semantics (clusters, 8.3
   names, FAT table) are legacy complexity with no userland consumer yet.
2. **ustar** (tar) — trivially host-writable, but no directory structure worth the
   name, and it's a tape format, not a FS.
3. **Custom minimal FS** — the from-scratch principle (ADR-003), sized exactly to
   what we need: superblock, bitmap, dirs, contiguous files.

## Decision

1. **AIkFS v1** — custom format: 512 B blocks; superblock (magic `AIkFS1`, version,
   counts); block bitmap; root directory with fixed 32-byte entries
   (`name[16]`, type, size, first_block, block_count — 16 entries/block); files as
   **contiguous extents**. One directory level. Full layout: Design/Phase-3.
2. **Host-side writer** — `tools/mkfs.py` (python — precedent: tools/ppm2png.py)
   bakes a file tree into the partition inside `build/disk.img`; build.sh invokes it.
   No new toolchain dependencies.
3. **RAM-backed in v1 (initramfs pattern)** — boot.asm copies the FS partition to
   `0x400000` (exactly how user blobs are already copied); the kernel FS driver reads
   from RAM. The disk image carries the identical layout, so a later ATA driver drops
   in without a format change. No ATA/PIO driver in v1 (recorded Phase 3.x).
4. **Read-only in v1** — content is baked by mkfs.py; runtime allocation/write path
   (and file syscalls for userland) is a Phase 3.x candidate.

## Consequences

**Positive:** from-scratch and testable (fsinfo/ls/cat over serial); zero new host
dependencies; the initramfs pattern is a real-OS technique (Linux does exactly this);
the format is deliberately small enough to reason about whole.
**Negative:** not host-mountable (verification is via the REPL/tests, not Explorer);
contiguous extents mean no fragmentation handling — acceptable at this scale; one
directory level limits organization.
**Neutral:** a format we own can evolve (v2: writes, subdirectories) without
compatibility debt to a legacy standard.
