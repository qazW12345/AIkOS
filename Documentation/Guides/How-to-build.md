# How to build AIkOS

> **Status:** current — toolchain installed and verified 2026-08-05 (ADR-005).

## Known environment quirks (hard-won knowledge — read before debugging!)

- **`python3` is a trap on this machine.** Typing `python3` opens the Microsoft Store instead of running Python. The correct command is **`python`**. (Cost us time once; never again.)
- **WSL is a dead end for now.** WSL exists but has no distro installed, and hardware virtualization is disabled in the firmware. Using WSL would require a BIOS change + reboot — do not go down this path without explicit approval.
- **`winget` is available** for installing tools.
- **Work lives on E:\** (`E:\Hermes_Agent\projects\AIkOS`). C: is limited; avoid it.
- Shell in the Hermes terminal is git-bash (POSIX syntax), not PowerShell/cmd.
- **`gh` CLI device-flow login is broken in this terminal** (browser confirms, CLI never receives the token). Workaround: PAT + API + `git credential approve`. gh is installed but not authenticated.

## Toolchain (installed 2026-08-05, ADR-005)

| Tool | Version | Location | Verified |
|---|---|---|---|
| NASM | 3.02 | `C:\Users\marce\AppData\Local\bin\NASM\nasm.exe` (per-user) | ✅ `-f elf64` |
| clang/LLVM | 22.1.8 | `C:\Program Files\LLVM\bin\` (clang.exe, ld.lld.exe, llvm-objcopy.exe) | ✅ freestanding `x86_64-elf` → ELF64 |
| QEMU | 11.0.50 | `C:\Program Files\qemu\qemu-system-x86_64.exe` | ✅ launches |
| python | 3.11 | `python` (not `python3`!) | ✅ image assembly |

**PATH caveat:** none of the above are on the PATH of existing shells (installed after session start). `build.sh` / `test.sh` must resolve absolute paths or export PATH at the top — do not rely on `nasm`/`clang`/`qemu-system-x86_64` resolving bare.

**QEMU timeout note:** a VM with no bootable disk does not exit on its own — command timeouts are expected, not failures. Real acceptance uses `-no-reboot` + timeout + serial-log grep (see `test.sh`).

## Kernel compile flags (ADR-005, design doc)

`clang --target=x86_64-elf -ffreestanding -nostdlib -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -O2 -Wall -Wextra -std=c11`

## Build steps

*(filled in when build.sh lands — see Design/Phase-0-Proof-of-Life.md for the full pipeline)*
