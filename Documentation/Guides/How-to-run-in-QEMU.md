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

## Expected boot log (Phase 1, v0.2.0)

```
SBMALCP123456789KAIkOS v0.2.0
The Machine Wakes
AIkOS> help
commands: help, echo <text>, ticks, version, panic
AIkOS> echo hello world
hello world
AIkOS> ticks
ticks: 431
```

## Interactive use

```
QEMU ... -serial stdio -display none -no-reboot -m 32M
```
Type into the terminal — the REPL echoes and responds. `panic` deliberately faults (ud2) to exercise the exception dump (ADR-009).
