# How to debug AIkOS

> **Status:** placeholder — no code to debug yet (2026-08-05).

The debugging playbook will live here: serial printf patterns, QEMU monitor commands, gdb stub usage, and — most importantly — **every non-obvious bug we defeat gets written down here with its root cause**, so the next session doesn't fight the same ghost twice.

Planned sections:

- Debug loop: build → boot in QEMU → read serial → inspect with monitor → fix → repeat
- Common boot failures and their signatures (triple faults, missing GDT, page faults, ...)
- The "blinking cursor of death" decision tree
- War stories (one entry per hard-won bug — what happened, what it turned out to be, how we found it)
