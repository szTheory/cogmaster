---
phase: 16-graphics-i-and-ii-draw
plan: 01
subsystem: tooling
tags:
  - riders
  - fetch-retry
  - dirty-tree
  - docs-vocabulary
  - ci-lane

requires: []
provides:
  - three-attempt assembler download in cmake/WlaDx.cmake with a transient-code retry and one SHA-256 check after the loop
  - a POSIX retry loop on the same curl exit codes in scripts/fetch-pin.sh
  - an early exit 2 in scripts/library.sh for a run against the tracked ledger from a tree with tracked changes outside .planning
  - nine ci-lane controls in tests/policy/tooling_cases.cmake, listed in the ci and wasm32 inventories
  - the current fault vocabulary in docs/audio.md, docs/embedding.md and docs/runner.md
affects:
  - 16-02
  - 16-03
  - 16-09

actuals:
  tokens: 7700
  tasks: 3
  commits: 4
plan_head_before: 5a652a3ee6b690d8498a4f0648b9eae52493c0ca
plan_head_after: 8b9e28411b791e37a142716d9d213aa5993c2ba2

tech-stack:
  added: []
  patterns:
    - "retry only a closed set of transient curl codes, verify the digest once after the last attempt"
    - "a refusal gated on the tracked ledger path, so tests that run the script in the real tree with stand-in ledgers never trip it"
    - "throwaway repositories reset at test time, so a rerun never reads what an earlier run wrote"

key-files:
  created:
    - tests/policy/tooling_cases.cmake
    - .planning/phases/16-graphics-i-and-ii-draw/deferred-items.md
  modified:
    - cmake/WlaDx.cmake
    - scripts/fetch-pin.sh
    - scripts/library.sh
    - tests/policy/tests.cmake
    - tests/policy/repository-checks.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/library/tests.cmake
    - docs/testing.md
    - docs/audio.md
    - docs/embedding.md
    - docs/runner.md

key-decisions:
  - "All nine rider tests are registered in the ci lane (included from tests/policy/tests.cmake, listed in both ci.txt and wasm32.txt, no platform guard), so cmake --workflow --preset ci runs them on every build leg"
  - "The retry set is curl codes 6, 7, 18, 28, 35, 52, 55 and 56; an HTTP error status (22) and a missing file:// URL (37) fail on attempt 1, and the digest is checked once after the loop, so a partial file is never accepted"
  - "policy.pins.retry runs through expect_fail.cmake with CODE 0, because a registration with PASS_REGULAR_EXPRESSION makes CTest ignore the exit status and a failing fetch-pin that printed calls=3 would pass"
  - "The dirty-tree refusal is gated on the tracked ledger path; the synthetic workflow step now runs against a stand-in ledger and page, so a dirty development tree still passes library.synthetic"
  - "The repository-check budget is unchanged at 887 check lines against 9386 src lines: WlaDx.cmake and fetch-pin.sh are build, library.sh is operation, tests/policy is outside the counted globs"

requirements-completed:
  - TUNE-12
  - TUNE-13
  - TUNE-14

coverage:
  - id: D1
    description: "The assembler download retries transient curl codes up to three attempts, prints the attempt number, removes a partial file before the next attempt and fails with the attempt count"
    requirement: TUNE-14
    verification:
      - kind: integration
        ref: "ctest --preset ci -R policy.wla_fetch.transient: cmake -P cmake/WlaDx.cmake against a refused local port with RETRY_DELAY 0, exit 1, output names attempt 3 of 3"
        status: pass
    human_judgment: false
  - id: D2
    description: "A permanent fetch failure and a digest mismatch each fail on the first attempt; the digest is checked once after the loop; the existing assembler pin self test still passes"
    requirement: TUNE-14
    verification:
      - kind: integration
        ref: "ctest --preset ci -R policy.wla_fetch.permanent, policy.wla_fetch.hash and fixtures.wla_pin.self_test, exit status 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "scripts/fetch-pin.sh retries curl exit codes 6, 7, 18, 28, 35, 52, 55 and 56 up to three calls and fails at once on any other code"
    requirement: TUNE-14
    verification:
      - kind: integration
        ref: "ctest --preset ci -R policy.pins.retry (fake curl exits 6 twice then succeeds, exit 0, three calls) and policy.pins.permanent (exit 22, exit 1, one call); ctest --preset hygiene -R policy.pins exit status 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "sh scripts/library.sh run refuses with exit 2 against the tracked ledger from a tree with tracked changes outside .planning, and does not refuse a modified .planning file, an untracked file or a clean tree"
    requirement: TUNE-13
    verification:
      - kind: integration
        ref: "ctest --preset ci -R policy.library_dirty, four tests, passed twice in a row, exit status 0"
        status: pass
      - kind: integration
        ref: "ctest --preset ci -R library. exit status 0 in a dirty tree (library.synthetic runs against stand-in ledgers)"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/audio.md, docs/embedding.md and docs/runner.md state the current fault vocabulary with every diagnose key kept"
    requirement: TUNE-12
    verification:
      - kind: integration
        ref: "ctest --preset ci -R docs. exit status 0; the plan's four grep acceptance counts; embedding.md 36909 and runner.md 37973 bytes against the 37100 and 38400 caps"
        status: pass
      - kind: other
        ref: "agent read of the doc diff against include/cogmaster.h, src/internal.h, every cogmaster__bus_fault caller in src and runner/diag_class.h: no raiser for the control device or rom_write, and wclass and pclass can only read none"
        status: pass
    human_judgment: false
  - id: D6
    description: "The nine controls run in the ci lane and the repository-check budget is unchanged"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit status 0, 685 tests passed, configure inventory check equal to tests/inventory/ci.txt"
        status: pass
      - kind: other
        ref: "cmake -DROOT=. -P tests/cmake/budgets.cmake exit status 0, 887 check lines against 9386 src lines"
        status: pass
    human_judgment: false

