#!/usr/bin/env bash
# lint-local.sh — house style linters, run locally before pushing (2026-08-06).
# The CI lint job now runs SECURITY linters only (gitleaks/checkov/zizmor);
# style (markdownlint/codespell/shellcheck) lives here + the reviewer agent.
# Usage: bash tools/lint-local.sh   (from the repo root)
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
FAIL=0

echo "[local-lint] markdownlint (docs + briefs)..."
if command -v npx >/dev/null 2>&1; then
    npx --yes markdownlint-cli@0.44.0 "Documentation/**/*.md" README.md AGENTS.md \
        --config .github/linters/.markdown-lint.yml 2>/dev/null || FAIL=1
else
    echo "  (npx not found — skipped; install Node or let the reviewer check)"
fi

echo "[local-lint] codespell..."
if command -v uvx >/dev/null 2>&1; then
    uvx codespell . -S ".git,archive,build" 2>/dev/null || FAIL=1
else
    echo "  (uvx not found — skipped)"
fi

echo "[local-lint] shellcheck (test.sh, tools/*.sh)..."
if command -v shellcheck >/dev/null 2>&1; then
    shellcheck test.sh tools/*.sh 2>/dev/null || FAIL=1
else
    echo "  (shellcheck not found — skipped; run locally if you want the full net)"
fi

if [ "$FAIL" -eq 0 ]; then
    echo "[local-lint] OK — style clean"
else
    echo "[local-lint] FAILURES above — fix before pushing"
fi
exit "$FAIL"
