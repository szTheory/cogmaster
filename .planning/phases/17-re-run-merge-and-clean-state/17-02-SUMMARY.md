---
phase: 17-re-run-merge-and-clean-state
plan: 02
subsystem: repository-tooling
tags: [milestone-close, gh, posix-sh, fake-gh-test, maintainer-docs]

requires:
  - phase: 17-re-run-merge-and-clean-state
    provides: plan 01 left the tree on the branch with docs.budgets passing, so this plan edits docs/maintaining.md after it
provides:
  - scripts/milestone-close.sh, a read-only POSIX sh check that prints six rows and exits 0, 1 or 2
  - policy.milestone_close, one CTest name driving eleven cases over a throwaway repository and a fake gh
  - the Closing a milestone paragraph in docs/maintaining.md
affects: [17-03, 17-04]

plan_head_before: 54aac4ce013ad14b3e642be9807bd0b928e71691
plan_head_after: b72a113801987a8ffd2bf95d7d0f86f626a2b965
commits: 4

actuals:
  tokens: 4256
  tasks: 3
  commits: 4

tech-stack:
  added: []
  patterns:
    - every gh read is v=$(gh ...) || v=unknown on one line, never piped, so a failed call becomes an unknown row
    - a fake gh outside the throwaway repository answers each endpoint already filtered, and refuses a request it does not know

key-files:
  created:
    - scripts/milestone-close.sh
  modified:
    - tests/policy/tooling_cases.cmake
    - tests/policy/repository-checks.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/maintaining.md

key-decisions:
  - "The script prints its rows in the order open-prs, main-ci, release, release-pr, tree, unreleased and the last line is close-check: all ok or close-check: N off, M unknown, with exit 0, 1 or 2"
  - "Every gh call sits on one line as v=$(gh ...) || v=unknown, so a failed or rate-limited call is an unknown row and exit 2, never a green one"
  - "The fake gh answers already-filtered text and exits 1 for an unknown path, so a request the script adds without an answer turns the baseline into unknown instead of passing"
  - "The script stays self-contained and does not count release assets; the docs point at the existing 19-asset check"

requirements-completed:
  - TUNE-17

coverage:
  - id: D1
    description: "the case loop: baseline exits 0 with close-check: all ok, one negative control per remote row family exits 1 with its row off, and the in-progress check run and the failing gh exit 2 with the row unknown"
    requirement: TUNE-17
    verification:
      - kind: integration
        ref: "ctest --preset ci -R '^policy\\.milestone_close$' (eleven cases, each ok)"
        status: pass
    human_judgment: false
  - id: D2
    description: "the dirty-tree controls: a tracked edit and an untracked file each exit 1 with the tree row off"
    requirement: TUNE-17
    verification:
      - kind: integration
        ref: "ctest --preset ci -R '^policy\\.milestone_close$' (cases tracked and untracked)"
        status: pass
    human_judgment: false
  - id: D3
    description: "the hand-run negative control: with untracked files ignored in the tree row the untracked case fails, and the restored script passes again"
    requirement: TUNE-17
    verification:
      - kind: other
        ref: "ctest --preset ci -R '^policy\\.milestone_close$' exited 8 naming case untracked with the line mutated, then 0 after the restore"
        status: pass
    human_judgment: false
  - id: D4
    description: "the script is classified operation with a reason that does not claim it cannot fail, and the new test name is in both inventories"
    requirement: TUNE-17
    verification:
      - kind: integration
        ref: "ctest --preset ci -R '^docs\\.budgets$' and the configure-time inventory comparison"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/maintaining.md carries the Closing a milestone paragraph: run from main after the last release pull request merges, a check to run and not a release gate, an off or unknown row is re-run"
    requirement: TUNE-17
    verification:
      - kind: other
        ref: "grep -q 'milestone-close.sh' docs/maintaining.md, grep -Eiq 'not a release gate|not a gate' docs/maintaining.md and wc -c under 38912, each exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "one live run on clean main printed all six rows with the tree row ok and exited 0"
    requirement: TUNE-17
    verification:
      - kind: other
        ref: "sh -c 'cd \"$1\" && exec sh \"$2\"' sh \"$wt\" \"$PWD/scripts/milestone-close.sh\" > \"$o\" exited 0, all six rows ok, last line close-check: all ok"
        status: pass
    human_judgment: false
  - id: D7
    description: "the ci and hygiene workflows pass with the new test, classification and docs, and no workflow names the script"
    requirement: TUNE-17
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (738 of 738, then 5 of 5); cmake --workflow --preset hygiene exit 0 on the rerun (84 of 84); sh scripts/hygiene.sh --tree exit 0; git grep -n milestone-close -- .github finds nothing"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-14
status: complete
---

