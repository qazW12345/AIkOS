#!/usr/bin/env bash
# AIkOS Phase 0 acceptance test (design doc): build, boot headless in QEMU,
# grep the serial log for the banner. Exit 0 = green.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

./build.sh

QEMU="/c/Program Files/qemu/qemu-system-x86_64.exe"
rm -f build/serial.log

echo "[boot] running QEMU headless (30s cap)"
python - "$QEMU" <<'PYEOF'
import subprocess, sys

qemu = sys.argv[1]
try:
    subprocess.run(
        [qemu, "-drive", "file=build/disk.img,format=raw",
         "-serial", "file:build/serial.log",
         "-display", "none", "-no-reboot", "-m", "32M"],
        timeout=30,
    )
except subprocess.TimeoutExpired:
    print("(QEMU still running after 30s — expected: kernel halts, VM never exits)")
PYEOF

if grep -q "AIkOS v0.1.0" build/serial.log 2>/dev/null; then
    echo "=== PASS: banner found in serial log ==="
    cat build/serial.log
    exit 0
else
    echo "=== FAIL: banner not found in serial log ==="
    cat build/serial.log 2>/dev/null || echo "(no serial log was produced)"
    exit 1
fi
