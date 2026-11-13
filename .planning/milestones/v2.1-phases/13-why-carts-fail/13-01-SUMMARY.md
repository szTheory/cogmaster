---
phase: 13-why-carts-fail
plan: 01
subsystem: runner
tags: [c, cmake, diagnose, expect, runner]

requires:
  - phase: 12-library-run
    provides: the library run and the boots@F predicate this verdict is computed from
provides:
  - cogmaster-run --diagnose FILE, one closed line of boot facts over 1800 frames
  - cogmaster__expect_observe and cogmaster__expect_boots shared by --expect and --diagnose
  - runner/diag_class.h, pure helpers for the fault bucket, order, pcslot and where words
  - tests runner.diagnose and runner.diagnose.class, and the stopped-early pin in runner.stop
affects: [13-02, 13-03, 13-04, 13-05, library diagnose]

actuals:
  tokens: 10700
  tasks: 3
  commits: 4
  plan_head_before: 34a9fc8e80f321a0f95abe07504ce105a6d424ff
  plan_head_after: 53455f592cb6598cba3dc7676592803c9976cb4e

tech-stack:
  added: []
  patterns:
    - "one shared observe and predicate between --expect and --diagnose"
    - "header-only static inline helpers so the existing bench binary can test derived words"

key-files:
  created:
    - runner/diag.c
    - runner/diag.h
    - runner/diag_class.h
    - tests/runner/diagnose.cmake
  modified:
    - runner/expect.c
    - runner/expect.h
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - runner/probe.c
    - CMakeLists.txt
    - tests/runner/tests.cmake
    - tests/runner/test_runner.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/runner.md

key-decisions:
  - "The phase branch phase/13-why-carts-fail was cut from the milestone-branch tip, not origin/main, so the milestone docs travel with the phase"
  - "The diagnose state is heap-allocated beside the pixel buffer, since it holds an 8192-byte RAM copy and an embedded expect and wasm32 stacks are small"
  - "verdict600 is cogmaster__expect_boots over the same cogmaster__expect state --expect judges, so the two cannot drift"
  - "The board-source word table lives once, in runner/diag.c; probe.c calls it"

requirements-completed: []

coverage:
  - id: D1
    description: "--diagnose prints one closed line of numbers and table words after 1800 frames, with verdict600 equal to --expect boots@600 on nine owned images"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: runner.diagnose
        status: pass
    human_judgment: false
  - id: D2
    description: "The fault_frame bucket, order, pcslot and where words are pure helpers pinned at every range edge, including 0-based frames 599 and 600 and the 4294967295 wrap"
    requirement: DIAG-01
    verification:
      - kind: unit
        ref: runner.diagnose.class
        status: pass
    human_judgment: false
  - id: D3
    description: "A stop request before call 1800 makes --diagnose exit 1 with one stderr line and no diagnose line"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: runner.stop
        status: pass
    human_judgment: false
  - id: D4
    description: "docs/runner.md documents the option, the line and first non-blank once, within the 38,912 byte budget"
    requirement: DIAG-01
    verification:
      - kind: other
        ref: cmake -DROOT=. -P tests/cmake/budgets.cmake and cmake --workflow --preset ci
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-11-12
status: complete
---

# Phase 13 Plan 01: Diagnose line in the runner Summary

**`cogmaster-run --diagnose FILE` runs 1800 frames and prints one closed line (verdict600, first display and non-blank frames, picture counts, RAM liveness, fault class, PC slot, mapper and its source) whose frame-600 verdict is the `--expect boots@600` predicate over the same state.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-11-12T22:50:00Z
- **Completed:** 2026-11-12T22:59:21Z
- **Tasks:** 3
- **Files modified:** 16

## Accomplishments

- `cogmaster__expect_frame` split into `cogmaster__expect_observe`, `cogmaster__expect_boots` and the case loop; `--expect` output unchanged (`runner.expect.predicates` passes).
- `--diagnose` mode with usage rules (a file is required; every other run option except the profile options is refused, exit 2) and a stop request before call 1800 exiting 1 with no line.
- `runner.diagnose` pins the anchored line shape, no digest or path, verdict equality with `--expect boots@600` and the verdict-from-parts rule on nine images (six when fixtures are off, failing by name when one is missing); `runner.diagnose.class` pins every bucket edge, order, pcslot and where word.
- `docs/runner.md` Diagnose section: 36,849 bytes against the 38,912 budget.

Measured: `runner.diagnose` takes about 15 to 22 seconds locally on one arm64 macOS host (the timeout stays 120 s, provisional). The static-display fixture reads `ram=still`, as the researcher measured. Observed lines: boot_to_play and vdp_lines_192 give `verdict600=pass first_display=2 first_nonblank=2 first_change=2 distinct600=3`; static-display gives `first_display=1 first_nonblank=none`; in-a-00 gives `unmapped_read dev=ctrl pc=0002 pcslot=0 where=port fault_frame=0 order=nopicture`.

## Task Commits