# Phase 17 Plan 02: Milestone-close check Summary

**`scripts/milestone-close.sh` reads open pull requests, `CI required` on `main` and on the latest tag, the manifest against the latest release and its drafts, a merged release pull request still pending, the work tree and unreleased `feat`/`fix`/`perf` commits, prints one row each, and exits 0, 1 or 2; a fake-gh test with eleven cases proves every exit status, and one live run on clean `main` printed six ok rows.**

## Performance

- **Duration:** 10 min
- **Started:** 2026-11-14T17:23:00Z
- **Completed:** 2026-11-14T17:33:00Z (about)
- **Tasks:** 3
- **Files modified:** 7 (6 changed by the task commits, plus this summary; 215 insertions and 9 deletions)

## Accomplishments

- `scripts/milestone-close.sh` (78 code lines, 102 with the header) is POSIX sh under `set -eu`. Its six rows are `open-prs`, `main-ci`, `release`, `release-pr`, `tree` and `unreleased`; the last line is `close-check: all ok` or `close-check: N off, M unknown`. Exit 0 means every row is ok, 1 any row off, 2 none off but one unreadable. Reasons hold counts and 7-character shas only; the script holds no URL.
- Nine `gh` calls, each `v=$(gh ...) || v=unknown` on one line, never piped; `CI required` is read with the exact `check-runs?check_name=CI%20required` filter on the `main` sha and on the tag, the manifest with `git show origin/main:.release-please-manifest.json`, drafts with `gh api --paginate`, a pending release pull request with the search endpoint, and releasable commits from the compare endpoint.
- `policy.milestone_close` (label `policy`, 60 s) builds a throwaway repository with a copy of the script, a notes file and the manifest at `0.1.0`, a remote-tracking `main`, and a fake `gh` outside it. A driver loops eleven cases: `baseline`, `tracked`, `untracked`, `open_pr`, `red_ci`, `release_tag`, `draft`, `release_pr`, `unreleased`, `pending`, `offline`. The test file has no `jq`.
- The script is classified `operation` with a reason that does not say it never fails; the test name is in `tests/inventory/ci.txt` and `tests/inventory/wasm32.txt`; the script is in `CMAKE_CONFIGURE_DEPENDS` so an edit refreshes the copy.
- `docs/maintaining.md` (20,537 to 21,432 bytes, cap 38,912) has a "Closing a milestone" paragraph after "Closing a phase".
- Nothing under `src/`, `include/` or `.github/` changed.

## Commands and exit statuses

All run from the repository root except the live run's one child shell (the stated working-directory exception). Nothing was pushed in this plan, so there is no run URL; the lane is plan 03's push.

