# ADR-006: Boot path — custom boot sector; 64-bit long mode from Phase 0

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing (the boot sector *code* is rewritten in Phase 3 to support our filesystem — that is implementation, not a reversal of this decision)

## Context

ADR-003's from-scratch principle explicitly covers the bootloader. Two candidate paths:

- **Custom boot sector** (NASM, ~200–300 lines for Phase 0): full ownership; with a raw disk layout (no filesystem yet) it is genuinely small; testable in QEMU via `-drive format=raw`.
- **Multiboot2 + GRUB/Limine**: standard and convenient (`-kernel` in QEMU), but borrows the bootloader — in direct tension with ADR-003 — and needs xorriso/mtools on Windows for ISO building.

Target architecture is x86-64, raising a second question: enter long mode in Phase 0, or start 32-bit and transition later.

## Decision

- **Custom boot sector** (NASM): real mode → A20 → protected mode; loads the flat kernel image from a raw disk (int 13h AH=42h LBA, drive 0x80) to 0x100000; jumps to the kernel.
- **Long mode in the kernel's entry assembly** (32-bit entry — the standard pattern every multiboot kernel also uses internally): identity-map the first 1 GiB with 2 MiB pages (PML4/PDPT/PD only), load GDT64, then CR4.PAE → EFER.LME → CR3 → CR0.PG, far jump into 64-bit kmain.
- **No filesystem in Phase 0**: raw sector layout (kernel at fixed sectors, shared constant between boot.asm and build.sh). The boot sector is rewritten in Phase 3 when our filesystem exists.
- **64-bit from day one**: no 32-bit intermediate phase — the target is x86-64 and the paging setup is a one-time, well-documented cost.

## Consequences

**Positive:**
- The boot chain is 100% ours — ADR-003 honored end to end.
- QEMU-testable immediately (`-drive file=disk.img`); Phase 1 starts directly in long mode; kernel C is `x86_64-elf` from the first compile.

**Negative / costs:**
- Real-mode assembly pain (int 13h, A20) lands in Phase 0 — accepted as the from-scratch tuition.
- Filesystem integration deferred to Phase 3 (boot sector rewritten then).
- Real-hardware boot specifics (floppy vs USB vs disk layout) deferred to Phase 7.