1. **Task 1: end-to-end `--diagnose` line (tracer)** - `c26dbec` (feat)
2. **Task 2 RED: failing tests for the full line and helpers** - `27cbb53` (test)
3. **Task 2 GREEN: complete the closed line** - `e69178c` (feat)
4. **Task 3: docs/runner.md** - `53455f5` (docs)

**Plan metadata:** the docs commit that follows this file (SUMMARY, then STATE and ROADMAP).

Tracer gate: interactive, `human_verify_mode` end-of-phase, automated-only verify, so the tracer verify was re-run (exit 0, `runner.expect.predicates` green) and expansion continued.

## TDD Gate Compliance

Task 2 carried `tdd="true"`. RED `27cbb53`: runner.diagnose failed on its anchored shape assertion (the Task 1 line lacked ram, pcslot, where, fault_frame, order and source) and runner.diagnose.class failed on the planned assertions (bucket, order, pcslot and where CHECKs, with `diag_class.h` holding placeholder values so the case compiled). Semantic assessment: the target tests ran and failed on the intended behaviour, not on a compile or load fault; the CTest console output is not one of the report formats `gsd_run check tdd-red-evidence` reads, so no classifier verdict is recorded. GREEN `e69178c` made both pass; no REFACTOR commit. The tracer feat commit precedes the RED commit by design (it is the Task 1 slice, not a TDD task).

## Files Created/Modified

- `runner/diag.c`, `runner/diag.h` - diagnose state, per-frame step, the one-line print, the kind, device and board-source word tables.
- `runner/diag_class.h` - static inline bucket, order, pcslot and where helpers.
- `runner/expect.c`, `runner/expect.h` - observe and boots split out of the judge.
- `runner/args.c`, `runner/args.h`, `runner/main.c`, `runner/probe.c`, `CMakeLists.txt` - the option, the frame loop wiring, the moved board-source table.
- `tests/runner/diagnose.cmake`, `tests/runner/test_runner.c`, `tests/runner/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the new tests, the stop pin and the inventory rows.
- `docs/runner.md` - option row, synopsis line and the Diagnose section.

## Decisions Made

See key-decisions. `include/` is unchanged against origin/main (`git diff --quiet origin/main -- include` exits 0).

## Deviations from Plan

### Recorded deviation (from the plan objective)

**1. [Rule 10 - Branching] Phase branch cut from the milestone-branch tip**
- **Found during:** Task 1
- **Issue:** the milestone docs commits are on milestone/v2.1-failing-carts and not on origin/main, so CLAUDE.md rule 10's "branch off origin/main" would strand them.
- **Fix:** `phase/13-why-carts-fail` created from HEAD (the milestone tip), as the plan directs. The owner may override.

### Auto-fixed Issues

**2. [Rule 3 - Blocking] Diagnose state is heap-allocated, not on the stack**
- **Found during:** Task 1
- **Issue:** the plan put `cogmaster__diag` on the stack; with the 8192-byte RAM copy and the embedded expect it is about 15 KB, on top of the existing expect state, which risks wasm32's small stack.
- **Fix:** malloc beside the pixel buffer in `run()` only when `--diagnose`, freed on every exit path.
- **Files modified:** runner/main.c
- **Verification:** workflow green, `runner.diagnose` passes
- **Committed in:** c26dbec

**3. [Rule 1 - Bug] Test facts were non-contiguous substrings**
- **Found during:** Task 2 GREEN
- **Issue:** my first `runner.diagnose` fact strings spanned fields in the middle of the line and could not match.
- **Fix:** facts are now contiguous runs of the printed line.
- **Files modified:** tests/runner/diagnose.cmake
- **Committed in:** e69178c

**4. [Rule 3 - Blocking] diagnose.cmake trimmed to the 100-line cap**
- **Issue:** the first draft was 115 lines against the plan's "at most 100".
- **Fix:** folded the no-file usage case into the refused-option loop and shortened comments.
- **Committed in:** e69178c

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking) plus 1 recorded branching deviation
**Impact on plan:** none on scope; every acceptance criterion in the plan passes.

## Issues Encountered

`sed -i` with a pipe-delimited expression failed once on macOS during the probe.c include edit and was redone with a script; no effect on the tree.

## Known Stubs

None. The placeholder `diag_class.h` of the RED commit was replaced in `e69178c`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 02 can call `cogmaster-run --diagnose` from `library.sh diagnose` and parse the closed grammar `diagnose 1 ... source=...` with awk (numbers, enum words and `pcslot` as a digit, never hex). `ram=live|still` and `first_change` are available to the pass-row histogram.

## Self-Check: PASSED

Created files exist (runner/diag.c, runner/diag.h, runner/diag_class.h, tests/runner/diagnose.cmake); commits c26dbec, 27cbb53, e69178c and 53455f5 are ancestors of HEAD; `cmake --workflow --preset ci` exits 0.

---
*Phase: 13-why-carts-fail*
*Completed: 2026-11-12*
