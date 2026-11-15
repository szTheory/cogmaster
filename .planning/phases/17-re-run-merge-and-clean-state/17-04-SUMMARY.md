---
phase: 17-re-run-merge-and-clean-state
plan: 04
subsystem: ci-timeouts
tags:
  - timeouts
  - ci
  - ctest
  - twice-the-slowest
  - ship-hand-off

requires:
  - phase: 17-re-run-merge-and-clean-state
    provides: plan 03 left the carried ledger and page committed on the branch, so the head that CI sampled equals the code that ships
provides:
  - one workflow commit (33bbfc2) that re-derives every timeout-minutes line and the CTest limits of library.diagnose and hygiene.tree from a fresh 40-run read, each citing a run in that window
  - the Timeouts and Ship hand-off sections of 17-RUN-EVIDENCE.md, with the limits considered, the lane-aware reading of runner.diagnose and the checks as commands and exit statuses
  - a Phase 17 timeout read bullet in SEED-015 for the readings that sit near their limits
affects:
  - ship

plan_head_before: 2cc8bb55027a116293a1707a583def093f416b90
plan_head_after: e7ac5e5e98e335acc78b736fedaec5a258d8a37e
commits: 2

actuals:
  tokens: 3821
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - a dispatched run on the pushed head with the old limits is the sample, never the pull request's own run
    - a CTest limit is compared with unscaled lane seconds: scaled lanes (asan 3, be32 8, wasm32 5) are divided by their scale before doubling

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - tests/library/tests.cmake
    - tests/hygiene/tests.cmake
    - .planning/phases/17-re-run-merge-and-clean-state/17-RUN-EVIDENCE.md
    - .planning/seeds/SEED-015-whole-list-follow-ups.md
    - .planning/STATE.md

key-decisions:
  - "The rehearsal job is the only job limit that rose (13 to 14 minutes, twice 393 s is 786 s); library.diagnose rose 206 to 289 s and hygiene.tree 60 to 75 s; every other timeout-minutes line kept its value and took the measured comment, and the perf job stays at 6 minutes"
  - "runner.diagnose holds at 633 s: the plan's rule compares unscaled lane seconds, the largest is 179.23 s on macos-x64 (twice is 358.46 s), and the 450.48 s reading is the be32 lane under a scale of 8"
  - "No push after the workflow commit: the ship step pushes once, which also starts the rehearsal job that a ci.yml edit triggers"

requirements-completed:
  - TUNE-16

coverage:
  - id: T1
    description: "the ledger head was pushed once and a dispatched ci run on it is green, so the sample is the shipped code under the old limits and the pull request's own run never sizes its own limits"
    requirement: TUNE-16
    verification:
      - kind: integration
        ref: "git push origin phase/17-re-run-merge-and-clean-state exit 0; gh workflow run ci.yml on 2cc8bb5, https://github.com/szTheory/cogmaster/actions/runs/43664171483 conclusion success; the run-selection test keyed to the pushed head exit 0 before any timeout file was edited"
        status: pass
    human_judgment: false
  - id: T2
    description: "every limit was re-measured at execute time from the last 40 successful ci runs: job seconds, test seconds per run, and the lane of each slowest passing line"
    requirement: TUNE-16
    verification:
      - kind: other
        ref: "sh scripts/ci-durations.sh ci.yml 40 exit 0; the run ID list from gh api exit 0 (40 IDs); sh scripts/ci-durations.sh tests for each ID exit 0 for 40 of 40; gh run view --log for the three slowest runs exit 0, and for the 40 runs of the lane-aware read exit 0 for 40 of 40"
        status: pass
    human_judgment: false
  - id: T3
    description: "the timeout-minutes comments and values satisfy the machine check: each value is at least twice the cited slowest run, in the accepted comment form, with no provisional word"
    requirement: TUNE-16
    verification:
      - kind: integration
        ref: "hygiene.timeout_comments and its self test inside cmake --workflow --preset hygiene (84 of 84) exit 0; the library.diagnose and hygiene.tree comment greps exit 0; test -f ci.yml and no provisional word exit 0"
        status: pass
    human_judgment: false
  - id: T4
    description: "all timeout edits sit in one commit that is the last non-planning commit, only timeout files changed, no value was lowered, every added run ID is in the measured window, and nightly, digest, release, src and include are untouched"
    requirement: TUNE-16
    verification:
      - kind: other
        ref: "git log --oneline origin/main..HEAD -- .github/workflows/ci.yml lists one commit exit 0; last non-planning commit equals it exit 0; name-only file list exit 0; paste and awk raise-only comparison exit 0; window citation loop exit 0; git diff --quiet origin/main on nightly.yml digest.yml release.yml src include exit 0"
        status: pass
    human_judgment: false
  - id: T5
    description: "the ci and hygiene workflows pass on the workflow commit and the whole Task 1 verify passes again after it (the tracer gate)"
    requirement: TUNE-16
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (738 of 738, then 5 of 5) on 33bbfc2; cmake --workflow --preset hygiene exit 0 (84 of 84); the Task 1 verify command re-run exit 0"
        status: pass
    human_judgment: false
  - id: T6
    description: "the ship hand-off names the commands for the shipped-tree fence, the title, the auto-merge shell command, the CI run on the head commit, the release pull request and the ancestry re-check, and none of them ran in this plan"
    requirement: TUNE-16
    verification:
      - kind: other
        ref: "git diff --quiet merge-base HEAD src include CMakeLists.txt cmake exit 0; the Task 2 verify command exit 0; gh pr list --head phase/17-re-run-merge-and-clean-state --state open returns 0"
        status: pass
    human_judgment: false

