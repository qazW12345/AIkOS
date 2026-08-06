# Phase 3 — Memory & Files (design doc)

**Status:** Design — 2026-08-05 (implementation next; v0.5.0 target)
**Companion ADRs:** ADR-015 (AIkFS), ADR-016 (ELF static loading), ADR-017 (buddy heap)

## Context & scope

Phase 2 gave AIkOS a ring-3 world with fixed flat-blob programs. Phase 3 makes it a
*general* world: a real kernel heap, a filesystem, and ELF programs loaded by name
from that filesystem. The Roadmap row: *Allocators, filesystem, ELF loader, first
userland apps* — exit criterion: *boots from disk image; runs /bin apps*.

Three promised threads are picked up: the **buddy allocator** (deferred in ADR-012),
the **filesystem** (first mention in the Roadmap), and the **ELF loader** (replacing
the flat-blob path for new apps, extending ADR-013).

## Goals

1. **Kernel heap** — `kmalloc`/`kfree` via a binary buddy allocator over pmm pages
   (ADR-017), with observability (`heap`, `heaptest` REPL commands).
2. **AIkFS v1** — a custom, from-scratch filesystem (ADR-015): superblock, block
   bitmap, directories, contiguous files. Baked into the disk image by a host-side
   `tools/buildfs.py`; served to the kernel from a RAM copy (initramfs pattern).
3. **ELF loader** — parse ET_EXEC ELF64 files, load `PT_LOAD` segments into the
   user region, run them (ADR-016). First real apps: `/bin/hello.elf`, `/bin/ver.elf`.
4. **REPL surface** — `ls`, `cat <file>`, `fsinfo`, `runelf <path>`, `heap`,
   `heaptest` — all testable over serial.
5. **test.sh v7** — the suite grows to ~31 checks; all 26 existing checks stay green.

## Non-goals (recorded as Phase 3.x+ candidates)

- **Higher-half kernel** (ADR-012 deferred; Phase 3+).
- **Write support in AIkFS** — v1 is read-only (content baked by buildfs.py); a write
  path (block allocation at runtime, cache) is Phase 3.x.
- **ATA/PIO disk driver** — the FS is RAM-backed in v1 (initramfs); real disk I/O is
  Phase 3.x (the disk image already carries the same layout, so the driver drops in).
- **File syscalls** (`open`/`read` for userland) — apps only need `write`/`exit` in v1.
- **Multi-process scheduling** — still one ring-3 process at a time (proc.c machinery).
- **Symbolic links, permissions, timestamps, fragmentation** — none in v1 (files are
  contiguous extents).

## Memory map (QEMU 32 MiB, v3)

| Range | Owner |
|---|---|
| 0x000000–0x09FBFF | low memory — reserved (BIOS, IVT, BDA) |
| 0x05000 / 0x04FFC | E820 buffer / count — reserved (boot.asm → mm.c) |
| 0x09000–0x0B000 | kernel page tables (PML4/PDPT/PD) — reserved |
| 0x0C000–0x0CFFF | pmm bitmap — reserved |
| 0x10000–0x14000 | boot read buffers — reserved |
| 0x100000–0x140000 | kernel (entry-first) — reserved |
| 0x200000–0x400000 | user region (2 MiB, U/S) — ELF segments load here |
| 0x400000–0x44FFFF | **AIkFS ramdisk** (boot copies the FS partition here; 64 sectors) |
| above | buddy-managed free pages (kernel heap via `kmalloc`) |

## Design per subsystem

### Buddy allocator + heap (buddy.c, new — ADR-017)

Binary buddy over pmm pages: free lists per order (0..10), split on alloc, merge on
free. `kmalloc(size)` → order = ceil(log2(size/4096)), returns 4 KiB-aligned memory
(no small-object slab in v1 — allocations are page-rounded; FS buffers and ELF images
fit this fine). `kfree` walks back to the buddy. `heap` prints total/free/allocated
pages + largest free order; `heaptest` runs an alloc/free stress round (fixed pattern,
prints OK) — both testable.

### AIkFS (fs.c, new — ADR-015)

Partition in the disk image starts at LBA 97 (after boot+kernel+user+fault blobs —
existing blobs stay for the t7/t8 regression). Block = 512 B (one sector).

- **Block 0 — superblock:** magic `AIkFS1` (6 bytes), version u8, partition block
  count u32, bitmap start u32, bitmap block count u32, root dir block u32.
