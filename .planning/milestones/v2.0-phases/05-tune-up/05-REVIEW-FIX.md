---
phase: 05-tune-up
fixed_at: 2026-11-10T00:00:00Z
review_path: .planning/phases/05-tune-up/05-REVIEW.md
iteration: 1
findings_in_scope: 12
fixed: 3
skipped: 9
status: partial
---

# Phase 5: Code Review Fix Report

**Fixed at:** 2026-11-10
**Source review:** `.planning/phases/05-tune-up/05-REVIEW.md`
**Iteration:** 1

**Summary:**
- Findings in the review: 12 (0 critical, 5 warning, 7 info)
- Fixed: 3 (WR-01, WR-03, WR-04), as selected by the orchestrator
- Deferred to `.planning/seeds/SEED-007-milestone-tune-up.md` under AGENTS rule 7: 9 (WR-02, WR-05, IN-01 to IN-07)

Edits were made in the main checkout (`workflow.use_worktrees` is false), on `phase/05-tune-up`.

## Fixed Issues

### WR-01: Dependabot auto-merges release-pipeline action pins with the rehearsal skipped

**Files modified:** `.github/workflows/dependabot-automerge.yml`, `tests/cmake/repository.sh`
**Commit:** 50eb5bd
**Applied fix:** Before `gh pr merge --auto --squash`, the job reads `gh pr diff --name-only` into a file in
`$RUNNER_TEMP` and does not merge, logging the reason, when the list is unreadable, empty, or contains a release input.
The list is the one in `scripts/classify-changes.sh`: `.github/workflows/release.yml`, `.github/workflows/ci.yml`,
`scripts/publish.sh`, `release-please-config.json`, `cmake/packaging.cmake`. The `dependabot` repository check now also
requires the `gh pr diff ... --name-only` call (appended to existing awk lines, no new counted lines). Because `ci.yml`
also holds the build pins, those bumps are no longer auto-merged either; this follows `release_inputs`.
**Verification:**
- `ctest --test-dir build/ci -R 'policy\.dependabot'`: exit 0, both tests passed.
- `actionlint -ignore 'label "ubuntu-26.04' -ignore 'unexpected key "queue"' .github/workflows/dependabot-automerge.yml`: exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, 729 check lines (budget unchanged).
- Status: fixed, requires human verification (the first real Dependabot pull request exercises the branch; the
  merge path is untestable locally).

### WR-03: Runner's size refusal message now states a range that does not load

**Files modified:** `runner/main.c`
**Commits:** a5af131, 673d9be (the second rewraps the string to the 100-column `.clang-format` limit that
`hygiene.format` enforces)
**Applied fix:** The message now reads "the body after any 512-byte copier header must be 1024 bytes to 1 MiB",
matching `docs/runner.md` and `src/cart.c`. It is a message string, not a comment; no test matches its text.
**Verification:**
- `cmake --build build/ci --target cogmaster-run` and `ctest --test-dir build/ci -R runner`: exit 0, 63 of 63 passed.
- `ctest --test-dir build/hygiene -R hygiene.format`: exit 0 after the rewrap (it failed once on the first commit).

### WR-04: Untrusted pull request titles are written verbatim into a public issue body

**Files modified:** `scripts/digest.sh`, `tests/hygiene/fixtures/digest.json`, `tests/hygiene/tests.cmake`
**Commit:** 71ffdc9
**Applied fix:** A `plain` jq function puts each pull request title inside a code span after removing `@`, backticks
and line breaks, so a title cannot mention a user or render a link (a code span is not autolinked). The fixture gains
pull request 9 titled with an `@name`, a URL and a backtick, and `digest.format` asserts the exact neutralised line and
that no `@name` appears. Nightly issue titles are produced by our own workflow and were left as they are.
**Verification:**
- `ctest --test-dir build/hygiene -R digest`: exit 0, 4 of 4 passed (`digest.timeouts`, `digest.format`,
  `policy.digest`, `policy.digest.self_test`).
- With the old `scripts/digest.sh` stashed, `digest.format` failed (0% passed), so the new case catches the fault.

## Deferred (seeds, not fixed)

Appended to `.planning/seeds/SEED-007-milestone-tune-up.md` (commit 1678f45) under AGENTS rule 7: review findings
about test or tooling code become seeds.

| Finding | Reason deferred |
|---------|-----------------|
| WR-02 | `check-title.sh` against Dependabot bodies containing `[skip ci]`; not reproduced, no Dependabot pull request exists yet |
| WR-05 | The size ratchet runs `quality.cmake` from the candidate tree (tooling) |
| IN-01 | A null ceiling or floor in `quality.json` silently disables the gate |
| IN-02 | Digest prints "runs,10.8" with no space after the comma |
| IN-03 | `budgets.cmake` exclusive against `quality.cmake` inclusive docs cap; README and header literal caps |
| IN-04 | `compare.c` gets `stdint.h` through `play.h`; `--flip` is a no-op on small images |
| IN-05 | `repository.sh` misses inline `permissions: write-all` and a required `on: pull_request` |
| IN-06 | Digest `gh list` default limit of 30 and a fragile `timeouts` awk |
| IN-07 | Auto-merge treats 0.x minors as safe and has no `--match-head-commit` |

## Gates

All run from the repository root in the main checkout (not an isolated worktree), after the last fix commit
(673d9be):

| Command | Exit status |
|---------|-------------|
| `cmake --workflow --preset hygiene` | 0 (48 of 48 passed; one earlier run exited 8 on `hygiene.format`, fixed by 673d9be) |
| `cmake --workflow --preset ci` | 0 |
| `actionlint -ignore 'label "ubuntu-26.04' -ignore 'unexpected key "queue"' .github/workflows/dependabot-automerge.yml` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (729 check lines, 7455 src lines) |

---

_Fixed: 2026-11-10_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