duration: 13 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 01: Riders Summary

**Transient-error retries for the pinned assembler and tool downloads, an early refusal of a library run from a dirty tree, and the current fault vocabulary in four docs, with nine ci-lane controls**

## Performance

- **Duration:** 13 min
- **Started:** 2026-11-14T02:58:20Z
- **Completed:** 2026-11-14T03:11:42Z
- **Tasks:** 3
- **Files modified:** 13 (one created)

## Accomplishments

- `cmake/WlaDx.cmake` downloads in up to three attempts, retrying only curl codes 6, 7, 18, 28, 35, 52, 55 and 56, with `RETRY_DELAY` (default 2) between them. It drops the expected-hash keyword, whose mismatch stops CMake before a status is read, and checks the SHA-256 once after the loop.
- `scripts/fetch-pin.sh` keeps `--retry 3` and wraps the call in a POSIX loop on the same exit codes, with `FETCH_PIN_RETRY_DELAY` (default 2).
- `scripts/library.sh` computes `dirty` once and, for `run` against the tracked ledger only, exits 2 before any work with a message that names the condition and no file, path or digest. The diagnose stamp reuses the variable.
- `tests/policy/tooling_cases.cmake` registers nine controls (`policy.wla_fetch.{transient,permanent,hash}`, `policy.pins.{retry,permanent}`, `policy.library_dirty.{refuses,planning,untracked,clean}`) in the ci lane. None downloads anything real: a refused local port, a missing and a tampered `file://` URL, a fake `curl` and four throwaway repositories.
- `docs/audio.md`, `docs/embedding.md` and `docs/runner.md` state the current fault vocabulary (six kinds, seven devices, no unmapped-port fault, `ctrl` and `rom_write` no longer recorded, `where=port`, `wclass` and `pclass` annotated as always none since behaviour revision 18) with every diagnose key kept; `docs/testing.md` gains the two sentences the plan asked for.

## Task Commits

1. **Task 1: tracer, the assembler fetch retries end to end** - `35b9e7a` (fix)
2. **Task 2: the library run refuses a dirty tree** - `908467f` (test, RED: the refusal case failed with exit 1 and the three controls passed) then `4f8e53d` (feat, GREEN: four of four, twice in a row)
3. **Task 3: the docs state the current fault vocabulary** - `8b9e284` (docs)

**Plan metadata:** the SUMMARY commit and the STATE and ROADMAP commit that follow this file.

The tracer gate for Task 1 was met by re-running its verify chain end to end (budgets, `cmake --preset ci`, the six ci tests, `cmake --preset hygiene`, the hygiene `policy.pins` tests, all exit 0) before expanding.

## Files Created/Modified

- `cmake/WlaDx.cmake` - three-attempt download, transient-code retry, one digest check after the loop
- `scripts/fetch-pin.sh` - POSIX retry loop on the transient curl exit codes
- `scripts/library.sh` - the early refusal and the shared `dirty` variable
- `tests/policy/tooling_cases.cmake` - the nine controls and their fixtures (new)
- `tests/policy/tests.cmake`, `tests/policy/repository-checks.txt` - the include and the `behavior` row; the `scripts/library.sh` row keeps `operation` and names the refusal
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - nine sorted names each, identical in both
- `tests/library/tests.cmake` - the synthetic workflow step uses a stand-in ledger and page
- `docs/testing.md`, `docs/audio.md`, `docs/embedding.md`, `docs/runner.md` - the corrections and two sentences

## Decisions Made

