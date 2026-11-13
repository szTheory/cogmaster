---
phase: 13-why-carts-fail
plan: 02
subsystem: library-judge
tags: [sh, awk, cmake, diagnose, library]

requires:
  - phase: 13-why-carts-fail
    provides: cogmaster-run --diagnose and its closed line (plan 01)
provides:
  - sh scripts/library.sh diagnose, a read-only branch that prints counts, leaves and catalog titles only
  - COGMASTER_DIAGNOSE block in scripts/library.cmake, one record per image with all output quiet
  - test library.diagnose (owned images, grammar, preflight, repeat call, ledger and page unchanged)
affects: [13-03, 13-04, 13-05, library diagnose]

actuals:
  tokens: 5500
  tasks: 2
  commits: 3
  plan_head_before: b44f7f6e924e5a58c03557cf7babb3b8c2da58a4
  plan_head_after: 6f906c7ab858e55ff4160e4790668808871900ec

tech-stack:
  added: []
  patterns:
    - "parse-and-reconstruct: every record field is matched against a closed pattern and the output is printf from validated values, never the record"
    - "one scratch directory from mktemp under build/, removed by an EXIT trap, no per-image file"
    - "tagged awk output lines (D, G, O) split and sorted by sh"

key-files:
  created:
    - tests/library/diagnose.cmake
  modified:
    - scripts/library.sh
    - scripts/library.cmake
    - tests/library/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "diagnose runs only the first NTSC profile library.cmake derives (Japanese FM and Game Gear Japan carts included) and skips only the PAL profile"
  - "diagnose runs only catalog rows with status unknown and pad/pad ports; other catalog rows count as skipped_excluded and uncatalogued images as uncatalogued"
  - "core= is the first 7 characters of the full commit id, plus +dirty when a tracked file outside .planning differs from HEAD"
  - "the retained output lives in COGMASTER_LIBRARY_OUT, default build/library/diag, under the same inside-build guard as run"

requirements-completed: []

coverage:
  - id: D1
    description: "sh scripts/library.sh diagnose prints four anchored line shapes and only fail rows, each fail row in one of D-17's leaves, with input_mismatch always printed"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D2
    description: "The counter and totals lines print every count (candidates, attempted, evaluated, malformed, uncatalogued, skipped_excluded, titles withheld) and the command exits 1 when groups_total differs from fail_total"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D3
    description: "A missing COGMASTER_ROM_DIR exits 2 with a fixed text on stderr, empty stdout and no value printed; the retained diagnose.txt equals stdout once CR is removed"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D4
    description: "Two diagnose calls print the same text and leave the stand-in ledger and page byte-identical, while the run command over the same inputs changes them"
    requirement: DIAG-01
    verification:
      - kind: integration
        ref: library.diagnose
        status: pass
    human_judgment: false
  - id: D5
    description: "The tracked ledger and page are unchanged against origin/main and the ci workflow is green"
    requirement: DIAG-01
    verification:
      - kind: other
        ref: git diff --quiet origin/main -- tests/compat/ledger.csv docs/compatibility.md and cmake --workflow --preset ci
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-11-12
status: complete
---

# Phase 13 Plan 02: library.sh diagnose Summary

**`sh scripts/library.sh diagnose` runs each catalogued default-profile cart through `cogmaster-run --diagnose`, validates every record against closed patterns in awk, classifies fail rows into the fixed cause leaves and prints counts, kinds and catalog titles only; `library.diagnose` pins the grammar, the counters, the preflight, repeat-call equality and the unchanged ledger and page.**

## Performance

- **Duration:** 11 min
- **Started:** 2026-11-12T23:01:00Z
- **Completed:** 2026-11-12T23:12:42Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- `scripts/library.sh`: the guard now lets `run` and `diagnose` through. The diagnose branch preflights `COGMASTER_ROM_DIR` (exit 2, fixed text), builds the runner only when `COGMASTER_LIBRARY_RUN` is unset (output discarded), keeps all scratch in one `mktemp -d` directory under an EXIT trap, reads the ledger only through `compat-common.sh cases`, runs the flat glob loop, reduces the records with one awk program and ends in `exit "$rc"` before the merge block. Four new `# Fault prevented:` gates.
- `scripts/library.cmake`: an early `COGMASTER_DIAGNOSE` block (probe to a variable, every stream quiet, catalog row matched and never split, status unknown and pad/pad only, first NTSC profile only, one `--diagnose` call with `TIMEOUT 60`) that returns before the `.path` write, so `run` is unchanged (`library.synthetic` passes).
- `tests/library/diagnose.cmake` (87 lines, cap 90) with `diag_call`, `diag_expect`, a table-driven image setup and six owned images: never.sms gives `dark_no_fault.never_draws`, in-a-00 gives `fault_no_display.unmapped_read.ctrl`, static-display gives `static_no_fault.frozen`, boot_to_play is a pass row and prints nothing, ed-00 is uncatalogued, jr-self is skipped. Totals: candidates=6 attempted=4 evaluated=4 malformed=0 uncatalogued=1 skipped_excluded=1 run_fail=3 groups_total=3 fail_total=3. attempted=4 also pins the PAL skip.

