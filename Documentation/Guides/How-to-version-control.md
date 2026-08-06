# How to version-control AIkOS (and roll back)

> **Status:** current (2026-08-05). Git is the backup AND the time machine — every commit is a restore point, mirrored on GitHub.

## Golden rule

**Commit before risky changes.** The working tree is your sandbox; a commit is insurance. If a change breaks something, you never lose the working state.

## Everyday rollback commands

```bash
# See what changed recently (this IS the backup log)
git log --oneline

# Restore ONE file to an older version
git log -- src/kernel/kmain.c          # find the commit sha
git checkout <sha> -- src/kernel/kmain.c

# Undo the LAST commit's changes (keeps history, creates a new commit)
git revert HEAD

# Discard uncommitted changes in one file / everything
git checkout -- src/kernel/serial.c
git checkout -- .

# Wipe back to the last commit entirely (careful — destroys uncommitted work)
git reset --hard HEAD
```

## Known-good checkpoints (tags)

When something is verified working (e.g., `test.sh` green), give it a name — tags are permanent bookmarks that survive on GitHub:

```bash
git tag known-good-20260805            # lightweight bookmark
git push origin known-good-20260805    # back it up
git checkout known-good-20260805 -- src/   # roll a directory back to it
```

Phase milestones become version tags per ADR-004: `v0.1.0` (Phase 0), `v0.2.0` (Phase 1), ... each with a GitHub Release carrying the working artifacts (disk.img, later ISO).

## Branches as sandboxes

For risky experiments that shouldn't touch the main line:

```bash
git checkout -b experiment-something   # work here
git checkout main                      # back to safety
git branch -D experiment-something     # throw it away (or merge if it worked)
```

## Backups

- The local repo is the working copy; **GitHub (private, qazW12345/AIkOS) is the off-machine backup** — push after every meaningful change: `git push`.
- Credentials are stored in Git Credential Manager — pushes are silent.
- Commits are authored as `AIko <aiko@aikos.local>` (see TaskLog; changeable with `git config user.name` if ever wanted).

## CI (ADR-011) — REMOVED 2026-08-06

GitHub Actions was retired: the suite runs **locally** (it always was the arbiter —
`./test.sh`), CI only added queue time and action-service outages (3 infrastructure
failures in one afternoon, zero real findings). Verification is now the merge-gate
discipline: **AIko runs `./test.sh` + `tools/lint-local.sh` (incl. gitleaks secrets
scan) before every merge**; the reviewer agent reviews each PR. GitHub's native
secret scanning stays as the passive backstop on the public repo. History (for the
record): workflow live since 2026-08-05, first green run `ddd9b17`; the original
PAT lacked `workflow` scope (pushes of `.github/workflows/` rejected) — resolved
with `read:org, repo, workflow`.
