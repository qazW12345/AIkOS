#!/usr/bin/env bash
# AIkOS build script (ADR-005) — nasm -> clang -> ld.lld -> objcopy -> disk image.
# Toolchain paths come from env.sh (platform-aware, ADR-011).
# Phase 2 (ADR-012/013): disk image = boot + kernel + user + userfault blobs.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
. ./env.sh

KERNEL_SECTORS=64   # must match the design doc memory map
USER_SECTORS=16
FAULT_SECTORS=16
USER_LBA=$((KERNEL_SECTORS + 1))
FAULT_LBA=$((USER_LBA + USER_SECTORS))

CFLAGS="-ffreestanding -nostdlib -fno-stack-protector -fno-pic -fno-pie \
-mno-red-zone -mgeneral-regs-only -O2 -Wall -Wextra -std=c11"
UCFLAGS="-ffreestanding -nostdlib -fno-stack-protector -fno-pic -fno-pie \
-O2 -Wall -Wextra -std=c11"        # user code: no -mgeneral-regs-only (XMM ok)

mkdir -p build

echo "[1/7] boot sector"
"$NASM" -f bin \
    -D KERNEL_SECTORS=$KERNEL_SECTORS -D USER_SECTORS=$USER_SECTORS \
    -D FAULT_SECTORS=$FAULT_SECTORS -D USER_LBA=$USER_LBA -D FAULT_LBA=$FAULT_LBA \
    -o build/boot.bin src/boot/boot.asm

echo "[2/7] kernel assembly (entry + interrupt stubs)"
"$NASM" -f elf64 -D USER_SECTORS=$USER_SECTORS -D FAULT_SECTORS=$FAULT_SECTORS \
    -o build/entry.o src/kernel/entry.asm
"$NASM" -f elf64 -o build/interrupt.o src/kernel/interrupt.asm

echo "[3/7] kernel C"
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/kmain.o    src/kernel/kmain.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/serial.o   src/kernel/serial.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/vga.o      src/kernel/vga.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/idt.o      src/kernel/idt.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/pic.o      src/kernel/pic.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/pit.o      src/kernel/pit.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/keyboard.o src/kernel/keyboard.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/repl.o     src/kernel/repl.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/printf.o   src/kernel/printf.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/rtc.o      src/kernel/rtc.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/cpuid.o    src/kernel/cpuid.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/mm.o       src/kernel/mm.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/tss.o      src/kernel/tss.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/syscall.o  src/kernel/syscall.c
"$CLANG" --target=x86_64-elf $CFLAGS -c -o build/proc.o     src/kernel/proc.c

echo "[4/7] link (entry.o first so the binary starts with _start)"
"$LLD" -T linker.ld -o build/kernel.elf \
    build/entry.o build/kmain.o build/serial.o build/vga.o build/interrupt.o \
    build/idt.o build/pic.o build/pit.o build/keyboard.o build/repl.o \
    build/printf.o build/rtc.o build/cpuid.o build/mm.o build/tss.o \
    build/syscall.o build/proc.o

echo "[5/7] flat binaries"
"$OBJCOPY" -O binary build/kernel.elf build/kernel.bin

KMAX=$((KERNEL_SECTORS * 512))
KSIZE=$(stat -c%s build/kernel.bin)
if [ "$KSIZE" -gt "$KMAX" ]; then
    echo "ERROR: kernel.bin is $KSIZE bytes, exceeds $KMAX"
    exit 1
fi
echo "       kernel.bin: $KSIZE bytes (budget $KMAX)"

echo "[6/7] user programs (ring 3, ADR-013)"
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/user_main.o user/main.c
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/user_fault.o user/fault.c
"$LLD" -Ttext=0x200000 -o build/user.elf build/user_main.o
"$LLD" -Ttext=0x220000 -o build/userfault.elf build/user_fault.o
"$OBJCOPY" -O binary build/user.elf build/user.bin
"$OBJCOPY" -O binary build/userfault.elf build/userfault.bin

USIZE=$(stat -c%s build/user.bin)
FSIZE=$(stat -c%s build/userfault.bin)
UMAX=$((USER_SECTORS * 512))
if [ "$USIZE" -gt "$UMAX" ] || [ "$FSIZE" -gt "$UMAX" ]; then
    echo "ERROR: user blob too big: $USIZE / $FSIZE bytes (budget $UMAX)"
    exit 1
fi
echo "       user.bin: $USIZE bytes, userfault.bin: $FSIZE bytes (budget $UMAX)"

echo "[7/7] disk image (boot + kernel + user + userfault)"
"$PYTHON" - "$KERNEL_SECTORS" "$USER_SECTORS" "$FAULT_SECTORS" <<'PYEOF'
import sys

kernel_sectors, user_sectors, fault_sectors = (int(x) for x in sys.argv[1:4])
with open('build/boot.bin', 'rb') as f:
    boot = f.read()
with open('build/kernel.bin', 'rb') as f:
    kernel = f.read()
with open('build/user.bin', 'rb') as f:
    user = f.read()
with open('build/userfault.bin', 'rb') as f:
    fault = f.read()

assert len(boot) <= 512, "boot.bin too big"
img = boot.ljust(512, b'\x00')
img += kernel.ljust(kernel_sectors * 512, b'\x00')
img += user.ljust(user_sectors * 512, b'\x00')
img += fault.ljust(fault_sectors * 512, b'\x00')

with open('build/disk.img', 'wb') as f:
    f.write(img)

print(f"       disk.img: {len(img)} bytes ({len(img) // 512} sectors): "
      f"boot+kernel({kernel_sectors})+user({user_sectors})+fault({fault_sectors})")
PYEOF

echo "BUILD OK"
