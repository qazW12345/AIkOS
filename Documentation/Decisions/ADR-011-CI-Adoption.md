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

## Update (2026-08-05)

CI grew without changing the core decision:

- **paths-filter** (`dorny/paths-filter`, SHA-pinned): the QEMU suite runs only when code changes (`src/**`, `user/**`, `test.sh`, `build.sh`, `.github/**`). Docs-only pushes skip the ~3–4 min suite — the "every push" clause above is now "every push that touches code" (ADR-011 intent preserved; docs cannot break the kernel).
- **super-linter** (SHA-pinned): shellcheck, markdownlint (house config `.github/linters/.markdown-lint.yml`), yamllint, gitleaks, checkov, zizmor, codespell. Formatter linters, textlint, and python linters are disabled by policy (one throwaway `tools/ppm2png.py`; revisit when real python tooling lands).
- **Supply-chain hardening:** all actions pinned to full SHAs (`owner/repo@sha`), least-privilege workflow permissions, `persist-credentials: false`.
- **actions/cache verdict:** NOT adopted — the workflow has no package-manager dependency tree to cache (freestanding C + nasm; toolchain via apt, ~1 min); the dominant CI cost is QEMU wall-clock test time, which caching cannot reduce. Revisit when Phase 3+ userland tooling introduces dependency downloads (npm/pip/cargo-style), or if CI time analysis ever shows container-image pulls dominating.

## Future considerations

- **Revisit `sdras/awesome-actions` when AIkOS becomes significantly bigger** (userland tooling, artifact/release automation, complex multi-job workflows) — it is a curated index of actions; today our CI needs are fully covered by the two actions above.
- Real-hardware boot tests (Phase 7) will extend this workflow.
