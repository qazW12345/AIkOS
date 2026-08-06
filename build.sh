#!/usr/bin/env bash
# shellcheck disable=SC2086   # $CFLAGS etc. are intentionally word-split
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
FS_SECTORS=64       # AIkFS partition (LBA 97; ramdisk at 0x400000)
USER_LBA=$((KERNEL_SECTORS + 1))
FAULT_LBA=$((USER_LBA + USER_SECTORS))
FS_LBA=$((FAULT_LBA + FAULT_SECTORS))

CFLAGS="-ffreestanding -nostdlib -fno-stack-protector -fno-pic -fno-pie \
-mno-red-zone -mgeneral-regs-only -O2 -Wall -Wextra -std=c11"
UCFLAGS="-ffreestanding -nostdlib -fno-stack-protector -fno-pic -fno-pie \
-O2 -Wall -Wextra -std=c11"        # user code: no -mgeneral-regs-only (XMM ok)

mkdir -p build

echo "[1/7] boot sector"
"$NASM" -f bin \
    -D KERNEL_SECTORS=$KERNEL_SECTORS -D USER_SECTORS=$USER_SECTORS \
    -D FAULT_SECTORS=$FAULT_SECTORS -D USER_LBA=$USER_LBA -D FAULT_LBA=$FAULT_LBA \
    -D FS_SECTORS=$FS_SECTORS -D FS_LBA=$FS_LBA \
    -o build/boot.bin src/boot/boot.asm

echo "[2/7] kernel assembly (entry + interrupt stubs)"
"$NASM" -f elf64 -D USER_SECTORS=$USER_SECTORS -D FAULT_SECTORS=$FAULT_SECTORS \
    -D FS_SECTORS=$FS_SECTORS \
    -o build/entry.o src/kernel/entry.asm
"$NASM" -f elf64 -o build/interrupt.o src/kernel/interrupt.asm

echo "[3/7] kernel C (source list derived from the filesystem — ADR-014)"
KOBJS=""
for f in src/kernel/*.c; do
    o="build/$(basename "$f" .c).o"
    "$CLANG" --target=x86_64-elf $CFLAGS -c -o "$o" "$f"
    KOBJS="$KOBJS $o"
done

echo "[4/7] link (entry.o first so the binary starts with _start)"
"$LLD" -T linker.ld -o build/kernel.elf build/entry.o build/interrupt.o $KOBJS

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

# Build ELF user programs for AIkFS (/bin/hello.elf, /bin/ver.elf, /bin/readtest.elf, /bin/opentest.elf, /bin/closetest.elf)
# Same flags as user/main.c: UCFLAGS + link at 0x200000 (ET_EXEC ELF64)
echo "[7/7] ELF user programs (ring 3, AIkFS apps)"
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/hello.o user/hello.c
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/ver.o user/ver.c
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/readtest.o user/readtest.c
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/opentest.o user/opentest.c
"$CLANG" --target=x86_64-elf $UCFLAGS -c -o build/closetest.o user/closetest.c
"$LLD" -Ttext=0x200000 -o build/hello.elf build/hello.o
"$LLD" -Ttext=0x200000 -o build/ver.elf build/ver.o
"$LLD" -Ttext=0x200000 -o build/readtest.elf build/readtest.o
"$LLD" -Ttext=0x200000 -o build/opentest.elf build/opentest.o
"$LLD" -Ttext=0x200000 -o build/closetest.elf build/closetest.o

# Stage ELF binaries into build/bin/ for buildfs.py
mkdir -p build/bin
cp build/hello.elf build/bin/hello.elf
cp build/ver.elf build/bin/ver.elf
cp build/readtest.elf build/bin/readtest.elf
cp build/opentest.elf build/bin/opentest.elf
cp build/closetest.elf build/bin/closetest.elf

# Create base disk image (boot + kernel + user + userfault blobs — 97 sectors)
echo "[8/8] disk image (boot + kernel + user + userfault + AIkFS partition)"
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

# Bake AIkFS partition at LBA 97 (64 sectors = 32 KiB) from build/bin/
# Disk grows from 97 to 161 sectors.
"$PYTHON" tools/buildfs.py build/disk.img 97 64 build/bin

echo "BUILD OK"
