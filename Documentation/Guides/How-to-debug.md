# How to debug AIkOS

> **Status:** current — the project debug toolkit (2026-08-06). Serial milestones, QEMU monitor, and war stories.

## The debug loop (proven in Phase 0)

1. Build: `./build.sh`
2. Boot headless with serial to file: `qemu-system-x86_64 -drive file=build/disk.img,format=raw -serial file:build/serial.log -display none -no-reboot -m 32M`
3. Read the serial log — the **milestone characters** tell you exactly how far the boot chain got:

```
S  boot sector entered            B  disk read OK          M  kernel copied to 1MB
E  E820 map collected             U  user blob read        F  fault blob read
R  AIkFS ramdisk read             A  A20 on                L  GDT loaded
C  CR0.PE set                    P  protected mode         1  kernel entry
2  page tables built             3  CR3 set                4  CR4.PAE set
5  EFER.LME set                  6  CR0.PG set (paging on) 7  GDT64 loaded
8  long mode reached             9  C stack set            K  calling kmain
```

4. If it dies early: check the milestones; if the boot sector itself fails, add `-monitor stdio` and use `info registers` / `xp /Nbx <addr>` / `screendump`.

## QEMU concurrency (multi-agent, ADR-018)

- **Only one actor runs the suite per worktree** — test.sh now refuses to start
  if `build/.test.lock` exists (single-runner lock). Separate worktrees are
  independent (their own build/ dirs).
- **Never kill QEMU by image name** (`taskkill //F //IM qemu-system-x86_64.exe`
  kills EVERY instance on the machine — including another agent's in-flight
  tests, which produced exactly that flaky failure in Phase 3). Kill by PID:
  capture `$!` at launch, or `taskkill //F //PID <pid>`. The
  `tools/qemu_run.sh` helper does this for you — it also fixes the stuck-pipe
  hang (input via `--in-cmd "sleep 4; printf '...'; sleep 1; printf '...'"`,
  bounded wait, guaranteed exit).
- Windows QEMU itself is fine with multiple concurrent instances — the
  collisions were purely the image-name kills and shared build/ dirs.

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

### #7 — User access needs U/S at EVERY level: PML4, PDPT, AND the PDE
The ring-3 entry iretq'd fine, but the very first user fetch #PF'd with error=5 (present + U/S violation) at the user code — despite the PDE being a perfect user page (0x2000e7). The handler's own dump showed why: `pml4[0]=0x10d023`, `pdpt[0]=0x10e023` — **bit 2 (U/S) was zero on the upper levels**. x86 requires U/S set at every level of the walk; the PDE alone isn't enough. A ring-0 probe read of the same address *succeeded* (ring 0 ignores U/S), which pointed the diagnosis at stale TLBs first — a red herring. **Fix: `pml4[0] = pdpt | 0x7` and `pdpt[0] = pd | 0x7` (P|RW|U/S), not `| 0x3`.**

### #8 — Same-ring iretq validates SS.RPL == CPL — a leftover user SS is #GP(0x20)
Killing a ring-3 task rewrites the interrupt frame's cs/rip to return to ring 0 (same-ring return). The frame's SS field still held the user's 0x23 (RPL 3) — and the CPU checks SS.RPL against CPL even when it doesn't pop SS on a same-ring return → **#GP with error code 0x20 (the selector, RPL masked)**. Every working iretq had SS.RPL == CPL: PIT frames read kernel stack addresses (low bits 0), ring-3 returns pop a matching SS. **Fix: rewrite the whole frame tail — `cs=0x08, ss=0x10, rsp=stack_top, rip=trampoline`.**

### #9 — You can't iretq your way back to a call chain the interrupt frames ate
After the frame-rewrite iretq lands in the kernel, the REPL call chain is gone: (a) ring-3 interrupt frames are pushed at TSS RSP0 (= stack_top) and overwrite the chain's top ~176 bytes — including the return addresses; (b) the user program clobbers callee-saved registers (its own rbp); (c) the interrupt frame only preserves the kernel's values *at int time*, which are the user's after ring-3 clobbering. **Fix: capture everything BEFORE entering ring 3 — the resume address and all six callee-saved registers — park the stack 4 KiB below the frame zone (`sub $0x1000, %rsp`), and the trampoline restores stack + registers and `jmp`s to the captured address.** Two capture pitfalls: the compiler's prologue runs before inline asm, so `[rsp]` at "function entry" is NOT the return address — use the frame-pointer guarantees `[rbp]` (caller's rbp) and `[rbp+8]` (return address); and `movq mem, mem` is invalid — use a register output for memory loads.

## Anti-patterns learned

- Debug milestones in the boot chain (one char per stage, serial) beat monitor forensics for boot failures — add them FIRST, not after hours of tracing.
- Verify the buffer AND the destination when a copy "completes" — the D/E dumps (raw bytes over serial) settled a read-vs-copy argument instantly.
- `-d in_asm` trace is definitive about what executed; `-d cpu` adds register state. Combined they turn "impossible" bugs into visible ones.
