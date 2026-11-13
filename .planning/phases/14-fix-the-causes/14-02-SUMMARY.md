---
phase: 14-fix-the-causes
plan: 02
subsystem: vdp-fault-record
tags: [vdp, fault-record, behaviour-revision, diagnose, baseline]

requires:
  - phase: 14-fix-the-causes
    provides: the vdpmode diagnose word and the counts-only class sub-rows of plan 01
provides:
  - the video mode select nibble M4 M3 M2 M1 in the addr of an unsupported-video-mode fault
  - behaviour revision 18, pinned in the core test and documented
  - the baseline run (run_fail 205, run_fail_not_in_ledger 0) with its class sub-rows
  - 14-FIX-OUTCOMES.md with the decision table and the rank-ordered outcome table
affects: [14-03, 14-04, 14-05, 14-06, 14-07, 14-08]

actuals:
  tokens: 4393
  tasks: 2
  commits: 2
plan_head_before: 69a825bd971ae5517fb8c4479afd747aa8425104
plan_head_after: 0619e310b92245144a595bd432771d72f3971158

tech-stack:
  added: []
  patterns:
    - "one static helper (vdp_mode) names the mode nibble for both the support test and the fault address"

key-files:
  created:
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md
  modified:
    - src/vdp_render.c
    - include/cogmaster.h
    - tests/core/test_core.c
    - tests/vdp/test_background.c
    - tests/runner/diagnose.cmake
    - docs/embedding.md
    - docs/hardware/video.md

key-decisions:
  - "The nibble rides in the existing addr field: no header field, no state format change, and no change to whether, when or where the fault is recorded"
  - "Nibble 0 (Graphics I) is a valid addr value for this kind; the end-to-end check uses nibble 2 so a nibble left at 0 fails"
  - "The baseline was run through scripts/library.sh diagnose, which reads the dump directory from the project's own ignored local settings, so no value was read or printed by the executor"

requirements-completed: []

coverage:
  - id: D1
    description: "An unsupported-video-mode fault records the mode select nibble in addr for every unsupported nibble on the SMS2 and SMS1 profiles, and a supported nibble records no fault"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest vdp.background.unsupported"
        status: pass
    human_judgment: false
  - id: D2
    description: "Behaviour revision 18 is pinned in the core test and the header carries only the revision line as a change"
    requirement: FIX-04
    verification:
      - kind: unit
        ref: "ctest core.* (tests/core/test_core.c pins 18u)"
        status: pass
    human_judgment: false
  - id: D3
    description: "cogmaster-run --diagnose on a program selecting Graphics II prints vdpmode=graphics2 end to end"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "ctest runner.diagnose"
        status: pass
    human_judgment: false
  - id: D4
    description: "The baseline reproduces run_fail 205 and run_fail_not_in_ledger 0 on a clean tree with the nibble in the record, and its class sub-rows are in the outcomes note"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose, exit status 0, then test with -eq on both totals, exit status 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The outcomes note holds the header, the per-kind decision table with one hardware source per line, the baseline classes and the eleven-rank outcome table in the Phase 13 order, counts only"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "Task 2 acceptance greps and the rank awk check, exit status 0"
        status: pass
    human_judgment: false

duration: 85min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 02: The video mode in the fault record Summary

**The unsupported-video-mode fault now carries the mode select nibble in addr (behaviour revision 18), the baseline of 205 failing rows is reproduced with real vdpmode classes, and 14-FIX-OUTCOMES.md holds the hardware-sourced decision table**

## Performance

