---
phase: 15-re-run-merge-and-timeouts
fixed_at: 2026-11-13T20:39:53Z
review_path: .planning/phases/15-re-run-merge-and-timeouts/15-REVIEW.md
iteration: 1
findings_in_scope: 4
fixed: 3
skipped: 1
status: partial
---

# Phase 15: Code Review Fix Report

**Fixed at:** 2026-11-13T20:39:53Z
**Source review:** .planning/phases/15-re-run-merge-and-timeouts/15-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 4 (fix scope `critical_warning`: WR-01, WR-03, WR-04, WR-05)
- Fixed: 3
- Skipped: 1

WR-02 is not in scope. It carries `**Category:** necessity`, and removing code is a scope decision for the owner, so
it was left untouched. It stays open in the review report.

**Verification ran in the main checkout** (`workflow.use_worktrees` is `false`, so no worktree was created). After
each edit: the workflow parsed as YAML, `cmake -DDIR=$PWD/.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake`
and the same with `-DCHECK=action_pins` exited 0. After the last commit: `sh scripts/hygiene.sh --tree` and
`sh scripts/hygiene.sh --history cb33771..HEAD` exited 0, the docs budget script exited 0, and
`ctest -R '^(hygiene|docs|policy)\.'` in `build/ci` passed 59 of 59. The full `cmake --workflow --preset ci` was not
run.

## Fixed Issues

### WR-01: Fuzz job is still `continue-on-error` after its stated exit condition was met

**Files modified:** `.github/workflows/nightly.yml`, `docs/perf-fuzz.md`
**Commit:** 34b464a
**Status:** fixed: requires human verification
**Applied fix:** Re-read the live API first: all 24 scheduled Nightly runs that include the fuzz job have it
`success` with no failed step, so the stated exit condition (ten green runs) is met. Deleted the stale comment and
`continue-on-error: true` from the `fuzz` job. Rewrote the sentence in `docs/perf-fuzz.md` so it says a finding fails
the night and opens or updates the nightly issue, still never a release gate of its own. The behaviour change shows
only on a run: the first fuzz finding after this merges will open the nightly issue.

### WR-03: Nightly runs on pull requests with no concurrency group, and overlaps CI's macOS jobs

**Files modified:** `.github/workflows/nightly.yml`, `docs/maintaining.md`
**Commit:** 01d5632
**Status:** fixed: requires human verification
**Applied fix:** Added a workflow-level `concurrency` block with group
`nightly-${{ github.event.pull_request.number || github.run_id }}` and `cancel-in-progress` set only for
`pull_request` events, with a short comment in the style of the one in `ci.yml`. A scheduled or dispatched run has a
group of its own and is never cancelled. Added one sentence to the CI speed paragraph in `docs/maintaining.md`: a
change to `nightly.yml`, the fuzz, z80 or vector tests or the bench sources also runs Nightly, macOS bench included,
and a later push cancels the earlier Nightly run. The pull request for this phase edits `nightly.yml`, so its own
Nightly run is the first test of the group expression.

### WR-04: Nightly `apt-get` calls have no retries; every `ci.yml` call has them

**Files modified:** `.github/workflows/nightly.yml`
**Commit:** 2eecf46
**Applied fix:** Both `apt-get` sites (the `bench-wasm32` WASI step and the `fuzz` runtime step) now pass
`-o Acquire::Retries=3` to `update` and to `install`, exactly as the four sites in `ci.yml` do.

## Skipped Issues

### WR-05: The new ledger stamp names an unpushed, docs-only commit that a squash merge will leave outside `main`

**File:** `tests/compat/ledger.csv`, `docs/compatibility.md`
**Reason:** The finding is a state of the remote, not of the files. Commit 0b4ae67 is not on any remote branch, and
publishing it needs a push, which this run is not allowed to do. The ledger must not be re-stamped. The review's own
fix is to run the post-push `gh api repos/<o>/<r>/commits/<sha>` check at ship, then add one sentence to
`docs/conformance.md#whole-list-ledger` saying a `core=` stamp from a phase branch resolves through
`git fetch origin pull/<n>/head` and that its tree matches the merge commit outside `.planning/`. Both steps belong
to the ship step, after the push. For later re-runs, judge a commit already on `main`.
**Original issue:** The ledger's 1116 evidence cells carry `core=0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4`, a docs-only
commit on the phase branch. It is not an ancestor of `origin/main` and, under a squash merge, never will be, so a
fresh clone cannot resolve the stamp without fetching the pull request ref.

---

_Fixed: 2026-11-13T20:39:53Z_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
