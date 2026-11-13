---
phase: 14-fix-the-causes
plan: 06
subsystem: io-faults
tags:
  - io
  - fault
  - open-bus
  - diagnose

requires:
  - phase: 14-fix-the-causes
    provides: the post-plan-05 diagnose counts (run_fail 69; ranks 4 and 5 at 12 and 8) and the decision table rows for the two port kinds
provides:
  - a read of an undriven port in $00-$3F returns open bus, still counts in the open-bus counter, and records no fault
  - a write to a port nothing decodes records no fault, after the D-11 stop check found no trigger
  - the seeds and tests that used the port-read fault repointed to the undefined ED opcode so each still proves a fault path
  - docs/embedding.md rows 1 and 2, the never-faults list and the revision-18 sentence updated
  - outcome rows for ranks 4 and 5 with before and after counts
affects: [14-07, 14-08]

actuals:
  tokens: 6100
  tasks: 3
  commits: 3
plan_head_before: e389ed50d7c94c853713f9ca7a435154ecd153bd
plan_head_after: a7fe8e075cd7821aadc1c1ca6e648e5db923c0a1

tech-stack:
  added: []
  patterns:
    - "a port nothing drives reads as open bus and a port nothing decodes ignores a write; neither is the first fault, so the record stays free for a real one"

key-files:
  created: []
  modified:
    - src/io.c
    - tests/bus/test_bus.c
    - tests/io/test_io.c
    - tests/api/v2.c
    - tests/runner/expect.cmake
    - tests/runner/tests.cmake
    - tests/runner/diagnose.cmake
    - tests/library/diagnose.cmake
    - docs/embedding.md
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

key-decisions:
  - "D-11 trigger not met: rank 4's only pclass sub-row is 3e-3f:gg:export (8), with no sms or sms1 row in region japan, so the write half is dropped; the drop remains an inference (no document lists a fault for such a write)"
  - "The COGMASTER_FAULT_KIND_UNMAPPED_READ and UNMAPPED_WRITE constants and the internal kinds stay: both still fire memory-side (cartridge or RAM disabled by port $3E)"
  - "The in-a-00 seed is kept as the uncatalogued library canary copy, since no catalog row holds its SHA-1, and the library fault row moves to ed-00"

requirements-completed: []

coverage:
  - id: C1
    description: "A read of an undriven port records no fault, returns $FF and counts once in the open-bus counter on the export console"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "tests/bus/test_bus.c#test_fault_unmapped_read (ctest fault.unmapped_read), exit status 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "A write to a port nothing decodes records no fault: OUT ($3E) on the Game Gear and OUT ($00) on the Japanese console, and the $02 mirror write in Game Gear SMS mode leaves mem_ctrl alone"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "tests/bus/test_bus.c#test_fault_unmapped_write (ctest fault.unmapped_write), exit status 0"
        status: pass
      - kind: unit
        ref: "tests/io/test_io.c#test_gg_sms_mode (ctest io.gg_sms_mode), exit status 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The memory-side positive controls still fault: port $3E cart or RAM off, mapper slot-off cases and the glasses ports"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest fault.*, io.port3e, mapper.slot_off and ports.glasses.*, exit status 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "The public API still names every fault constant and reports the ED_UNDEFINED kind on the CPU device for ED 00, with the new program's byte peeked at $0002"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "tests/api/v2.c (ctest abi.api_v2 and abi.api_version), exit status 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The runner prints the ed_undefined first fault for the repointed seed on --trace first-fault and the expectation run, stops --png past the fault, and prints fault=none for the in-a-00 seed"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "ctest runner.trace.fault, runner.png_unreached, runner.expect.predicates and runner.diagnose, exit status 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "The library diagnose main case keeps a fault row (ed_undefined on the CPU) and an uncatalogued canary that no catalog row holds, with its totals unchanged"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "ctest library.diagnose and library.diagnose.*, exit status 0"
        status: pass
    human_judgment: false
  - id: C7
    description: "The diagnose re-run on the clean plan head: ranks 4 and 5 no longer print, run_fail 69 to 49, run_fail_not_in_ledger 0, ledger_fail_now_pass 136 to 156"
    requirement: FIX-04
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose from the repository root at 4e7f825, exit status 0; counts in 14-FIX-OUTCOMES.md"
        status: pass
    human_judgment: false
  - id: C8
    description: "The full workflow passes on the plan head"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit status 0 (676 tests and 5 release tests pass)"
        status: pass
    human_judgment: false

duration: 75min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 06: Port reads and writes stop faulting Summary

**The two port raise sites in src/io.c are removed so a read of an undriven port is open bus and a write to a port nothing decodes is ignored, neither recorded as the first fault; the diagnose re-run drops ranks 4 (12 to 0) and 5 (8 to 0) and the total run_fail from 69 to 49 with no leaf growing.**

## Performance

- **Duration:** 75 min (most of it the diagnose run over the dump set)
- **Completed:** 2026-11-13
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments

- A read of `$00`-`$3F` that nothing drives returns open bus, counts in the open-bus counter and records no fault; the shipped-game behaviour is now consistent with the controller-port open bus the docs already promised.
- D-11's stop check ran before the write drop: rank 4's baseline `pclass` split is `3e-3f:gg:export` 8 only, so there were 0 mirror writes on a Japanese full-decode profile, the trigger did not fire, and a write to a port nothing decodes now records no fault (an inference, tagged so in the decision table).
- Negative controls: fault.unmapped_read (`present == 0`, `A == $FF`, `open_bus_reads == 1`), fault.unmapped_write (Game Gear `OUT ($3E)`, Japanese `OUT ($00)`, export `OUT ($00)`), io.gg_sms_mode (no fault after the `$02` mirror write, `mem_ctrl` unchanged), runner.diagnose (`fault=none` for in-a-00).
- Positive controls kept: the memory-side unmapped read and write cases (port `$3E` cart or RAM off, mapper slot-off, glasses) still fault, and ED 00 is the first fault in abi.api_v2, runner.trace.fault, runner.png_unreached, runner.expect.predicates, runner.diagnose and library.diagnose, so no test went vacuous.
- Re-run on a clean tree: run_fail 69 to 49, run_fail_not_in_ledger 0, ledger_fail_now_pass 136 to 156. All 20 rows that left ranks 4 and 5 now pass; ranks 2, 6, 9 and 10 are unchanged (40, 6, 2, 1), so no dark_no_fault or static_no_fault leaf grew.

## Task Commits

1. **Task 1 (tracer): port reads are open bus with no fault** - `e83f9c1` (fix) - src/io.c, tests/bus/test_bus.c, tests/api/v2.c, tests/runner/expect.cmake, tests/runner/tests.cmake, tests/runner/diagnose.cmake, tests/library/diagnose.cmake, docs/embedding.md
2. **Task 2: writes to ports nothing decodes, after the D-11 check** - `4e7f825` (fix) - src/io.c, tests/bus/test_bus.c, tests/io/test_io.c, docs/embedding.md
3. **Task 3: re-run and record ranks 4 and 5** - `a7fe8e0` (docs) - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

**Plan metadata:** the commit that carries this file (docs: complete plan)

The tests of tasks 1 and 2 were flipped before the core change. RED, semantic assessment: fault.unmapped_read failed on `fault_of(m).present == 0` (1 != 0) and runner.diagnose on the in-a-00 `fault=none` expectation (the run printed `fault=unmapped_read`) for task 1; fault.unmapped_write and io.gg_sms_mode failed on the no-fault assertions for task 2 (io.gg_sms_mode: `present == 0 failed: 1 != 0`). Each failed on the planned assertion for the planned reason, and each passed after the raise was removed (GREEN). The plan is `type: execute`, so the red and green steps share the one commit the plan names for each task, with no separate `test` commit.

## Files Created/Modified

- `src/io.c` - the `p < 0x40u` read raise and the `p < 0xC0u` write raise are gone, both comments rewritten as hardware comments; the trailing `return;` that the write drop left redundant was removed
- `tests/bus/test_bus.c`, `tests/io/test_io.c`, `tests/api/v2.c` - assertions flipped, API test program moved to `ED 00; DI; HALT` with its peek of `$0002` now `$F3`
- `tests/runner/expect.cmake`, `tests/runner/tests.cmake`, `tests/runner/diagnose.cmake`, `tests/library/diagnose.cmake` - seeds repointed to ed-00, the in-a-00 diagnose line made the negative control, and the library canary copy moved to in-a-00
- `docs/embedding.md` - rows 1 and 2 drop the port clauses, the never-faults list and the revision-18 sentence gain the two clauses (36,791 bytes, under the 38,912 budget)
- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - rank 4 and rank 5 rows and a Plan 06 re-run paragraph, counts only

## Decisions Made

See key-decisions. The tests/library/diagnose.cmake reducer cases that feed synthetic diagnose text (`unmapped_read` leaf lines) stay as they are: they exercise the reducer on text, not the core, and the leaf names remain valid grammar.

## Deviations from Plan

None - plan executed exactly as written. The orchestrator's note applied: the plan's older quoted counts (rank 4 at 8 and rank 5 at 7) are kept in the outcome rows as the Phase 13 baseline, and the re-run's starting counts are the post-plan-05 figures (rank 4 at 12, rank 5 at 8).

## Issues Encountered

None. D-11's trigger did not fire, so the plan's gap-round branch was not taken and plan 08 has nothing to list for rank 4. FIX-01 stays out of `requirements-completed` as the plan words it (plan 08 closes the phase).

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- Commits `e83f9c1`, `4e7f825` and `a7fe8e0` are ancestors of HEAD; `git rev-list --count` over the plan range measured 3.
- Task acceptance greps exit 0, the task 3 verify command exits 0, and the plan-level `cmake --workflow --preset ci` exits 0 (676 tests and 5 release tests pass).
- `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md .github/workflows/ci.yml tests/inventory/ci.txt` exits 0, and no CTest was renamed.
- The note holds no file name, dotted machine token, title field or home path.
