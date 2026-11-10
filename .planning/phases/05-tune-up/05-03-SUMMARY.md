---
phase: 05-tune-up
plan: "03"
subsystem: ci
tags: [ci, timeouts, tests, tooling]
status: complete

requires:
  - phase: 05-tune-up
    provides: docs split and the folded comparator (plans 05-01 and 05-02)
provides:
  - scripts/ci-durations.sh, a read-only CI efficiency report that counts runs rather than matrix lanes
  - a timeouts check that rejects provisional comments, missing run IDs, short timeouts and a floor under 5 minutes
  - every timeout-minutes line in ci.yml, nightly.yml and release.yml re-derived from measured runs
  - a recorded reason for keeping the be32 C++ compiler
affects: [05-04, 05-06, 05-13]

actuals:
  tokens: 5300
  tasks: 2
  commits: 2

plan_head_before: 380795a1ae7918bbc6f606bc08171d2afa20c8a0
plan_head_after: 2002713930296a31c0ab713a9bc9cbe4e0c66628
commits: 2

tech-stack:
  added: []
  patterns:
    - "A timeout comment carries its own arithmetic, so the check reads minutes and seconds from the line and compares them"
    - "Every gh api answer is written to a file in a mktemp directory before jq reads it"

key-files:
  created:
    - scripts/ci-durations.sh
  modified:
    - tests/cmake/repository.sh
    - tests/hygiene/tests.cmake
    - tests/policy/repository-checks.txt
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - .github/workflows/release.yml
    - docs/testing.md

key-decisions:
  - "The be32 C++ trim is not taken: every normal configure enables C++ for the public header check, so the lane needs the C++ cross compiler (static evidence, no step time measured)"
  - "Floored jobs keep a parenthetical reason such as (apt) or (API) between the floor words and the arithmetic, and the check accepts it"
  - "scripts/ci-durations.sh is classified as operation, so it does not count toward the check-line budget"

patterns-established:
  - "Size a timeout from the slowest of the last 40 successful runs and name that run in the comment"

requirements-completed: [TUNE-01]

coverage:
  - id: D1
    description: "hygiene.timeout_comments fails the five bad fixture lines (bare, a guess, provisional, 1 minute under twice 40 s, a 3-minute floor-only line) and accepts the floor-only line at 5 minutes and a correct 8-minute line"
    requirement: TUNE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R hygiene.timeout_comments.self_test failed with exit 8 against the old check (RED) and passed with exit 0 after the awk change; running the check on the fixture names exactly the five bad lines and prints timeout_comments: 5 lines"
        status: pass
    human_judgment: false
  - id: D2
    description: "sh scripts/ci-durations.sh summarise counts N in runs: two lanes of one run give N 1 slowest 187 run 42 minutes 7, and three lane instances over two runs give N 2 slowest 187 run 42"
    requirement: TUNE-03
    verification:
      - kind: unit
        ref: "the plan verify awk on both stdin fixtures exited 0 (one-run and two-runs)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The report runs against the live repository for ci.yml, nightly.yml and release.yml, and every N is between 1 and 40"
    requirement: TUNE-03
    verification:
      - kind: integration
        ref: "sh scripts/ci-durations.sh ci.yml, nightly.yml and release.yml each exit 0; the awk range check on the ci.yml report exit 0 (N from 4 to 40)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every timeout-minutes line in the three workflows reads 2 x S s, slowest of N runs, run id with minutes*60 at least 2*S (or the floor form), none says provisional, and each cited run exists"
    requirement: TUNE-01
    verification:
      - kind: integration
        ref: "cmake -DDIR=.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake exit 0; grep -rqi provisional .github/workflows exit 1; gh api repos/szTheory/cogmaster/actions/runs/ID --jq .id exit 0 for all 17 cited IDs"
        status: pass
    human_judgment: false
  - id: D5
    description: "The three workflows lint clean and the repository gates stay green with the check lines under one tenth"
    requirement: TUNE-01
    verification:
      - kind: integration
        ref: "actionlint with the ubuntu-26.04 label ignore and the queue-key ignore exit 0 on all three workflows; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 printing 532 check lines / 7444 src lines; cmake --workflow --preset hygiene exit 0 (38 of 38); cmake --workflow --preset ci exit 0 (399 of 399)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The be32 C++ trim is settled from recorded static evidence: abi/tests.cmake is included on the normal path and enable_language(CXX) sits at column 1, so the lane keeps g++-powerpc-linux-gnu"
    requirement: TUNE-03
    verification:
      - kind: other
        ref: "grep -n abi/tests.cmake tests/tests.cmake exit 0; grep -n '^enable_language(CXX)' tests/abi/tests.cmake exit 0; grep -q g++-powerpc-linux-gnu .github/workflows/ci.yml exit 0"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 03: CI Durations and Measured Timeouts Summary