- The nine controls are ci-lane tests with no platform guard, as the plan decided; the hosted legs first run them at plan 03's draft push. Their timeouts (30 s and 60 s) are provisional local measurements: the whole group runs in about 1.1 s here.
- The dirty-tree controls keep one throwaway repository per case so parallel runs share nothing, and reset it at test time.
- The `dirty` block sits after the `page=` line rather than directly after `ledger=`; the order has no effect because both are plain assignments.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The synthetic workflow step ran the script against the tracked ledger in the real tree**
- **Found during:** Task 2 (running `ctest --preset ci -R '^library\.'` in a tree with uncommitted changes)
- **Issue:** `library.synthetic` runs `cmake --workflow --preset library` in the real tree, which calls `scripts/library.sh run` with no ledger override, so the new refusal made it fail with a missing `results.csv` whenever a tracked file outside `.planning` was modified. The plan gated the refusal on the tracked ledger for the stand-in calls but did not list this call site.
- **Fix:** the workflow step now passes `COGMASTER_LEDGER` and `COGMASTER_PAGE` pointing at the stand-in copies the test already builds, so it never reads or writes the tracked ledger.
- **Files modified:** `tests/library/tests.cmake` (not in the plan's file list)
- **Verification:** `ctest --preset ci -R '^library\.'` passed 9 of 9 in a dirty tree, and again inside the full workflow in a clean one.
- **Committed in:** `4f8e53d`

**2. [Rule 1 - Bug] A PASS_REGULAR_EXPRESSION registration ignores the exit status**
- **Found during:** Task 1 (writing `policy.pins.retry`)
- **Issue:** the plan called the retry control "a plain registration, since a non-zero exit already fails it". CTest ignores the exit code when a pass expression is set, so a fetch-pin that retried three times and then failed would still pass.
- **Fix:** `policy.pins.retry` runs through `tests/cmake/expect_fail.cmake` with CODE 0 and the regex `calls=[ ]*3`, which checks both. The three `policy.library_dirty` controls stay plain registrations on purpose: they assert only that the run was not refused.
- **Files modified:** `tests/policy/tooling_cases.cmake`
- **Committed in:** `35b9e7a`

**3. [Rule 2 - Missing critical] Throwaway repositories hold copies of two scripts**
- **Found during:** Task 2
- **Issue:** the controls copy `scripts/library.sh` and `scripts/compat-common.sh` at configure time, so an edit to either left the controls testing a stale copy until the next manual configure (the first GREEN run failed for exactly this reason).
- **Fix:** both scripts are added to the directory's `CMAKE_CONFIGURE_DEPENDS`, so a build reconfigures when either changes.
- **Files modified:** `tests/policy/tooling_cases.cmake`
- **Committed in:** `4f8e53d`

**4. [Rule 3 - Plan variant] The reset avoids `git clean`**
- **Found during:** Task 2
- **Issue:** the plan's reset was `git checkout -q -- . && git clean -qfd`. `git clean` is a blanket removal that this project's agents are told never to run in a worktree; the throwaway repository only ever holds `build` and `new.txt` as untracked files.
- **Fix:** the wrapper runs `git checkout -q -- .` and `rm -rf build new.txt` inside a directory it first checks holds a `.git`.
- **Files modified:** `tests/policy/tooling_cases.cmake`
- **Committed in:** `908467f`

---

**Total deviations:** 4 auto-fixed (2 bugs, 1 missing critical, 1 plan variant)
**Impact on plan:** all four are needed for the controls to mean what the plan says; none widens scope beyond one test file outside the plan's list.

## Issues Encountered

- `hygiene.tree` (60 s timeout) passed alone in about 43 s but timed out in one parallel run of the whole hygiene lane (84 tests). It is not touched by this plan and is logged in `.planning/phases/16-graphics-i-and-ii-draw/deferred-items.md`.
- One optional sanity run (the old `scripts/fetch-pin.sh` against the retry fixture) was refused by the harness because the shell script held an `rm -rf`; it was not retried another way. The old `cmake/WlaDx.cmake` was run instead and prints no attempt line, so the transient control cannot pass against it.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02 can edit `tests/inventory/ci.txt` and `tests/inventory/wasm32.txt` after this plan's lines; both keep the same relative order.
- The base hashes file for the later subset compare can be taken from this head: the riders change no code that builds the runner or the fixture images.
- The hosted legs, including both `ci-msvc` legs, first run the nine controls at plan 03's draft push; the timeouts are provisional until then.
- Nothing is pushed; `ci.yml` and `cmake/packaging.cmake` are untouched (`git diff --name-only origin/main -- .github/workflows/ci.yml cmake/packaging.cmake` prints nothing).

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

All twelve changed files and the new `tooling_cases.cmake` and `deferred-items.md` exist; commits 35b9e7a, 908467f, 4f8e53d and 8b9e284 are ancestors of HEAD. The plan-level verification ran after the last commit: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, `ctest --preset ci -R '^policy\.(wla_fetch|pins|library_dirty)'` 9 of 9, `ctest --preset ci -R '^(fixtures\.wla_pin|library\.|docs\.)'` all passed, `ctest --preset hygiene -R '^policy\.pins'` 2 of 2, the clang-format dry run exit 0, and `cmake --workflow --preset ci` exit 0 with 685 of 685 tests.