Measured: `library.diagnose` takes about 14 to 18 seconds locally on one arm64 macOS host (timeout stays 120 s, provisional; the comment at the registration says about 15 s). Registration added 6 lines to `tests/library/tests.cmake`. `cmake --workflow --preset ci` exits 0 (669 tests) and `tests/cmake/budgets.cmake` exits 0.

## Task Commits

1. **Task 1: end-to-end `library.sh diagnose` over one fail cart and one pass cart (tracer)** - `3935a62` (feat)
2. **Task 2 RED: failing owned scenario for the counters, repeat call and ledger check** - `539d570` (test)
3. **Task 2 GREEN: complete the diagnose counters** - `6f906c7` (feat)

**Plan metadata:** the docs commit that follows this file (SUMMARY, then STATE, ROADMAP and REQUIREMENTS).

Tracer gate: interactive, `human_verify_mode` end-of-phase, automated-only verify, so the tracer verify was re-run end to end (exit 0, `library.diagnose` and `library.synthetic` green) and expansion continued.

## TDD Gate Compliance

Task 2 carried `tdd="true"`. RED `539d570`: `library.diagnose` ran and failed on its planned assertion (`diagnose output lacks: candidates=6 attempted=4 evaluated=4 malformed=0 malformed_io=0 ...`, the Task 1 line printed `malformed=2` because `other` lines were counted as malformed). Semantic assessment: the target test ran, and failed on the intended behaviour, not on a load, syntax or fixture fault; the CTest console output is not one of the report formats `gsd_run check tdd-red-evidence` reads, so no classifier verdict is recorded. GREEN `6f906c7` made it pass; no REFACTOR commit. The Task 1 tracer feat commit precedes the RED commit by design.

## Files Created/Modified

- `scripts/library.sh` - the diagnose branch and the argument guard.
- `scripts/library.cmake` - the early diagnose block.
- `tests/library/diagnose.cmake` - the new test (CASE main).
- `tests/library/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - registration and inventory rows.

## Decisions Made

See key-decisions. The tracked ledger and page are unchanged against origin/main.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Test compressed to the 90-line cap by table and helper changes, with every assertion kept**
- **Found during:** Task 2
- **Issue:** the first full draft of `tests/library/diagnose.cmake` was 117 lines against the plan's cap of 90.
- **Fix:** `diag_call` is a macro with the CR removal in one place; the stand-in ledger is seeded from a header-only ledger and the page is plain text (diagnose never reads either, and the control run still changes both hashes); the four line shapes are one anchored alternation with distinct literal prefixes (a line can match at most one, and every line must match one) instead of a per-shape counting loop; the uncatalogued ed-00 image is copied outside the table and is not probed, since it is never run; comments became trailing comments.
- **Files modified:** tests/library/diagnose.cmake
- **Verification:** `wc -l` prints 87; `library.diagnose` passes
- **Committed in:** 539d570

**2. [Rule 1 - Bug] CMake match groups read inside the same condition that sets them**
- **Found during:** Task 1 (own first draft, before any run)
- **Issue:** `CMAKE_MATCH_n` referenced in the same `if()` as the `MATCHES` that sets them would expand before the match runs.
- **Fix:** match first, then read the groups on the next lines.
- **Files modified:** scripts/library.cmake
- **Committed in:** 3935a62

---

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking)
**Impact on plan:** none on scope; every listed assertion is kept and every acceptance criterion passes.

## Issues Encountered

None. `pcslot` is validated by the awk but, as D-09 lists the printed fields, it is not printed.

## Known Stubs

None.

## Threat Flags

None. The new surface (reading `COGMASTER_ROM_DIR`, one scratch directory) is the one in the plan's threat model.

## User Setup Required

None - no external service configuration required. The real library run (plan 05) needs the owner to pass `COGMASTER_ROM_DIR`.

## Next Phase Readiness

Plan 03 can extend the same awk (it reads the `cases` file the ledger gate already produces) with the sub-rows, the group order, the cross-cuts, the pass-row histogram and the ledger fields between `run_fail` and `groups_total`, and can reuse `diag_call` and `diag_expect` from `tests/library/diagnose.cmake` (87 of the 156-line cap after plan 03).

## Self-Check: PASSED

Created files exist (tests/library/diagnose.cmake); commits 3935a62, 539d570 and 6f906c7 are ancestors of HEAD; `cmake --workflow --preset ci` exits 0; `git diff --quiet origin/main -- tests/compat/ledger.csv docs/compatibility.md` exits 0.

---
*Phase: 13-why-carts-fail*
*Completed: 2026-11-12*
