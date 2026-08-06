#!/usr/bin/env bash
# qemu_run.sh — run QEMU headless with serial capture, GUARANTEED to terminate.
#
# Usage: tools/qemu_run.sh [--in <input_file>] [--timeout N] [--out <file>] [qemu args...]
#   --in       file piped to the guest serial stdin (default: /dev/null)
#   --timeout  seconds to wait before force-killing (default: 25)
#   --out      serial capture file (default: build/serial.log)
#   remaining args pass through to QEMU (-drive etc.)
#
# Solves two multi-agent traps (ADR-018 / How-to-debug.md):
#   (a) THE STUCK PIPE — `( sleep; printf ... ) | qemu > out` never returns:
#       QEMU runs forever, so the pipeline never closes and the caller hangs.
#       Here input is a FILE redirect and QEMU is backgrounded, so nothing
#       blocks; the bounded wait + kill is the only exit path.
#   (b) IMAGE-NAME KILLS — `taskkill //F //IM qemu-system-x86_64.exe` kills
#       EVERY QEMU on the machine, including another agent's in-flight tests.
#       This helper kills by PID only.
set -u
cd "$(dirname "$0")/.." || exit 1

IN_CMD=""
TIMEOUT=25
OUT_FILE=build/serial.log
while [ $# -gt 0 ]; do
    case "$1" in
        --in-cmd) IN_CMD="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --out) OUT_FILE="$2"; shift 2 ;;
        *) break ;;
    esac
done

. ./env.sh
mkdir -p "$(dirname "$OUT_FILE")"

# --in-cmd: a bash command whose stdout feeds the guest serial stdin. Use the
# proven (sleep; printf) idiom with TIME GAPS between writes — the 16550 FIFO
# drops bursts >16 bytes (war story #6), so split long commands:
#   --in-cmd "sleep 4; printf 'hexdump 40000'; sleep 1; printf '0 8\n'"
# The pipeline is backgrounded so the CALLER never blocks on QEMU's endless
# run (the stuck-pipe trap); the bounded wait + PID kill below is the exit.
if [ -n "$IN_CMD" ]; then
    ( eval "$IN_CMD" ) | \
        "$QEMU" "$@" -serial stdio -display none -no-reboot -m 32M \
        > "$OUT_FILE" 2>/dev/null &
else
    "$QEMU" "$@" -serial stdio -display none -no-reboot -m 32M \
        > "$OUT_FILE" 2>/dev/null &
fi
QP=$!

# Wait up to TIMEOUT for a natural exit (guest halt/panic); then PID-kill.
for _ in $(seq 1 "$TIMEOUT"); do
    sleep 1
    if ! kill -0 "$QP" 2>/dev/null; then
        wait "$QP" 2>/dev/null
        exit 0
    fi
done

if kill -0 "$QP" 2>/dev/null; then
    # Windows: taskkill //F //PID (MSYS kill alone often cannot kill native).
    taskkill //F //PID "$QP" >/dev/null 2>&1 || kill -9 "$QP" 2>/dev/null
    wait "$QP" 2>/dev/null
    echo "qemu_run: killed after ${TIMEOUT}s (PID $QP)" >&2
fi
exit 0
