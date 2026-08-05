# How to run AIkOS in QEMU

> **Status:** current — Phase 0 verified commands (2026-08-05).

## Canonical acceptance test

```bash
./test.sh        # builds, boots headless, greps serial log for the banner
```

## Manual runs

```bash
QEMU="/c/Program Files/qemu/qemu-system-x86_64.exe"

# headless, serial to file (the standard debug run)
"$QEMU" -drive file=build/disk.img,format=raw \
        -serial file:build/serial.log -display none -no-reboot -m 32M

# with monitor for registers/memory/screenshot
(sleep 5; echo "screendump build/screen.ppm"; sleep 1; echo quit) | \
"$QEMU" -drive file=build/disk.img,format=raw -serial file:build/serial.log \
        -display none -no-reboot -m 32M -monitor stdio
```

## Gotchas

- **A VM with no bootable disk never exits** — timeouts are expected, not failures. test.sh handles it.
- **`screendump` outputs PPM, not PNG** (Windows can't open it): `python tools/ppm2png.py build/screen.ppm build/screen.png`.
- **`-serial file:`** captures raw bytes — includes the boot-chain milestone characters (see How-to-debug.md).
- QEMU 11 SeaBIOS: int 13h AH=42h reads **must** use a buffer below 1MB (see war story #2) — our boot sector handles this internally now.

## Expected boot log (Phase 2, v0.4.0)

```
SBMEUFALCP123456789KAIkOS v0.4.0
Two Worlds
AIkOS> help
commands: help, echo <text>, ticks, version, panic, time, cpuid, vga, run, runfault
AIkOS> echo hello world
hello world
AIkOS> ticks
ticks: 431
AIkOS> run
entering ring 3...
SYSCALL 1 (write) len=18
hello from ring 3
SYSCALL 2 (exit)
user exited
back in kernel
AIkOS> runfault
entering ring 3 (faulting program)...
USER FAULT 13 (GENERAL PROTECTION) error=0000000000000000
... (register dump) ...
user program terminated
back in kernel
AIkOS>
```

## Interactive use

```
QEMU ... -serial stdio -display none -no-reboot -m 32M
```
Type into the terminal — the REPL echoes and responds. `panic` deliberately faults (ud2) to exercise the exception dump (ADR-009). `run` enters ring 3 (the user program syscalls out); `runfault` runs a program that deliberately #GPs so you can watch the kernel survive (ADR-013).