| Command | Exit |
|---|---|
| `cmake --preset ci` then `cmake --build --preset ci` | 0 |
| `ctest --preset ci -R '^(policy\.milestone_close\|docs\.budgets)$' --output-on-failure --no-tests=error` (the tracer gate, three cases then) | 0 |
| the same test after the eight remote cases were added and before the rows existed (RED) | 8 (eight cases failed: no such row) |
| the same command after the rows were added (GREEN, eleven cases) | 0 |
| hand-run negative control: `--untracked-files=no` in the tree row, `ctest --preset ci -R '^policy\.milestone_close$'` | 8 (case `untracked` FAILED 0) |
| the same command after restoring the line | 0 |
| Task 1 acceptance (six greps and `sh -n`) and Task 2 acceptance (eight greps and awk checks) and Task 3 acceptance (five greps) | 0 each |
| `git fetch origin main:main` (local `main` f8c0f4e to 0a97075, a ref update only, no commit on `main`) | 0 |
| `test "$(git rev-parse main)" = "$(git rev-parse origin/main)"` | 0 |
| `git worktree add "$wt" main`, then `git -C "$wt" status --porcelain` empty, then `git -C "$wt" symbolic-ref --short HEAD` equal to `main` | 0, 0, 0 |
| live run: `sh -c 'cd "$1" && exec sh "$2"' sh "$wt" "$PWD/scripts/milestone-close.sh" > "$o"` | 0 |
| live acceptance: every row name printed, `tree` ok, last line well formed | 0 |
| `git worktree remove --force "$wt"` then `git worktree prune` | 0, 0 |
| `cmake --workflow --preset ci` (738 of 738, then 5 of 5) | 0 |
| `cmake --workflow --preset hygiene` | 8 first run (`hygiene.tree` timed out at 60 s under host load), 0 on the rerun (84 of 84) |
| `sh scripts/hygiene.sh --tree` | 0 |
| `git grep -n milestone-close -- .github` | 1 (no match, as required) |

## Live run on clean main

Local `main` equalled `origin/main` at `0a97075`, the worktree was clean and on the `main` branch. Six rows printed: `open-prs` ok, `main-ci` ok, `release` ok, `release-pr` ok, `tree` ok, `unreleased` ok; last line `close-check: all ok`, exit 0. Reasons read: `0 open`, `CI required success at 0a97075`, `v0.15.0, no draft, CI success`, `none pending`, `main at 0a97075`, `none since v0.15.0`. Because no row was off or unknown there was nothing to triage, so each remote row was checked by hand instead with plain reads: `gh pr list --state open` counts 0, `gh release view` shows `v0.15.0` with `isDraft` false, the `CI required` check run filter on the `main` sha answers `success`, the search for a merged `autorelease: pending` pull request counts 0, and the compare filter counts 0 from `v0.15.0` and 1 from `v0.14.2` (a positive control for the `feat`/`fix`/`perf` pattern). The filters were also run once on sample JSON with a local `jq` (counts 3, draft id 2, `success`, `in_progress`, `null`); that `jq` is not in the test file.

## Task Commits

1. **Task 1: tree row end to end (tracer)** - `c787726` (feat)
2. **Task 2, RED: cases for the remote rows and the two unknown controls** - `d28d35e` (test)
3. **Task 2, GREEN: the five remote rows** - `1cb98a1` (feat)
4. **Task 3: the maintainer paragraph** - `b72a113` (docs)

**Plan metadata:** the commit that carries this file (docs: complete plan).

## TDD notes (Task 2)

- **RED:** `policy.milestone_close`, with the fake gh extended and eight cases added. Baseline, tracked and untracked still passed; the eight new cases failed with the script printing no such row (`open_pr` through `unreleased` and `pending` exited 0 with `close-check: all ok`, `offline` exited 2 with the wrong row). That is the planned reason. The ctest output is outside the formats the RED classifier reads and `workflow.tdd_mode` is off for this project, so the evidence is the unmodified ctest report and this semantic assessment.
- **GREEN:** the rows were added to the script and all eleven cases passed.
- **REFACTOR:** none needed.

## Files Created/Modified

- `scripts/milestone-close.sh` - the check: six rows, summary line, exit 0, 1 or 2
- `tests/policy/tooling_cases.cmake` - the throwaway repository, the fake `gh`, the case driver, the `policy.milestone_close` registration and the script in `CMAKE_CONFIGURE_DEPENDS`
- `tests/policy/repository-checks.txt` - the `operation` row for the script, and the `tooling_cases.cmake` reason now names the milestone-close controls
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - one `policy.milestone_close` line each
- `docs/maintaining.md` - the Closing a milestone paragraph