- **Block 1 — block bitmap:** one bit per data block.
- **Block 2.. — root directory:** fixed 32-byte entries: `name[16]` (NUL-padded),
  `type` u8 (1=file, 2=dir), `size` u32, `first_block` u32, `block_count` u32 →
  16 entries per block. Files are **contiguous extents** (first_block + block_count).
- **Data blocks** start at block **4** (block 3 is the bin directory itself).
  Bitmap bit ordering: bit i = block i, LSB-first within each byte (buildfs.py
  convention — the kernel fs.c must match).

`tools/buildfs.py` (host, python — precedent: tools/ppm2png.py) bakes a directory of
files into the partition inside `build/disk.img`; build.sh invokes it after the blob
payloads. Boot copies the partition to `0x400000` (ramdisk — same pattern as the user
blob copy-up); `fs_init` parses the superblock from RAM; `fs_ls`, `fs_open(path)`
(component walk, one dir level), `fs_read(file, buf)` (whole-file read into a kmalloc
buffer). REPL: `ls`, `cat <file>` (raw bytes), `fsinfo` (magic/version/blocks/files).

### ELF loader (elf.c, new — ADR-016)

Validate `\x7fELF`, ELF64 LE, `ET_EXEC`, machine x86-64. Walk the program headers;
for each `PT_LOAD`: copy `[p_offset, p_offset+p_filesz)` → `p_vaddr`, zero-fill the
rest of `p_memsz`. Entry = `e_entry`. Constraints: `p_vaddr` must land inside
`[0x200000, 0x400000)` (the mapped user region); no relocations, no `PT_INTERP`,
no `PT_PHDR` handling in v1. `proc.c` gains `proc_run_elf(entry, stack)` reusing the
existing address-space/return machinery; the flat-blob `proc_run`/`proc_run_fault`
stay untouched for regression. build.sh produces the app `.elf` files (clang already
emits them before objcopy); buildfs.py places them in `/bin`.

### Userland apps (user/, new)

- `user/hello.c` → `/bin/hello.elf` — `write` syscall prints `hello from /bin/hello`,
  then `exit`. Linked at 0x200000 (no crt0; the existing freestanding pattern).
- `user/ver.c` → `/bin/ver.elf` — prints `AIkOS v0.5.0`.

### REPL (repl.c — command table, ADR-014)

New commands slot into the existing table: `ls`, `cat <file>`, `fsinfo`,
`runelf <path>`, `heap`, `heaptest`. Each is a file-local handler — the command-table
refactor pays off exactly as designed.

## Test plan (test.sh v7 — 36 checks)

Existing t1–t10 (26 checks) unchanged. New (added per implementation chunk):

- **t11 heap** — `heap` → grep the free-pages line (buddy chunk).
- **t12 heaptest** — `heaptest` → grep `heaptest OK` (buddy split/merge round-trip).
- **t13 fsinfo** — `fsinfo` → grep `AIkFS1` (superblock magic) and the version (FS chunk).
- **t14 ls** — `ls` → grep `bin` and `hello.elf` (FS chunk).
- **t15 runelf** — `runelf bin/hello.elf` → grep `hello from /bin/hello` +
  `back in kernel` (ELF chunk — the exit criterion).

All new test input stays ≤15 bytes per write (war story #6).

**Exit criterion:** test.sh v7 all green locally + CI green → tag **v0.5.0** + release
with the new disk.img (ADR-004).

## Risks & pitfalls

- **Buddy merge bugs** are silent memory corruption — `heaptest` exercises split/merge
  before any real workload trusts the heap.
- **ELF entry convention** — no crt0: `_start` must be the first section; the linker
  script pins it (the existing user linker pattern).
- **p_vaddr collisions** — two apps must not claim overlapping addresses; the linker
  base is fixed at 0x200000 and the loader validates the range.
- **Ramdisk/partition mismatch** — boot copies a fixed 64 sectors; buildfs.py must not
  exceed it (build.sh asserts).
- **FIFO test input** — new REPL commands are typed by tests; keep each piped write
  ≤15 bytes (war story #6).

## Implementation order

1. buddy.c + `heap`/`heaptest` (ADR-017) — heap before anything that allocates.
2. buildfs.py + build.sh baking + fs.c + `fsinfo`/`ls`/`cat` (ADR-015).
3. elf.c + proc_run_elf + apps + `runelf` (ADR-016).
4. test.sh v7; regression sweep; tag v0.5.0 + release.
