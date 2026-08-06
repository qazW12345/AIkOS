#!/usr/bin/env bash
# AIkOS Phase 2 acceptance tests (Design/Phase-2-Two-Worlds.md):
#   t1 regression  — boot + banner (v0.5.0)

# Single-runner lock (multi-agent safety, ADR-018): only one suite per worktree.
# Two actors in the SAME tree would clobber build/serial.log; separate worktrees
# have separate build/ dirs and need no coordination.
mkdir -p build
if [ -d build/.test.lock ]; then
    echo "test.sh: another suite is running in this worktree (build/.test.lock exists) — exiting."
    exit 1
fi
mkdir build/.test.lock
trap 'rmdir build/.test.lock 2>/dev/null' EXIT
#   t2 REPL        — help/echo/ticks over -serial stdio
#   t3 keyboard    — scancodes + keyboard-typed command (sendkey)
#   t4 panic       — ud2 -> exception dump + halt (ADR-009)
#   t5 time        — RTC command (ADR-010)
#   t6 cpuid       — CPUID command (ADR-010)
#   t7 run         — ring-3 program syscalls out (ADR-013)
#   t8 runfault    — user fault kills task, kernel lives (ADR-013)
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

# --- Modular test selection (2026-08-06) -----------------------------------
# Usage:  ./test.sh               full suite (L3 — the release/batch-end gate)
#         ./test.sh <keywords>    subset — core (t1-t3,t5,t6) + static (t17)
#                                 always run; then only the named groups.
# Keywords (comma/space separated): syscall ring3 fs mm debug kernel full
# Rules:
#   independence — untouched subsystems' tests can't break, so they're skipped;
#   subsumption  — syscall runs drop t7/t15 (runelf/ring-3 machinery is fully
#                  exercised by t18/t19, which are strictly more complex);
#   risk         — boot-path / kernel.h / kmain.c changes ALWAYS want the full
#                  suite (./test.sh) — see tools/test_plan.sh for the mapper.
FILTER="${1:-}"
case "$FILTER" in ""|full|all) FILTER="";; esac
GROUP_SYSCALL="t8 t18 t19 t20"
GROUP_RING3="t7 t8 t15"
GROUP_FS="t13 t14 t15"
GROUP_MM="t11 t12 t16"
GROUP_DEBUG="t9 t10"
GROUP_KERNEL="t4 t8"
want_group() {  # $1=test id, $2=subsystem
    [ -z "$FILTER" ] && return 0
    local key g
    for key in $FILTER; do
        [ "$key" = "$2" ] && return 0
        eval "g=\$GROUP_$(echo "$key" | tr '[:lower:]' '[:upper:]')"
        case " $g " in *" $1 "*) return 0;; esac
    done
    return 1
}

echo "[t1] Phase 0/1/1.5 regression (boot + banner)"
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
if grep -q "AIkOS v0.5.0" build/serial.log; then ok "t1 boot banner"; else bad "t1 boot banner"; fi
if grep -q "Memory & Files" build/serial.log; then ok "t1 phase line"; else bad "t1 phase line"; fi
if grep -q "SBMEUFRALCP" build/serial.log; then ok "t1 boot chain (FS ramdisk)"; else bad "t1 boot chain (FS ramdisk)"; fi

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

