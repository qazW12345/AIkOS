# How to debug AIkOS

> **Status:** current — the Phase 0 debug toolkit (2026-08-05). Serial milestones, QEMU monitor, and war stories.

## The debug loop (proven in Phase 0)

1. Build: `./build.sh`
2. Boot headless with serial to file: `qemu-system-x86_64 -drive file=build/disk.img,format=raw -serial file:build/serial.log -display none -no-reboot -m 32M`
3. Read the serial log — the **milestone characters** tell you exactly how far the boot chain got:

```
S  boot sector entered            B  disk read OK          M  kernel copied to 1MB
A  A20 on                        L  GDT loaded            C  CR0.PE set
P  protected mode               1  kernel entry           2  page tables built
3  CR3 set                      4  CR4.PAE set           5  EFER.LME set
6  CR0.PG set (paging on)       7  GDT64 loaded          8  long mode reached
9  C stack set                  K  calling kmain
```

4. If it dies early: check the milestones; if the boot sector itself fails, add `-monitor stdio` and use `info registers` / `xp /Nbx <addr>` / `screendump`.

## QEMU monitor quirks (Windows)

- `-monitor stdio` works with piped stdin **only if you don't use `communicate()`** — write commands, sleep, kill, then read the file. Telnet/tcp monitor chardevs refused connections in testing.
- Pattern that works: `(sleep 5; echo "screendump build/x.ppm"; sleep 1; echo quit) | qemu ... -monitor stdio`
- **`screendump` writes PPM, not PNG, regardless of extension** — Windows can't open it. Convert with `python tools/ppm2png.py build/x.ppm build/x.png`.

## War stories (hard-won; read before re-fighting these ghosts)

### #1 — 16-bit `mov dx, imm16` executed in 32-bit mode eats the next byte
The boot sector's BITS-16 `serial_putc` (`mov dx, 0x3FD` = 3 bytes, no prefix) was called from 32-bit `pm_entry`. In 32-bit mode, `BA` is `mov edx, imm32` (4 bytes) — it swallowed the following `in al, dx` byte into the immediate, then misdecoded the next instructions, popped wrong, and `ret`'d to garbage (EIP=0xEFD79080). **Rule: a routine's encoding must match the mode it executes in — never call a 16-bit routine from 32-bit code.**

### #2 — QEMU 11 SeaBIOS hangs on int 13h AH=42h with a buffer above 1MB
Reading the kernel directly to 0x100000 made SeaBIOS spin in its own serial debug loop (its high-memory path enables PAE with its own page tables at 0x9000 and gets stuck; the IDE controller is never touched, serial shows `ffffff` from SeaBIOS itself). **Fix: read to a low buffer (0x10000, below 1MB), then `rep movsw` it up to 0x100000 in real mode.** A20 must be on before the copy (it is — SeaBIOS enables it during POST).

### #3 — `0x200083` is not a "2MiB page" flag — it's a physical base address
The identity-map PD fill started with `mov eax, 0x200000 | 0x83`. In a 2MiB page-directory entry, bits 21–31 ARE the physical base: `0x200083` maps virtual 0–2MiB to physical **2–4MiB**. Every fetch after CR0.PG landed in empty RAM → the "zero walk" (executing 00 00 forever). Symptoms were beautifully misleading: the kernel ran fine pre-paging, all registers/table entries "correct" (they were exactly what the code wrote — the code was wrong). **Fix: start the fill at `0x83` (base 0).**

### #4 — The 16550 UART is PORT-mapped I/O — memory stores to 0x3F8 are silent
`*(volatile unsigned char *)0x3F8 = c` compiles to `movb %al, 0x3f8` — a memory write to RAM. The serial port needs the `out` **instruction**. Inline asm: `outb %0, %w1` with `"a"(val), "d"(port)` (the `%w1` 16-bit view is mandatory — `out` only accepts DX or imm8, and 0x3F8 doesn't fit imm8). The VGA text buffer at 0xB8000, by contrast, is genuinely memory-mapped — plain stores work.

### #5 — SysV ABI: the first char argument arrives in DIL, not AL
An asm routine exported to C as `kputc(char c)` must read `dil`, not the pushed `al`. Wrong convention = it prints whatever RAX happened to hold (we got three 'K's where 'a','b','c' belonged — which is how we found it).

### #6 — QEMU stdio chardev bursts >16 bytes into the UART RX FIFO (test harness)
Piped input to `-serial stdio` is pushed into the 16550's RX FIFO synchronously — a burst larger than the 16-byte FIFO silently drops the overflow, even though the kernel polls and drains constantly. Real terminals never burst (humans type slowly), so the kernel is fine — **automated tests must chunk input ≤ 15 bytes per write with small gaps.** Related: with `-serial file:...`, QEMU's stdin goes *nowhere* — guest input must come via `-serial stdio`; and our hex dumps are lowercase (`KB: 0x1e`), so grep lowercase.

## Anti-patterns learned

- Debug milestones in the boot chain (one char per stage, serial) beat monitor forensics for boot failures — add them FIRST, not after hours of tracing.
- Verify the buffer AND the destination when a copy "completes" — the D/E dumps (raw bytes over serial) settled a read-vs-copy argument instantly.
- `-d in_asm` trace is definitive about what executed; `-d cpu` adds register state. Combined they turn "impossible" bugs into visible ones.
