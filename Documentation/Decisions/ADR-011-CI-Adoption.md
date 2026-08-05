# ADR-011: CI adoption — GitHub Actions

- **Status:** Accepted (2026-08-05)
- **Supersedes:** ADR-004's "CI: Deferred" clause
- **Superseded by:** nothing

## Context

ADR-004 deferred CI with the note: "genuinely attractive — Linux runners ship nasm/gcc/qemu, could build *and boot-test* on every push, closing the ad-hoc-verification gap; adoption becomes its own ADR once Phase 0 stabilizes."

Two phases are now green (v0.1.0, v0.2.0) and `test.sh` is the canonical suite (4 tests, 10 checks). The recurring cost of *not* having CI is visible: every code change requires a manual verification ritual. The stabilization condition is met.

## Decision

- **GitHub Actions workflow** `.github/workflows/build.yml` on every push: `ubuntu-latest`, install `nasm clang lld llvm qemu-system-x86`, run `./test.sh`.
- **Platform-aware toolchain paths** via a shared `env.sh` sourced by `build.sh` and `test.sh` (MINGW/git-bash absolute paths vs Linux package names; `python` vs `python3`).
- CI is informational for now (no required status checks, no release automation).

## Consequences

**Positive:**
- Every push builds AIkOS, boots it in QEMU, and runs the full gauntlet — regressions surface the moment they're pushed.
- The local ad-hoc verification ritual can retire for normal changes.
- Real-hardware boot tests (Phase 7) will one day extend this.

**Negative / costs:**
- Workflow + portability maintenance (small).
- GitHub-hosted runners only (no caching of toolchain — ~1 min install per run, acceptable).