if want_group t4 kernel; then
echo "[t4] panic command -> exception dump + halt"
rm -f build/panic.out
( sleep 4; printf 'panic\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/panic.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "EXCEPTION 6 (INVALID OPCODE)" build/panic.out; then ok "t4 vector 6 named"; else bad "t4 vector 6 named"; fi
if grep -q "rax=" build/panic.out; then ok "t4 register dump"; else bad "t4 register dump"; fi
if grep -q "rip=" build/panic.out; then ok "t4 rip in dump"; else bad "t4 rip in dump"; fi
fi

echo "[t5] time (RTC)"
rm -f build/time.out
( sleep 4; printf 'time\n'; sleep 3 ) | \
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
( sleep 4; printf 'cpuid\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/cpuid.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "cpuid: vendor" build/cpuid.out; then ok "t6 vendor"; else bad "t6 vendor"; fi
if grep -q "cpuid: family" build/cpuid.out; then ok "t6 family"; else bad "t6 family"; fi

if want_group t7 ring3; then
echo "[t7] ring-3 program syscalls out (ADR-013)"
rm -f build/user.out
( sleep 4; printf 'run\n'; sleep 4 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/user.out 2>/dev/null &
QPID=$!
sleep 9
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "SYSCALL 1 (write)" build/user.out; then ok "t7 syscall write"; else bad "t7 syscall write"; fi
if grep -q "hello from ring 3" build/user.out; then ok "t7 user text"; else bad "t7 user text"; fi
if grep -q "user exited" build/user.out; then ok "t7 syscall exit"; else bad "t7 syscall exit"; fi
if grep -q "back in kernel" build/user.out; then ok "t7 kernel survives"; else bad "t7 kernel survives"; fi
fi

if want_group t8 ring3; then
echo "[t8] user fault kills task, kernel lives (ADR-013)"
rm -f build/fault.out
( sleep 4; printf 'runfault\n'; sleep 4 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/fault.out 2>/dev/null &
QPID=$!
sleep 9
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "USER FAULT" build/fault.out; then ok "t8 user fault caught"; else bad "t8 user fault caught"; fi
if grep -q "GENERAL PROTECTION" build/fault.out; then ok "t8 fault named"; else bad "t8 fault named"; fi
if grep -q "user program terminated" build/fault.out; then ok "t8 task killed"; else bad "t8 task killed"; fi
if grep -q "back in kernel" build/fault.out; then ok "t8 kernel survives"; else bad "t8 kernel survives"; fi
fi

if want_group t9 debug; then
echo "[t9] hexdump REPL command"
rm -f build/hexdump.out
# input chunked <=15 bytes with gaps: QEMU's stdio chardev bursts overflow
# the 16550 RX FIFO (war story #6) — 'hexdump 200000 10' is 18 bytes raw
( sleep 4; printf 'hexdump 20000'; sleep 0.3; printf '0 10\n'; sleep 1
  printf 'hexdump z';     sleep 0.3; printf 'zz 10\n'; sleep 2 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/hexdump.out 2>/dev/null &
QPID=$!
sleep 12
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "55 48 89 e5" build/hexdump.out; then ok "t9 hexdump shows user prog bytes"; else bad "t9 hexdump shows user prog bytes"; fi
if grep -q "200000" build/hexdump.out; then ok "t9 hexdump shows address"; else bad "t9 hexdump shows address"; fi
if grep -q "bad address" build/hexdump.out; then ok "t9 hexdump bad address"; else bad "t9 hexdump bad address"; fi
fi

if want_group t10 debug; then
echo "[t10] unknown command handling"
rm -f build/unknown.out
# 'foobar\n' is 7 bytes — one write is fine per FIFO rule
( sleep 4; printf 'foobar\n'; sleep 1 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/unknown.out 2>/dev/null &
QPID=$!
sleep 6
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "unknown command (try help)" build/unknown.out; then ok "t10 unknown command"; else bad "t10 unknown command"; fi
fi

if want_group t11 mm; then
echo "[t11] heap REPL command"
rm -f build/heap.out
# 'heap\n' is 5 bytes — one write is fine per FIFO rule
( sleep 4; printf 'heap\n'; sleep 1 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/heap.out 2>/dev/null &
QPID=$!
sleep 6
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "heap: free" build/heap.out; then ok "t11 heap free pages"; else bad "t11 heap free pages"; fi
if grep -q "largest order" build/heap.out; then ok "t11 heap largest order"; else bad "t11 heap largest order"; fi
fi

if want_group t12 mm; then
echo "[t12] heaptest REPL command"
rm -f build/heaptest.out
# 'heaptest\n' is 9 bytes — one write is fine per FIFO rule
( sleep 4; printf 'heaptest\n'; sleep 10 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/heaptest.out 2>/dev/null &
QPID=$!
sleep 15
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "heaptest OK" build/heaptest.out; then ok "t12 heaptest OK"; else bad "t12 heaptest OK"; fi
fi

if want_group t13 fs; then
echo "[t13] fsinfo REPL command"
rm -f build/fsinfo.out
# 'fsinfo\n' is 7 bytes — one write is fine per FIFO rule
( sleep 4; printf 'fsinfo\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/fsinfo.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "AIkFS1" build/fsinfo.out; then ok "t13 fsinfo magic"; else bad "t13 fsinfo magic"; fi
if grep -q "v1" build/fsinfo.out; then ok "t13 fsinfo version"; else bad "t13 fsinfo version"; fi
fi

if want_group t14 fs; then
echo "[t14] ls REPL command"
rm -f build/ls.out
# 'ls\n' is 3 bytes — one write is fine per FIFO rule
( sleep 4; printf 'ls\n'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/ls.out 2>/dev/null &
QPID=$!
sleep 8
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "bin" build/ls.out; then ok "t14 ls shows bin"; else bad "t14 ls shows bin"; fi
if grep -q "hello.elf" build/ls.out; then ok "t14 ls shows hello.elf"; else bad "t14 ls shows hello.elf"; fi
fi

if want_group t15 ring3; then
echo "[t15] runelf REPL command"
rm -f build/runelf.out
# 'runelf bin/hello.elf\n' is 19 bytes — exceeds 16-byte FIFO (war story #6)
# chunk into 'runelf bin/hel' (13) + 'lo.elf\n' (8) with gap — matches t9's 13+5 pattern
( sleep 4; printf 'runelf bin/hel'; sleep 0.3; printf 'lo.elf\n'; sleep 4 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/runelf.out 2>/dev/null &
QPID=$!
sleep 10
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "hello from /bin/hello" build/runelf.out; then ok "t15 runelf hello output"; else bad "t15 runelf hello output"; fi
if grep -q "back in kernel" build/runelf.out; then ok "t15 runelf kernel survives"; else bad "t15 runelf kernel survives"; fi
fi

if want_group t16 mm; then
echo "[t16] memmap REPL command"
rm -f build/memmap.out
# 'memmap\n' is 7 bytes — safe as a single write
( sleep 4; printf 'memmap\n'; sleep 1 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/memmap.out 2>/dev/null &
QPID=$!
sleep 6
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "E820 memory map" build/memmap.out; then ok "t16 memmap header"; else bad "t16 memmap header"; fi
if grep -q "type=1" build/memmap.out; then ok "t16 memmap usable type"; else bad "t16 memmap usable type"; fi
fi

if want_group t19 syscall; then
echo "[t19] runelf opentest.elf (open syscall)"
rm -f build/opentest.out
# 'runelf bin/opentest.elf\n' is 23 bytes — chunk into 12 + 11 with gap
( sleep 4; printf 'runelf bin/ope'; sleep 0.3; printf 'ntest.elf\n'; sleep 4 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/opentest.out 2>/dev/null &
QPID=$!
sleep 10
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "open:3" build/opentest.out; then ok "t19 open syscall returns fd 3"; else bad "t19 open syscall returns fd 3"; fi
if grep -q "user exited" build/opentest.out; then ok "t19 opentest exited"; else bad "t19 opentest exited"; fi
fi

if want_group t20 syscall; then
echo "[t20] runelf closetest.elf (close syscall)"
rm -f build/closetest.out
# 'runelf bin/closetest.elf\n' is 24 bytes — chunk into 14 + 10 with gap
( sleep 4; printf 'runelf bin/clo'; sleep 0.3; printf 'setest.elf\n'; sleep 4 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/closetest.out 2>/dev/null &
QPID=$!
sleep 10
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "close:0" build/closetest.out; then ok "t20 close syscall returns 0"; else bad "t20 close syscall returns 0"; fi
if grep -q "back in kernel" build/closetest.out; then ok "t20 closetest kernel survives"; else bad "t20 closetest kernel survives"; fi
fi

echo "[t17] ADR-014 contract validator (host-side, no QEMU)"
rm -f build/contracts.out
python tools/check_contracts.py > build/contracts.out 2>&1
if grep -q "contracts OK" build/contracts.out; then ok "t17 contract validator"; else bad "t17 contract validator"; fi

if want_group t18 syscall; then
echo "[t18] readtest ELF user program (syscall 3 read)"
rm -f build/readtest.out
# 'runelf bin/readtest.elf\n' is 24 bytes — exceeds 16-byte FIFO; chunk into
# two bursts (14 + 10) with a gap (war story #6: multi-burst drops under load)
( sleep 4; printf 'runelf bin/rea'; sleep 0.5; printf 'dtest.elf\n'; sleep 2; printf 'Z'; sleep 3 ) | \
    "$QEMU" -drive file=build/disk.img,format=raw -serial stdio -display none \
    -no-reboot -m 32M > build/readtest.out 2>/dev/null &
QPID=$!
sleep 11
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true
if grep -q "read:Z" build/readtest.out; then ok "t18 readtest output read:Z"; else bad "t18 readtest output read:Z"; fi
if grep -q "back in kernel" build/readtest.out; then ok "t18 readtest kernel survives"; else bad "t18 readtest kernel survives"; fi
fi

echo
echo "RESULT: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
