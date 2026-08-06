#!/usr/bin/env bash
# test_plan.sh — suggest the minimal suite subset for a change (2026-08-06).
# Usage:  tools/test_plan.sh <changed file...>
# Prints the ./test.sh command to run for that change.
# Rules (see test.sh header):
#   independence  — untouched subsystems' tests can't break -> skipped;
#   risk          — boot path / kernel.h / kmain.c changes ALWAYS need the
#                   full suite (the whole kernel depends on them).
set -u

FULL_RISK="src/boot src/kernel/entry.asm src/kernel/idt.c src/kernel/proc.c src/kernel/kmain.c src/kernel/kernel.h"

KW=""
for f in "$@"; do
    # full-risk first: any match wins over narrower keywords
    for r in $FULL_RISK; do
        case "$f" in "$r"*) KW="full";; esac
    done
    case " $KW " in *" full "*) continue;; esac
    case "$f" in
        src/kernel/syscall.c|src/kernel/fd.c)             KW="$KW syscall";;
        src/kernel/fs.c|tools/buildfs.py)                 KW="$KW fs";;
        src/kernel/buddy.c|src/kernel/mm.c|src/kernel/memmap.c) KW="$KW mm";;
        src/kernel/repl.c|src/kernel/hexdump.c)           KW="$KW debug";;
        src/kernel/elf.c|src/kernel/tss.c)                KW="$KW ring3";;
        user/*|src/kernel/elf.c)                          KW="$KW syscall";;
        Documentation/*|*.md|AGENTS.md)                   KW="$KW static";;
        *)                                                KW="$KW full";;
    esac
done

case " $KW " in
    *" full "*) echo "./test.sh                    # FULL (boot/kernel-wide change)";;
    "")         echo "./test.sh                    # no code touched — full or skip";;
    *)
        KW=$(echo "$KW" | tr ' ' '\n' | sort -u | tr '\n' ' ' | sed 's/ $//')
        echo "./test.sh $KW   # subset — core+static always included"
        ;;
esac
