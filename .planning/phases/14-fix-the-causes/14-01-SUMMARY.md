---
phase: 14-fix-the-causes
plan: 01
subsystem: runner-diagnostic
tags: [diagnose, runner, library-reducer, closed-vocabulary, leak-scan]

requires:
  - phase: 13-explain-the-failures
    provides: the cogmaster-run --diagnose line and the sh scripts/library.sh diagnose reducer
provides:
  - seven closed-vocabulary evidence words after source= on the diagnose line (vdpmode, faddr, pow2, wclass, pclass, size, header)
  - counts-only colon-joined class sub-rows under each failing leaf of the library reducer
  - the base hashes file build/p14-base-hashes.txt taken from origin/main before any edit
affects: [14-02, 14-03, 14-04, 14-05, 14-06, 14-07, 14-08]

actuals:
  tokens: 9900
  tasks: 2
  commits: 2
plan_head_before: 84798d496e1758ca37bc6c54e6345e5ba139b87f
plan_head_after: 6c26fb07f7e0a2df8b376e8250cf575914595f49

tech-stack:
  added: []
  patterns:
    - "pure class functions in runner/diag_class.h, one per diagnose word, each returning a literal from a closed list"
    - "compound sub-row keys join their parts with a colon so the leak scan name pattern never fires on a class word"

key-files:
  created: []
  modified:
    - runner/diag_class.h
    - runner/diag.c
    - tests/runner/test_runner.c
    - tests/runner/diagnose.cmake
    - scripts/library.sh
    - tests/library/diagnose.cmake
    - docs/runner.md
    - docs/conformance.md

key-decisions:
  - "Sub-rows of the new labels sort by the key column holding the class value itself (byte order under LC_ALL=C), not a constant 0: the order is the same as the plan states (label index, then value byte order) and needs no whole-line tie break"
  - "pow2 rounds the body up to 16 KiB pages before testing the power of two, so a 16385-byte body is two pages and answers y; a body of 0 bytes answers n"
  - "The first fault example in docs/runner.md now shows the ed_undefined line of the ed-00 seed (dev=cpu), taken from the build tree"

patterns-established:
  - "A canned library test line ends with one shared tail variable of the words every record carries, with per-row variants made by string(REPLACE)"

requirements-completed: []

coverage:
  - id: D1
    description: "Seven pure class functions for the diagnose words, each closed and checked at both ends of every range"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest runner.diagnose.class"
        status: pass
    human_judgment: false
  - id: D2
    description: "The diagnose line carries the seven words after source= and stays one closed line with no value, address, byte, size or name"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "ctest runner.diagnose"
        status: pass
      - kind: integration
        ref: "ctest runner.trace.fault"
        status: pass
    human_judgment: false
  - id: D3
    description: "The library reducer validates every new word against its closed list and prints counts-only colon-joined sub-rows per leaf"
    requirement: FIX-03
    verification:
      - kind: integration
        ref: "ctest library.diagnose"
        status: pass
      - kind: integration
        ref: "ctest library.diagnose.taxonomy"
        status: pass
      - kind: integration
        ref: "ctest library.diagnose.hostile"
        status: pass
    human_judgment: false
  - id: D4
    description: "The leak cases still fail by name on their plants and no class word trips the scan"
    requirement: FIX-04
    verification:
      - kind: integration
        ref: "ctest library.diagnose.leak_name library.diagnose.leak_path library.diagnose.leak_bytes library.diagnose.leak_digest library.diagnose.leak_title"
        status: pass
    human_judgment: false
  - id: D5
    description: "The base hashes file was taken from origin/main before any edit of the phase"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "ctest --test-dir build/ci -R '^runner.hashes_file$' then cp build/ci/hashes.txt build/p14-base-hashes.txt, exit 0 and 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "The runner and conformance docs list the words as classes, and docs.budgets stays inside its limits"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "ctest docs.budgets docs.api_names policy.claims_scoped"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-12
status: complete
---

# Phase 14 Plan 01: The evidence words Summary

**Seven closed-vocabulary class words (video mode, mapper address, power-of-two pages, write address, port, size, header) on the --diagnose line, validated by the library reducer and printed as colon-joined counts-only sub-rows**

## Performance

- **Duration:** about 10 min
- **Started:** 2026-11-13T03:18:00Z
- **Completed:** 2026-11-13T03:28:00Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- `cogmaster-run --diagnose` prints `vdpmode`, `faddr`, `pow2`, `wclass`, `pclass`, `size` and `header` after `source=`, each a word from a fixed list. Every function is a pure function in `runner/diag_class.h`, and `runner.diagnose.class` checks all sixteen mode nibbles and both ends of every range.
- `scripts/library.sh diagnose` now requires 32 fields per record, rejects a word outside its list as malformed, and prints `faddr`, `vdpmode`, `wclass`, `pclass` and `size` sub-rows under each leaf. Keys join their parts with `:`; a live row from a generated nibble-0 program prints `vdpmode=graphics1:sms:none rows=1` and passes the leak scan on every channel.
- The base hashes file `build/p14-base-hashes.txt` (173 lines, ignored by git) was taken from origin/main commit `81a9cc9761873f20910738e393bed8b3558822b6` before any edit: `git diff --quiet origin/main HEAD -- src include runner scripts tests` exited 0, the build exited 0 and `runner.hashes_file` passed (exit 0). Plans 03, 04, 07 and 08 can compare against it directly.
- docs/runner.md lists the seven words as classes (never a byte count) and its first fault example shows a live line from the build tree; docs/conformance.md names the evidence classes as counts only. docs/runner.md is 37,844 bytes against the 38,912 limit.