## Decisions Made

- Followed the plan's shape. The script takes no options, stays self-contained (no function from `compat-common.sh`) and does not count release assets; the docs say the two existing checks above still apply.
- The `release` row reads the manifest in two steps (`git show`, then `sed` on the variable) so a failed read is `unknown`, and a reason for an unreadable release lists the pieces (`latest`, manifest, drafts, tag CI) so a re-run has something to look at.
- The compare filter takes the first line of each commit message before matching, so a body line that begins `fix:` cannot count as a releasable commit under an engine where `^` also matches after a newline.

## Deviations from Plan

### Auto-fixed Issues

None. These are variances, not fixes:

**1. [Variance] Script length over the planned size**
- **Found during:** Task 2
- **Issue:** the plan estimated about 45 to 60 lines; the script is 102 lines (78 code lines, 15 header lines, the rest blank).
- **Fix:** none. Nothing in the acceptance criteria or in a budget caps it (the script is class `operation`, so the check-line budget does not count it); the length is the header that names the rows and the fault, two small helpers (`verdict`, `count`) and the per-row reasons.
- **Files modified:** scripts/milestone-close.sh
- **Commit:** 1cb98a1

**2. [Variance] Task 2 split into a RED and a GREEN commit**
- **Found during:** Task 2
- **Issue:** the task is `tdd="true"`, which calls for a failing test commit first; the plan counted tasks, not commits (as in plan 01).
- **Fix:** `d28d35e` holds the cases and the fake gh answers, `1cb98a1` holds the rows.

**3. [Variance] hygiene.tree timed out once under host load**
- **Found during:** Task 3 (`cmake --workflow --preset hygiene`)
- **Issue:** `hygiene.tree` hit its 60 s limit on the first run (exit 8). Host load average was 37 from processes of another project and a virtual machine; run alone the test took 45 s, a direct `sh scripts/hygiene.sh --tree` took 47 s and exited 0, and the workflow rerun passed 84 of 84. This is the same load-bound margin the 16-04 note records and the re-derivation of that limit belongs to the timeout plan (the `hygiene.tree` value is on its list), so no limit was touched here.
- **Fix:** none; a rerun of the unchanged tree.

---

**Total deviations:** 0 auto-fixed, 3 variances noted
**Impact on plan:** none. Every acceptance criterion passed as written.

## Issues Encountered

- The first hygiene workflow run exited 8 on the `hygiene.tree` timeout described above; the rerun exited 0.
- An ad-hoc compare command of mine combined `length` with `.total_commits` in one filter and exited 1 on my own filter error; the script's filter ends at `length` and was run clean right after, so the script was not affected.

## Known Stubs

None.

## Threat Flags

None. The script reads GitHub through the signed-in `gh` and adds no endpoint, auth path or write request to the product. The mitigations in the plan hold: a failed call is an unknown row and exit 2 (the `offline` case), reasons carry counts and short shas and no URL, and the read-only grep over the script finds no write method, fetch, push or URL.

## For the owner

Nothing is needed from you for this plan. `milestone-close.sh` is not wired into any workflow or schedule; run it by hand after the last release pull request of a milestone merges. Plan 03 pushes plans 01 and 02 once and then asks for the judge and diagnose runs.

## Next Phase Readiness

- Plan 03 can push this branch head; the new test is a `ci`-lane test (738 of 738 locally) and the `policy.milestone_close` inventory lines are in both lists.
- The local `main` ref now equals `origin/main` (0a97075) because of the fast-forward; nothing was committed on `main`.

## Self-Check: PASSED

- Files found: `scripts/milestone-close.sh`, `tests/policy/tooling_cases.cmake`, `tests/policy/repository-checks.txt`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`, `docs/maintaining.md`.
- Commits found as ancestors of HEAD: `c787726`, `d28d35e`, `1cb98a1`, `b72a113`; the evaluation-scope resolver lists them for plan 17-02.

---
*Phase: 17-re-run-merge-and-clean-state*
*Completed: 2026-11-14*
