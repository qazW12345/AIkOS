# How to build AIkOS

> **Status:** placeholder — toolchain not installed yet (2026-08-05). This page will contain exact, copy-pasteable commands once the toolchain is decided.

## Known environment quirks (hard-won knowledge — read before debugging!)

- **`python3` is a trap on this machine.** Typing `python3` opens the Microsoft Store instead of running Python. The correct command is **`python`**. (Cost us time once; never again.)
- **WSL is a dead end for now.** WSL exists but has no distro installed, and hardware virtualization is disabled in the firmware. Using WSL would require a BIOS change + reboot — do not go down this path without explicit approval.
- **`winget` is available** for installing tools (QEMU, NASM, compilers).
- **Work lives on E:\** (`E:\Hermes_Agent\projects\AIkOS`). C: is limited; avoid it.
- Shell in the Hermes terminal is git-bash (POSIX syntax), not PowerShell/cmd.

## Toolchain plan (not yet executed)

| Tool | Purpose | Install path | Status |
|---|---|---|---|
| NASM | assembler (boot code) | winget | ⬜ |
| QEMU | emulator / test target | winget | ⬜ |
| Compiler | kernel C (freestanding) | **TBD** — candidates: clang/LLVM (cross-compiles out of the box), MSYS2 gcc, OSDev cross-toolchain zip | ⬜ |
| make | build orchestration | winget (or via compiler package) | ⬜ |

## Build steps

*(to be filled in after toolchain install — see TaskLog for the decision trail)*