## Task Commits

1. **Task 1: vdpmode end to end (tracer)** - `8e8c6f2` (fix)
2. **Task 2: the other six words, sub-rows and docs** - `6c26fb0` (fix)

**Plan metadata:** the docs commit that carries this file, STATE.md and ROADMAP.md.

## Files Created/Modified

- `runner/diag_class.h` - the seven class functions, with their hardware sources cited
- `runner/diag.c` - the line gains the seven words; the body size is `image_size - body_offset`
- `tests/runner/test_runner.c` - diag_class() covers every nibble, range end and none case
- `tests/runner/diagnose.cmake` - the line shape, a generated nibble-0 program, and per-seed word expectations
- `scripts/library.sh` - the 32-field validator and five new sub-row labels
- `tests/library/diagnose.cmake` - a shared tail for canned lines, one live catalog row, the extended grammar and one sub-row expectation per label
- `docs/runner.md`, `docs/conformance.md` - the words and the live first fault example

## Decisions Made

See key-decisions above. The plan's `vdpmode` note holds: until plan 02 puts the mode nibble in `addr`, the core records 0, so every unsupported-mode fault reads `graphics1`; the nibble-0 tracer fixture is correct on both sides of plan 02.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] pow2 expectation in my own test data**
- **Found during:** Task 2 (GREEN)
- **Issue:** the first test table said a 16385-byte body is not a power of two; it rounds up to two 16 KiB pages, which is one.
- **Fix:** the test row now expects `y`, matching the plan's own definition (rounded up to pages).
- **Files modified:** tests/runner/test_runner.c
- **Verification:** `runner.diagnose.class` passes
- **Committed in:** 6c26fb0

**2. [Rule 3 - Blocking] leak_title canned line needed the full tail**
- **Found during:** Task 2
- **Issue:** the one canned line of the title leak case carried only `vdpmode=none` from Task 1, so the 32-field validator counted it malformed and the planted title never printed.
- **Fix:** it carries the same shared words as the other canned lines.
- **Files modified:** tests/library/diagnose.cmake
- **Verification:** `library.diagnose.leak_title` passes
- **Committed in:** 6c26fb0

**3. [Plan note] TDD cycle kept inside one commit**
- Task 2 is `tdd="true"`. RED was run locally with stub functions returning fixed words (the test built and failed on its assertions), then GREEN. The plan names one commit for the task, and a commit with a failing test would break bisect, so no separate `test(...)` commit exists. `workflow.tdd_mode` is false.

**4. [Plan note] sub-row sort key**
- The plan says the new labels print with sort key 0 in the numeric column. The reducer already prints the class value in the key column for every non-`where` label, and under `LC_ALL=C` that column is a byte-order key, so no change to the print line was needed: the order is label index, then value byte order, as the plan states.

---

**Total deviations:** 2 auto-fixed (1 bug in test data, 1 blocking), 2 plan notes
**Impact on plan:** none on scope; no public header, flag, CTest name or repository-check code was added.

## Issues Encountered

- `ctest` reported the `runner.hashes_file` file unchanged on disk (the writer does not rewrite identical content), so `build/ci/hashes.txt` kept yesterday's mtime; its content is the origin/main code's, since `src include runner scripts tests` were identical to origin/main at HEAD.

## Commands and exit statuses

- `git diff --quiet origin/main HEAD -- src include runner scripts tests`: 0
- `cmake --preset ci` and `cmake --build --preset ci`: 0
- `ctest --test-dir build/ci -R '^runner\.hashes_file$' --output-on-failure --no-tests=error`: 0, then `cp build/ci/hashes.txt build/p14-base-hashes.txt`: 0
- Task 1 verify (`runner.diagnose`, `runner.diagnose.class`, `runner.trace.fault`, `library.diagnose*`): 0, 11 tests passed
- Task 2 verify (above plus `runner.exit.table`, `docs.budgets`, `docs.api_names`, `policy.claims_scoped`): 0, 14 tests passed; `test "$(wc -c < docs/runner.md)" -lt 38912`: 0; `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake`: 0
- All acceptance greps of both tasks: 0
- `cmake --workflow --preset ci`: 0 (run locally on the plan head 6c26fb0, no hosted run URL for this plan)

## Known Stubs

None.

## Threat Flags

None. The new words are class words from fixed lists; the leak scan and its five by-name cases pass unchanged.

## Next Phase Readiness

Plan 02 can put the mode nibble in the fault address and add the non-zero nibble check; it then runs the baseline and writes the outcomes note against these words. The base hashes file is in place for the pin compares of plans 03, 04, 07 and 08.

## Self-Check: PASSED

- FOUND: runner/diag_class.h, runner/diag.c, scripts/library.sh, tests/library/diagnose.cmake, tests/runner/diagnose.cmake, tests/runner/test_runner.c, docs/runner.md, docs/conformance.md
- FOUND commits: 8e8c6f2, 6c26fb0 (both ancestors of HEAD)
- FOUND: build/p14-base-hashes.txt (git-ignored)
