# Phase 0 — Proof of Life (design doc)

**Status:** Accepted — design for implementation (2026-08-05)
**References:** ADR-001 (methodology), ADR-003 (from-scratch), ADR-004 (releases), ADR-005 (toolchain), ADR-006 (boot path); Roadmap Phase 0

## Context & scope

First breath of AIkOS: boot in QEMU from a raw disk image via our own boot sector, reach 64-bit long mode, print a banner to the screen and the serial port, halt cleanly. This phase exists to prove the entire chain — toolchain, build, boot, test harness — that every later phase builds on. Nothing fancy: no interrupts, no memory management beyond the identity map, no user mode.

## Goals

1. Custom boot sector loads the kernel from a raw disk image in QEMU.
2. Kernel enters 64-bit long mode (identity-mapped).
3. Banner `AIkOS v0.1.0` printed to VGA text (0xB8000) **and** COM1 serial (115200 8N1).
4. Kernel halts cleanly.
5. `test.sh` automates acceptance: headless QEMU, serial captured, banner grepped.

## Non-goals (deliberately deferred)

- Interrupts/IDT, timer, keyboard → Phase 1 (the halt loop needs none of them)
- Memory allocator, heap, paging beyond the identity map → Phase 2
- Filesystem → Phase 3 (raw sector layout now; boot sector rewritten then)
- ELF loader → Phase 3 (kernel ships as a flat binary)
- Real hardware boot → Phase 7

## Toolchain (ADR-005)

- `clang --target=x86_64-elf -ffreestanding -nostdlib`, link with `-fuse-ld=lld`
- NASM for boot sector + entry assembly
- `llvm-objcopy -O binary` for the flat kernel image
- Kernel flags: `-ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -O2 -Wall -Wextra -std=c11`
  - `-mno-red-zone` from day one: interrupt handlers (Phase 1) must not use the red zone; keeping it off avoids a silent disaster later.
- Build: `build.sh` (bash) — nasm → clang → ld.lld → disk image (image assembled with python to avoid MSYS dd quirks)

## Boot sequence (ADR-006)

1. BIOS loads the boot sector (512 bytes) at 0x7C00.
2. Boot sector (`src/boot/boot.asm`):
   - set up stack, save drive number (DL)
   - load kernel.bin via int 13h AH=42h (LBA, drive 0x80) to 0x100000 — fixed sector count for Phase 0 (kernel is tiny; build pads to K sectors)
   - enable A20 (fast A20, port 0x92)
   - load GDT (32-bit), set CR0.PE, far jump into protected mode
3. Kernel entry (`src/kernel/entry.asm`, 32-bit):
   - build identity map for the first 1 GiB using 2 MiB pages: PML4 (1 entry) → PDPT (1 entry) → PD (512 entries) — three tables at 0x9000..0xC000 (above the 0x7C00 boot-sector region, below 1 MiB)
   - load GDT64 (code 0x08 / data 0x10)
   - CR4.PAE → EFER.LME → CR3=PML4 → CR0.PG → far jump into 64-bit
4. `kmain` (C, x86_64-elf):
   - `serial_init` (COM1 0x3F8, 115200 8N1), `vga_clear` + banner, serial banner
   - `cli; hlt` loop

## Memory map (Phase 0)

| Range | Use |
|---|---|
| 0x7C00 | boot sector |
| 0x9000–0xC000 | page tables (PML4 / PDPT / PD, 3 × 4 KiB) |
| 0x100000 | kernel (flat binary load address) |
| kernel .bss | stack (16 KiB) — RSP set in entry.asm |

## Disk image layout (raw, no FS)

- Sector 0: boot sector (512 B)
- Sectors 1..K: kernel.bin (padded to a fixed K sectors — `KERNEL_SECTORS` constant shared by boot.asm and build.sh, with a build-time size assertion)

## Output & debugging

- **Serial is the lifeline**: COM1 → QEMU `-serial file:build/serial.log` (headless runs) or `-serial stdio` (interactive).
- VGA text mode: 80×25, white-on-black banner at 0xB8000.
- QEMU dev flags: `-display none -no-reboot`; monitor (`-monitor stdio`) for `info registers` / `info mem` on faults; `screendump` for visual VGA proof.
- Triple-fault drill: last serial line before the fault + monitor registers.

## Acceptance test (test.sh)

1. `build.sh` → `build/disk.img`
2. `qemu-system-x86_64 -drive file=build/disk.img,format=raw -serial file:build/serial.log -display none -no-reboot` (30 s timeout)
3. grep for `AIkOS v0.1.0` in `build/serial.log` → exit 0/1
4. Visual proof: screendump PNG shows the banner on VGA

**Exit criterion (Roadmap):** test.sh green; screendump verified; tag `v0.1.0` + GitHub Release (ADR-004).

## Risks & pitfalls

- **A20 off** → memory wrap: enable fast A20 early; verify by reading back port 0x92 bit 1.
- **Long-mode order matters**: CR4.PAE → EFER.LME → CR3 → CR0.PG; wrong order = #GP/#UD. Serial debug lines before each step.
- **GDT mistakes** → triple fault: keep GDTs in known memory; test with serial between transitions.
- **Link order**: entry.asm must be first in the link so the flat binary starts with entry code.
- **objcopy alignment**: the linker script must produce a binary whose entry point is at offset 0.
- **Fixed sector count**: boot.asm and build.sh share `KERNEL_SECTORS`; assert kernel.bin ≤ K×512 bytes at build time.
- **MSYS dd quirks**: image assembly via python (`python`, never `python3` — see Guides/How-to-build.md).

## Deliverables

```
src/boot/boot.asm     src/kernel/entry.asm   src/kernel/kmain.c
src/kernel/serial.c   src/kernel/vga.c       linker.ld
build.sh              test.sh                build/ (gitignored)
```

Tag `v0.1.0`; GitHub release carrying `disk.img` as the artifact (ADR-004).