**`scripts/ci-durations.sh` reports twice the slowest successful run per job with N counted in runs, the timeouts check now verifies that arithmetic and rejects provisional comments, and all 22 workflow timeouts are re-derived from live runs with the run named.**

## Performance

- **Tasks:** 2 of 2, 2 commits
- **Files:** 8 touched (1 created)

## Accomplishments

- RED first: the `hygiene.timeout_comments.self_test` fixture became seven lines (two good, five bad) and failed against the old check (ctest exit 8). The `timeouts` branch of the awk program in `tests/cmake/repository.sh` then took the new rules (provisional rejected, floor-only form needs 5 minutes or more, otherwise `2 x S s` and a 9-or-more-digit run ID with minutes*60 at least 2*S). The self test passes (exit 0) and the check on the fixture names exactly the five bad lines in file order.
- `scripts/ci-durations.sh` (POSIX sh, `gh` and `jq`, read-only): live mode lists the workflow's last successful runs (default 40), writes every answer to a `mktemp -d` directory before jq reads it, and pipes the merged jobs into its own `summarise` mode. `summarise` strips the matrix suffix, skips nested reusable-workflow names (containing ` / `), counts distinct run IDs and sorts slowest first. It is classified `operation` in `tests/policy/repository-checks.txt`.
- Re-derived every `timeout-minutes` line from `sh scripts/ci-durations.sh <workflow>` output (the minutes come from the report, not from the planning notes). Network-bound and roll-up jobs keep the 5-minute floor; matrix jobs name the worst lane.
- `docs/testing.md` no longer says a timeout stays provisional until ten runs exist; it describes the 40-run rule, the N-in-runs counting, the floor, the no-runs-yet form and the script.

## Measured timeouts

Report heads (job, N, slowest seconds, run, minutes), from the live reports:

| Workflow | Job | N | Slowest s | Run | Minutes set |
|----------|-----|---|-----------|-----|-------------|
| ci.yml | rehearsal | 4 | 387 | 42721125470 | 13 |
| ci.yml | be32 | 37 | 275 | 42741490263 | 10 (was 5) |
| ci.yml | build (worst lane) | 37 | 178 | 42741490263 | 6 |
| ci.yml | perf | 26 | 151 | 42752770307 | 6 (was 10) |
| ci.yml | asan | 37 | 84 | 42728887374 | 5 floor |
| ci.yml | wasm32 | 37 | 67 | 42752737395 | 5 floor |
| ci.yml | nofp (worst lane) | 37 | 54 | 39373821097 | 2 (was 1) |
| ci.yml | hygiene | 40 | 52 | 39515950108 | 2 |
| ci.yml | retroarch | 37 | 47 | 42752737395 | 5 floor |
| nightly.yml | bench-wasm32 | 33 | 438 | 42561596371 | 15 (was 20) |
| nightly.yml | bench (worst lane) | 33 | 297 | 40817678633 | 10 (was 5) |
| nightly.yml | vectors | 25 | 91 | 39377102899 | 5 floor |
| nightly.yml | fuzz | 26 | 90 | 41002722591 | 5 floor |
| release.yml | publish | 4 | 361 | 42723573587 | 13 (was 15) |
| release.yml | release-please | 13 | 22 | 42722464705 | 5 floor |
| release.yml | install-line-latest | 4 | 11 | 42723573587 | 5 floor |

The remaining ci.yml, nightly.yml and release.yml jobs (changes, title, hash-equality, cleanup, CI required, report) are 1 minute or less and take the floor. Two timeouts were genuinely too tight: be32 had 5 minutes against a 275-second slowest run, and nofp had 1 minute against a 54-second one.

## Evidence

Commands run from the repository root; `$TMPDIR` holds scratch files only and no log is committed.