- **Duration:** about 85 min, of which about 17 min was the library baseline run
- **Completed:** 2026-11-13T03:50:00Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- `vdp_mode` in `src/vdp_render.c` names M4 M3 M2 M1 once; `vdp_supported` and the fault raise both use it, so the record's `addr` is the nibble. The fault's condition, device, kind and position are unchanged.
- `COGMASTER_BEHAVIOUR_REVISION` is 18 and `tests/core/test_core.c` pins it; `docs/embedding.md` opens the revision 18 sentence and says `addr` holds the nibble for kind 6 and that 0 is valid; `docs/hardware/video.md` says the same in one sentence. The header diff against origin/main is the revision line only.
- `vdp.background.unsupported` asserts `addr == mode` for every unsupported nibble on both profiles and asserts no fault for every supported nibble. `runner.diagnose` has a Graphics II program that prints `vdpmode=graphics2`; before the edit that call read `graphics1`.
- The baseline ran on a clean tree at core commit `faf9cc7`: `run_fail 205`, `run_fail_not_in_ledger 0`. With the nibble in the record the 36 static rank-2 rows split into graphics2 (29) and graphics1 (7), and all 6 rank-6 rows are graphics1; the class sub-rows are pasted into the note by script, not retyped.
- `14-FIX-OUTCOMES.md` has the header, the decision table (eight lines, each citing a SMS-* section or a DEC entry), the baseline classes, the outcome table with ranks 1 to 11 and leaf names as the Phase 13 note prints them, the work order line and the pins stub. All outcomes read `pending`.

## Task Commits

1. **Task 1: the video mode nibble end to end** - `faf9cc7` (fix)
2. **Task 2: baseline run and the outcomes note** - `0619e31` (docs)

**Plan metadata:** the docs commit that carries this file, STATE.md and ROADMAP.md.

## Files Created/Modified

- `src/vdp_render.c` - `vdp_mode` helper; the raise passes the nibble as the address
- `include/cogmaster.h` - the revision line only
- `tests/core/test_core.c` - the revision pin and its comment
- `tests/vdp/test_background.c` - `addr == mode` for unsupported nibbles; a negative control for supported ones
- `tests/runner/diagnose.cmake` - the Graphics II program and its `vdpmode=graphics2` expectation; fixture counts 8 and 11
- `docs/embedding.md`, `docs/hardware/video.md` - the nibble and revision 18
- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - the outcomes record

## Decisions Made

See key-decisions above.

## Deviations from Plan

None - plan executed exactly as written. Two notes on the dump branch, neither a deviation:

- `COGMASTER_ROM_DIR` was absent from the shell, so no ask was needed to proceed: `scripts/library.sh diagnose` resolves it from the project's ignored local settings file by design, which the executor never read or printed. A direct check of that file by the executor was refused by the secret-read guard and was not retried; the script run was the plan's own command.
- The baseline run happened after Task 1, so its `vdpmode` classes already show the nibble (graphics2 rows that the Phase 13 run recorded as graphics1).

## Issues Encountered

None.

## Commands and exit statuses

- `cmake --preset ci` and `cmake --build --preset ci`: 0
- Task 1 verify (ctest over `vdp.background.unsupported`, `core.*`, `runner.diagnose`, `runner.trace.fault`, `library.diagnose*`, `docs.api_names`, `docs.budgets`, `policy.claims_scoped`): 0, 43 tests passed
- Task 1 acceptance greps and the header-diff check: 0; `wc -c docs/embedding.md` is 36701, under 38912
- `sh scripts/library.sh diagnose`: 0, from the repository root; `test "$rf" -eq 205 && test "$rnl" -eq 0`: 0
- Task 2 verify and acceptance (note structure, source count, rank awk, ledger and compatibility page and ci.yml unchanged against origin/main): 0
- `cmake --workflow --preset ci`: 0 (local, on `0619e31`; no hosted run URL for this plan)

## Known Stubs

None. The `pending` cells of the outcome table are filled by plans 03 to 08 by design.

## Threat Flags

None. The note holds counts and class words only; no title, name, path, byte or digest was written.

## Next Phase Readiness

Plan 03 can start on ranks 1 and 8 against the baseline rows in the note and `build/p14-base-hashes.txt`. Later plans extend the revision 18 paragraph in `docs/embedding.md` and do not bump the revision again.

## Self-Check: PASSED

- FOUND: src/vdp_render.c, include/cogmaster.h, tests/core/test_core.c, tests/vdp/test_background.c, tests/runner/diagnose.cmake, docs/embedding.md, docs/hardware/video.md, .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md
- FOUND commits: faf9cc7, 0619e31 (both ancestors of HEAD)