duration: 29min
completed: 2026-11-15
status: complete
---

# Phase 17 Plan 04: Re-derive the timeouts and write the ship hand-off Summary

**A green dispatched run on the ledger head was the sample, a fresh read of the last 40 successful runs re-derived every limit by the twice-the-slowest rule in one workflow commit (`33bbfc2`): the rehearsal job rose from 13 to 14 minutes, `library.diagnose` from 206 to 289 s and `hygiene.tree` from 60 to 75 s, nothing was lowered, and `cmake --workflow --preset ci` and `--preset hygiene` exit 0 on it.**

## Performance

- **Duration:** about 29 min (push and dispatch 2026-11-15T01:40Z, evidence commit 02:06Z); the dispatched run itself took 8 minutes
- **Tasks:** 2 (Task 1 tracer, Task 2 auto)
- **Commits:** 2 measured from the plan base (`2cc8bb5`) to `e7ac5e5`
- **Files modified:** 6 (`ci.yml`, two CTest files, the evidence note, SEED-015, STATE.md). `actuals.tokens` is chars over four of the realized diff, about 3.8k, almost all of it the evidence note.

## Accomplishments

- **Push and sample.** `2cc8bb5` was pushed once (hooks ran) and `ci` dispatched on it: https://github.com/szTheory/cogmaster/actions/runs/43664171483, success. The run-selection gate keyed to the pushed head exited 0 before any file was edited, and nothing was committed between that push and the workflow commit.
- **Measurement.** `sh scripts/ci-durations.sh ci.yml 40` and the per-test reads of all 40 window runs exited 0 (none left out). The lane of each slowest passing line came from the run logs.
- **Job limits.** Of the 14 `timeout-minutes` lines, one rose (rehearsal, 2 x 393 s is 786 s against 780 s); `be32` holds at 21 (2 x 609 s), `build` at 12 (exactly 2 x 360 s), `asan` at 9, the perf job at 6 (the rule gives 4, raise-only keeps 6), and the hygiene job at its 5-minute floor with a new comment. Every comment now cites a run in the window.
- **Test limits.** `library.diagnose` 289 s (2 x 144.43 s, macos-x64, 24 runs, run 43460663238), `hygiene.tree` 75 s with its first timeout comment (2 x 37.32 s, hygiene, 40 runs, run 43562888383), `runner.diagnose` unchanged at 633 s.
- **Commit and gates.** One commit touches `ci.yml`, it is the last non-planning commit and holds only timeout files; the hygiene workflow (84 of 84) and the ci workflow (738 of 738, then 5 of 5) pass on it; the whole Task 1 verify re-ran green as the tracer gate.
- **Hand-off.** The evidence note holds `## Timeouts` and `## Ship hand-off`; the fence `git diff --quiet "$(git merge-base HEAD origin/main)" HEAD -- src include CMakeLists.txt cmake` exits 0 and no pull request is open.

## Task Commits

1. **Task 1 (tracer): sample, measure, edit, check** - `33bbfc2` (ci, `ci: re-derive drifted job and test timeouts from live runs`)
2. **Task 2: timeouts note, ship hand-off, SEED-015 breadcrumb, Runtime Log row** - `e7ac5e5` (docs)

**Plan metadata:** the commit that carries this file (docs: complete plan).

## Commands and exit statuses

The full tables are in `17-RUN-EVIDENCE.md` (Timeouts, Ship hand-off).

| Command | Exit |
|---|---|
| `git push origin phase/17-re-run-merge-and-clean-state` (head `2cc8bb5`) | 0 |
| `gh workflow run ci.yml --ref phase/17-re-run-merge-and-clean-state`, then `gh run watch --exit-status` | 0, 0 |
| run-selection gate on the pushed head (remote head equals HEAD, one green dispatched run) | 0 |
| `sh scripts/ci-durations.sh ci.yml 40` | 0 |
| `sh scripts/ci-durations.sh tests <id>` for 40 IDs | 0 for 40 of 40 |
| lane-aware read: `gh run view <id> --log` for 40 IDs and three slowest runs | 0 for 43 of 43 |
| `cmake --workflow --preset hygiene`, first after the edits | 8 (host load, see Variances) |
| `cmake --workflow --preset hygiene`, re-run | 0 |
| `cmake --workflow --preset ci` on `33bbfc2` | 0 |
| Task 1 verify, re-run as the tracer gate | 0 |
| one-commit, last-non-planning, only-timeout-files, raise-only, window-citation, comment greps, no `provisional` | 0 each |
| fence on `nightly.yml`, `digest.yml`, `release.yml`, `src`, `include` against `origin/main` | 0 |
| shipped-tree fence `git diff --quiet "$(git merge-base HEAD origin/main)" HEAD -- src include CMakeLists.txt cmake` | 0 |
| Task 2 verify, SEED-015 count, Runtime Log row, leak grep | 0 each |
| `gh pr list --head phase/17-re-run-merge-and-clean-state --state open` count | 0 |

