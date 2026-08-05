# ADR-004: GitHub workflow — releases per phase, no issues/PRs for now

- **Status:** Accepted (2026-08-05)
- **Supersedes:** nothing
- **Superseded by:** nothing

## Context

The repo lives at https://github.com/qazW12345/AIkOS (private, backup + versioning). Question raised: which GitHub features (Issues, Pull Requests, Releases, ...) do we actually use?

Constraints from ADR-001: docs-as-code — the source of truth lives *in the repo*; no second, unversioned tracking system. Project reality: single-writer (AIko + Marcel), no external contributors.

## Decision

- **Releases: yes.** Each phase exit = a git tag (`v0.1.0`, `v0.2.0`, ...) + a GitHub Release carrying artifacts (kernel image, ISO) once they exist. A phase is not done until it is tagged. This makes the exit-criteria discipline externally visible and gives Marcel downloadable artifacts for real-hardware testing (Phase 7).
- **Issues: no, for now.** TaskLog.md + Roadmap.md are the issue tracker — versioned, session-readable, and already the handoff mechanism. GitHub Issues would create a second source of truth that doesn't travel with the repo.
- **Pull requests: no, for now.** Single-writer; branch + merge in git is available if we ever want structured history. If Marcel ever wants to review-before-merge, we adopt PRs then.
- **CI (GitHub Actions): deferred.** Linux runners ship nasm/gcc/qemu — an automated build + boot-test on every push is genuinely attractive (it would close our ad-hoc-verification gap with a canonical test). Deferred until Phase 0 stabilizes; adoption itself becomes an ADR.
- **Wiki: no.** `Documentation/` is the wiki, versioned.
- Root **README.md** exists as the human entry point; AGENTS.md is the agent entry point; `Documentation/README.md` is the deep router.

## Consequences

**Positive:**
- Public progress milestones with zero ceremony; artifacts flow naturally to hardware testing.
- No dual-source-of-truth; every tracking artifact survives a machine failure (it's all in git).
- The repo now has all three entry points (README/AGENTS/Documentation router) that any future contributor — human or model — needs.

**Negative / costs:**
- No issue tracker means task conversations live in the TaskLog instead — fine for us, friction if the project ever gains outside contributors (revisit then).
- Releases without CI are manual tags — acceptable at this scale.