| Command | Exit |
|---------|------|
| `ctest --test-dir build/hygiene -R '^hygiene\.timeout_comments\.self_test$'` (old check, RED) | 8 |
| the same ctest after the awk change | 0 |
| `sh scripts/ci-durations.sh summarise` on the one-run and two-run fixtures, then the verify awk on each | 0, 0 |
| `sh scripts/ci-durations.sh ci.yml`, `nightly.yml`, `release.yml` | 0, 0, 0 |
| `awk -F '\t' 'NR>1 && ($2 < 1 \|\| $2 > 40) {bad=1} END {exit bad}'` on the ci.yml report | 0 |
| `cmake -DDIR=.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake` | 0 |
| `grep -rqi provisional .github/workflows` | 1 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0, prints `532 check lines / 7444 src lines, strict one tenth` |
| `cmake --workflow --preset hygiene` | 0, 38 of 38 tests |
| `cmake --workflow --preset ci` | 0, 399 of 399 tests |

Each cited run ID confirmed with `gh api repos/szTheory/cogmaster/actions/runs/<id> --jq .id`, exit 0 for all 17: 39373821097, 39377102899, 39451067810, 39455262847, 39515950108, 40817678633, 41002722591, 41272772226, 42561596371, 42721125470, 42722464705, 42722509271, 42723573587, 42728887374, 42741490263, 42752737395, 42752770307. Example run URL: https://github.com/szTheory/cogmaster/actions/runs/42741490263.

No hosted run of these changes exists yet; plan 05-06's phase-branch dispatch is the first.

## be32 trim evidence

| Command | Output | Exit |
|---------|--------|------|
| `grep -n 'abi/tests.cmake' tests/tests.cmake` | `19:  include(${CMAKE_CURRENT_LIST_DIR}/abi/tests.cmake)` and `31:include(${CMAKE_CURRENT_LIST_DIR}/abi/tests.cmake)` | 0 |
| `grep -n '^enable_language(CXX)' tests/abi/tests.cmake` | `107:enable_language(CXX)` | 0 |
| `grep -q 'g++-powerpc-linux-gnu' .github/workflows/ci.yml` | none | 0 |

The second match is at column 1 outside any `if()` block, and line 31 includes the file unconditionally, so every normal configure (be32 included) enables C++ for the public header's C++ compile check. The be32 lane therefore needs `g++-powerpc-linux-gnu` and the toolchain's CXX line.

## Decisions

- The be32 C++ trim was settled by static evidence (the two greps above showing every configure enables C++) instead of the configure trial the CONTEXT speed decision described. The trim was not taken: the header test is not weakened to make it pass. No step time was measured and none is claimed.
- A floored comment may carry a short reason in brackets, such as `5-minute floor (apt); 2 x 84 s, ...`; the check accepts it because the arithmetic follows a `; ` separator.

## Task Commits

1. **Task 1: the duration report and the tightened timeouts check** - `c27ead4` (feat)
2. **Task 2: re-derive every workflow timeout; record the be32 decision** - `2002713` (ci)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] actionlint needs a second ignore for release.yml**
- **Found during:** Task 2 verification
- **Issue:** The plan's `actionlint -ignore 'label "ubuntu-26.04'` command exits 1 on `release.yml` line 20 (`unexpected key "queue" for "concurrency" section`). The same error appears on the unmodified `release.yml` at the previous commit, so it is the local linter's metadata lag (like the 26.04 labels), not this plan's edit.
- **Fix:** Added `-ignore 'unexpected key "queue"'`; actionlint then exits 0 on all three workflows, and exits 0 on `ci.yml` and `nightly.yml` with only the plan's ignore. No workflow content was changed for it.
- **Files modified:** none

**2. [Observation] hygiene.tree timed out once under load**
- **Found during:** the first `cmake --workflow --preset hygiene`
- **Issue:** `hygiene.tree` (60-second limit) timed out while 38 tests ran in parallel; run alone it passed in 37 seconds. The check scans every tracked file and is slower than its limit leaves room for under contention. It is unrelated to this plan's files, so it is not fixed here; the repeat of the full hygiene workflow exited 0.
- **Files modified:** none

**Total deviations:** 1 auto-fixed (blocking), 1 observation. **Impact:** none on behaviour.

## Known Stubs

None.

## Threat Flags

None. The script reads GitHub run metadata with the user's existing `gh` login, writes only to a temporary directory it removes, and adds no network call to CI.

## Self-Check: PASSED

- scripts/ci-durations.sh found
- commits c27ead4 and 2002713 are ancestors of HEAD
