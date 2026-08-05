#!/usr/bin/env bash
# AIkOS Phase 1.5 acceptance tests (Design/Phase-1.5-The-Senses.md):
#   t1 regression  — boot + banner (v0.3.0)
#   t2 REPL        — help/echo/ticks over -serial stdio
#   t3 keyboard    — scancodes + keyboard-typed command (sendkey)
#   t4 panic       — ud2 -> exception dump + halt (ADR-009)
#   t5 time        — RTC command (ADR-010)
#   t6 cpuid       — CPUID command (ADR-010)
# Input chunking <=15 bytes with gaps: QEMU's stdio chardev bursts into the
# 16550 RX FIFO and drops overflow (war story #6).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
. ./env.sh

./build.sh

PASS=0
FAIL=0
ok()  { echo "PASS: $1"; PASS=$((PASS+1)); }
bad() { echo "FAIL: $1"; FAIL=$((FAIL+1)); }

echo "[t1] Phase 0/1 regression (boot + banner)"
rm -f build/serial.log
"$PYTHON" - "$QEMU" <<'PYEOF'
import subprocess, sys
try:
    subprocess.run([sys.argv[1], "-drive", "file=build/disk.img,format=raw",
                    "-serial", "file:build/serial.log", "-display", "none",
                    "-no-reboot", "-m", "32M"], timeout=20)
except subprocess.TimeoutExpired:
    pass
PYEOF
if grep -q "AIkOS v0.3.0" build/serial.log; then ok "t1 boot banner"; else bad "t1 boot banner"; fi
if grep -q "The Senses" build/serial.log; then ok "t1 phase line"; else bad "t1 phase line"; fi

echo "[t2] REPL over serial (piped input)"
rm -f build/repl.out
( sleep 2
  printf 'help\n';        sleep 0.5
  printf 'echo hello ';   sleep 0.3
  printf 'world\n';       sleep 0.5
  printf 'ticks\n';       sleep 0.5
  printf 'ticks\n';       sleep 2 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/repl.out 2>/dev/null &
QPID=$!
sleep 10
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "commands: help" build/repl.out; then ok "t2 help lists commands"; else bad "t2 help"; fi
if grep -q "hello world" build/repl.out; then ok "t2 echo works"; else bad "t2 echo"; fi
T1=$(grep -oP 'ticks: \K\d+' build/repl.out | head -1)
T2=$(grep -oP 'ticks: \K\d+' build/repl.out | tail -1)
if [ -n "$T1" ] && [ -n "$T2" ] && [ "$T1" != "$T2" ]; then
    ok "t2 ticks increment ($T1 -> $T2)"
else
    bad "t2 ticks increment (got '$T1' '$T2')"
fi

echo "[t3] keyboard scancodes + keyboard-typed command (sendkey)"
rm -f build/kbd.log
( sleep 4
  echo "sendkey a";   sleep 0.5
  echo "sendkey ret"; sleep 0.5
  echo "sendkey b";   sleep 0.5
  echo "sendkey ret"; sleep 0.5
  echo "sendkey h";   sleep 0.3
  echo "sendkey e";   sleep 0.3
  echo "sendkey l";   sleep 0.3
  echo "sendkey p";   sleep 0.3
  echo "sendkey ret"; sleep 1
  echo "quit" ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial file:build/kbd.log \
    -display none -no-reboot -m 32M -monitor stdio >/dev/null 2>&1
if grep -q "KB: 0x1e" build/kbd.log; then ok "t3 scancode a (0x1e)"; else bad "t3 scancode a"; fi
if grep -q "KB: 0x30" build/kbd.log; then ok "t3 scancode b (0x30)"; else bad "t3 scancode b"; fi
if grep -q "commands: help" build/kbd.log; then ok "t3 keyboard-typed command"; else bad "t3 keyboard-typed command"; fi

echo "[t4] panic command -> exception dump + halt"
rm -f build/panic.out
( sleep 2; printf 'panic\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/panic.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "EXCEPTION 6 (INVALID OPCODE)" build/panic.out; then ok "t4 vector 6 named"; else bad "t4 vector 6 named"; fi
if grep -q "rax=" build/panic.out; then ok "t4 register dump"; else bad "t4 register dump"; fi
if grep -q "rip=" build/panic.out; then ok "t4 rip in dump"; else bad "t4 rip in dump"; fi

echo "[t5] time (RTC)"
rm -f build/time.out
( sleep 2; printf 'time\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/time.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -qE "20[0-9]{2}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}" build/time.out; then
    ok "t5 time format"
else
    bad "t5 time format"
fi

echo "[t6] cpuid"
rm -f build/cpuid.out
( sleep 2; printf 'cpuid\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/cpuid.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "cpuid: vendor" build/cpuid.out; then ok "t6 vendor"; else bad "t6 vendor"; fi
if grep -q "cpuid: family" build/cpuid.out; then ok "t6 family"; else bad "t6 family"; fi

echo
echo "RESULT: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
