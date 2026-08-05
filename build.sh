#!/usr/bin/env bash
# AIkOS build script (ADR-005) — nasm -> clang -> ld.lld -> objcopy -> disk image.
# Paths are absolute: the winget-installed tools are NOT on existing shells' PATH.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

NASM="/c/Users/marce/AppData/Local/bin/NASM/nasm.exe"
CLANG="/c/Program Files/LLVM/bin/clang.exe"
LLD="/c/Program Files/LLVM/bin/ld.lld.exe"
OBJCOPY="/c/Program Files/LLVM/bin/llvm-objcopy.exe"
PYTHON="python"   # never python3 on this machine (see Guides)

KERNEL_SECTORS=64   # must match boot.asm default; injected at assembly time

CFLAGS="-ffreestanding -nostdlib -fno-stack-protector -fno-pic -fno-pie \
-mno-red-zone -mgeneral-regs-only -O2 -Wall -Wextra -std=c11"

mkdir -p build

echo "[1/6] boot sector"
"$NASM" -f bin -D KERNEL_SECTORS=$KERNEL_SECTORS -o build/boot.bin src/boot/boot.asm

echo "[2/6] kernel assembly (entry + interrupt stubs)"
"$NASM" -f elf64 -o build/entry.o src/kernel/entry.asm
"$NASM" -f elf64 -o build/interrupt.o src/kernel/interrupt.asm

echo "[3/6] kernel C"
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/kmain.o    src/kernel/kmain.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/serial.o   src/kernel/serial.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/vga.o      src/kernel/vga.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/idt.o      src/kernel/idt.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/pic.o      src/kernel/pic.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/pit.o      src/kernel/pit.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/keyboard.o src/kernel/keyboard.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/repl.o     src/kernel/repl.c

echo "[4/6] link (entry.o first so the binary starts with _start)"
"$LLD" -T linker.ld -o build/kernel.elf \
    build/entry.o build/kmain.o build/serial.o build/vga.o build/interrupt.o \
    build/idt.o build/pic.o build/pit.o build/keyboard.o build/repl.o

echo "[5/6] flat kernel binary"
"$OBJCOPY" -O binary build/kernel.elf build/kernel.bin

SIZE=$(stat -c%s build/kernel.bin)
MAX=$((KERNEL_SECTORS * 512))
if [ "$SIZE" -gt "$MAX" ]; then
    echo "ERROR: kernel.bin is $SIZE bytes, exceeds $MAX (KERNEL_SECTORS=$KERNEL_SECTORS)"
    exit 1
fi
echo "       kernel.bin: $SIZE bytes (budget $MAX)"

echo "[6/6] disk image (boot sector + kernel, raw layout)"
"$PYTHON" - "$KERNEL_SECTORS" <<'PYEOF'
import sys

sectors = int(sys.argv[1])
with open('build/boot.bin', 'rb') as f:
    boot = f.read()
with open('build/kernel.bin', 'rb') as f:
    kernel = f.read()

assert len(boot) <= 512, "boot.bin too big"
assert len(kernel) <= sectors * 512, "kernel.bin exceeds sector budget"

img = boot.ljust(512, b'\x00') + kernel.ljust(sectors * 512, b'\x00')
with open('build/disk.img', 'wb') as f:
    f.write(img)

print(f"       disk.img: {len(img)} bytes ({len(img) // 512} sectors), kernel {len(kernel)} bytes")
PYEOF

echo "BUILD OK"