## Decisions Made

- Followed the plan's shape. Every value is from the fresh read, not copied from the discuss step; the discuss-time shape held (be32 21, rehearsal 14, `library.diagnose` 289, `hygiene.tree` 75, hygiene job at its floor, perf at 6).
- `nofp` keeps its worst-lane note: the 53 s figure still comes from the slowest matrix leg. All other comments are the report's comment text.
- `runner.diagnose` stays at 633 s (see Variances for why the 450.48 s reading is not the figure to double).

## Deviations from Plan

None - the plan executed as written. Variances only.

### Variances

**1. [Variance] The `runner.diagnose` decision uses unscaled lane seconds, with an added lane-aware read**
- The plan's rule is "T = twice S in raw hosted seconds (ctest scales per lane)" and "change only if T exceeds 633". The aggregated per-test files give S = 450.48 s for `runner.diagnose`, which is the be32 lane, where the 633 s limit is multiplied by 8. To apply the rule on the same scale as the limit I read all 40 window logs once more, keeping job name, test and seconds of every passing line of the three tests (424 lines, 40 of 40 reads exit 0). The largest unscaled-lane reading is 179.23 s (macos-x64, run 43460663238), twice is 358.46 s, which is under 633 s, so the file is untouched as the plan allows. Taking the 450.48 s at face value would have set 901 s (a 7208 s limit on be32) with no benefit. The table and the per-lane readings are in the evidence note.

**2. [Variance] The first local hygiene workflow run after the edit failed on host load, not on the limit**
- `hygiene.tree` timed out at its new 75 s limit while other processes on this machine held the load average near 69 (the same pattern plan 02 and Phase 16 recorded); `sh scripts/hygiene.sh --tree` alone took 34 s. The re-run at a load average near 18 passed in 32.36 s, and the hosted slowest is 37.32 s, so the limit was not raised from a local reading (XFER-012). Recorded in the evidence note and SEED-015.

**3. [Variance] The plan-start time was not recorded at the start**
- The duration is estimated from the dispatch time of the sample run (01:40Z) minus a few minutes, to the evidence commit (02:06Z).

---

**Total deviations:** 0 auto-fixed, 3 variances noted
**Impact on plan:** none; the limits and checks are those the plan names.

## Issues Encountered

- The `be32` job and the other long jobs need a stale-run refresh only; the one near-miss is `build`, whose limit equals exactly twice its slowest (720 s) and so has no margin. It is recorded in SEED-015 with `wasm32` (141 s against a 300 s floor) and left alone here (D-12 keeps the change raise-only).

## Known Stubs

None. No placeholder or hard-coded empty value was added.

## Threat Flags

None. T-17-09: limits come from twice the slowest of a fresh 40-run read, raise-only, checked by `hygiene.timeout_comments`, and the sample is a green run on the shipped code with the old limits. T-17-10: `nightly.yml`, `digest.yml` and `release.yml` are unchanged against `origin/main`. T-17-SC: no package was installed.

## For the owner

Nothing is needed from you. The branch holds the carried ledger, the diagnose prune, the milestone-close script and the one workflow commit, and nothing is pushed past `2cc8bb5` yet: `/gsd-ship` pushes once (which also starts the rehearsal job that the `ci.yml` edit triggers).

## Next

The orchestrator's steps, none of which ran in this plan:

1. Code review, then verification of phase 17.
2. `/gsd-ship 17`, which pushes the branch once and opens the pull request titled `fix(compat): publish the library results judged on main and re-derive drifted timeouts`.
3. In a shell, after the pull request exists: `gh pr merge --auto --squash`, then confirm a CI run on the head commit (with its rehearsal job), the release pull request green and merged, the release assets, and `git merge-base --is-ancestor 0a97075f34e48e2b1d0f72aa231006cc9366a398 origin/main` on the new `origin/main`. The exact commands are under `## Ship hand-off` in `17-RUN-EVIDENCE.md`.

## Self-Check: PASSED

- Files found: `.github/workflows/ci.yml`, `tests/library/tests.cmake`, `tests/hygiene/tests.cmake`, `.planning/phases/17-re-run-merge-and-clean-state/17-RUN-EVIDENCE.md`, `.planning/seeds/SEED-015-whole-list-follow-ups.md`.
- Commits found as ancestors of HEAD: `33bbfc2`, `e7ac5e5`.

---
*Phase: 17-re-run-merge-and-clean-state*
*Completed: 2026-11-15*
