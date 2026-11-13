---
phase: 14-fix-the-causes
plan: 03
subsystem: sega-mapper
tags: [mapper, fault-record, sega-board, diagnose]

requires:
  - phase: 14-fix-the-causes
    provides: the baseline faddr class sub-rows and the rank-ordered outcome table of plans 01 and 02
provides:
  - a Sega $FFFC write stores every bit and records no fault
  - fault.mapper_range with the boundary pair and the stored-value check
  - docs row 4 and the never-faults list updated, revision 18 sentence extended
  - measured outcome rows for ranks 1 and 8 (both leaves 0 after) and the shift decision line
affects: [14-08]

actuals:
  tokens: 2300
  tasks: 3
  commits: 5
plan_head_before: 8b6f1868853bbf9d33bd4181b34257e7277c66dc
plan_head_after: f439bae78e48b799fb083c4a2376229bd42994eb

tech-stack:
  added: []
  patterns:
    - "the bank-past-the-end marker stays in bank_write, the only mapper_range raise left on the Sega board"

key-files:
  created: []
  modified:
    - src/mapper.c
    - tests/bus/test_bus.c
    - docs/embedding.md
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

key-decisions:
  - "D-05 stop rule not met: the baseline holds faddr=fffc:y 109 and faddr=fffd-f:n 0 for rank 1, so the plan proceeds"
  - "D-06 trace not taken (it needs the dump directory path, which was not read); the Sega bank shift is left as it is and no pin moved"
  - "The diagnose re-run measured rank 1 at 0 and rank 8 at 0 (run_fail 205 to 104, run_fail_not_in_ledger 0), so both read fixed in the core"

requirements-completed: []

coverage:
  - id: D1
    description: "A write to $FFFC with bit 7, 4, 1 or 0 set records no fault on a Sega board and the value is stored in the control register"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest fault.mapper_range"
        status: pass
    human_judgment: false
  - id: D2
    description: "A bank number at or past the page count of a non-power-of-two image still records mapper_range at the register address, with 24 faulting and 23 not on a 24-page image"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest fault.mapper_range"
        status: pass
      - kind: unit
        ref: "ctest eeprom.board.banks"
        status: pass
    human_judgment: false
  - id: D3
    description: "The Sega bank shift on 512 KiB is unchanged"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest mapper.sega.shift"
        status: pass
    human_judgment: false
  - id: D4
    description: "D-06 trace of the written value's bits 1-0 on one failing cart: not taken, so no shift gate was added; the question stays open"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "not taken: the trace needs the dump directory path"
        status: unknown
    human_judgment: false
  - id: D5
    description: "Diagnose re-run: rank 1 leaf 109 to 0, rank 8 leaf 1 to 0, run_fail 205 to 104, run_fail_not_in_ledger 0"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose, exit status 0, on a clean tree at a head whose core source equals eb8dfee"
        status: pass
    human_judgment: false
  - id: D6
    description: "The full workflow passes on the plan head"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit status 0"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 03: The $FFFC unused-bit fault Summary

**Sega `$FFFC` writes to the unused bits 7, 4 and 1-0 now store the value and record no fault, while the bank-past-the-end marker on non-power-of-two images stays; a diagnose re-run measures both ranks at 0 leaf rows (run_fail 205 to 104).**

## Performance

- **Duration:** 12 min
- **Started:** 2026-11-13T03:40:00Z
- **Completed:** 2026-11-13T03:52:11Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments

- Removed the `$FFFC` unused-bit raise and its mask define from `src/mapper.c`; the store `p->ctrl = v` stays with a one-line hardware comment citing SMS-CARTRIDGES section 2.1.
- `fault.mapper_range` now asserts no fault and `m->mapper.ctrl` equal to the value for `$80`, `$10`, `$01`, `$02`, `$03` on a 512 KiB image, and gained the boundary case (bank 24 of 24 faults at `$FFFF`; bank 23 does not).
- Docs: the Faults table row 4 drops the `$FFFC` clause, the never-faults list names it, and the revision 18 sentence carries it. `docs/embedding.md` is 36,746 bytes, under the 38,912 budget.
- The outcomes note gives ranks 1 and 8 their rows. The D-05 stop rule was read from the baseline: faddr=fffc:y 109, fffd-f:n 0, fffd-f:y 0 for rank 1, so it did not fire.
- Re-run of `sh scripts/library.sh diagnose` after the fix: rank 1 leaf 109 to 0 and rank 8 leaf 1 to 0, so both read `fixed in the core`. Totals: run_fail 205 to 104, run_fail_not_in_ledger 0, ledger_fail_now_pass 101. Nine rows landed on later faults: rank 3 32 to 36, rank 4 8 to 11, rank 5 7 to 8, rank 9 1 to 2 (consistent with 110 moved rows = 101 passing + 9 exposed; the per-cart link was not traced).

## Task Commits

1. **Task 1: stop recording `$FFFC` unused-bit writes** - `eb8dfee` (fix)
2. **Task 2: the Sega shift gate** - no commit; the D-06 trace was not taken (dumps absent), so the shift stays as it is and no code changed
3. **Task 3: record ranks 1 and 8** - `cf2fda6` (docs), then the measured counts in `f439bae` (docs) after the diagnose re-run

**Plan metadata:** the commit that carries this file (docs: complete plan)

## Files Created/Modified

- `src/mapper.c` - the `$FFFC` branch stores every bit with no fault
- `tests/bus/test_bus.c` - positive and negative controls for both mapper_range raise sites
- `docs/embedding.md` - fault table row 4, never-faults list, revision 18 sentence
- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - rank 1 and 8 rows, shift decision line, pins line

## Decisions Made

- The baseline was measured, so the D-05 gate was read from it and did not fire. The shift stays untouched: the D-06 trace needs a dump file, and its path was not read. The re-run shows no sign by itself either way: 101 of 110 rows pass and 9 landed on later faults in ranks 3, 4, 5 and 9, which a wrong bank shift could also produce. Whether to take the trace is left to the coordinator.

## Deviations from Plan

None in the code - plan executed as written. The diagnose re-run was first skipped in error: `COGMASTER_ROM_DIR` was not exported in the shell, but `scripts/library.sh` reads it from the ignored dotenv file itself. After the correction the re-run was taken and the outcome rows were rewritten from `fixed, unconfirmed on dumps` to the measured counts.

## Verification

- `cmake --preset ci` then build then `ctest -R '^(fault|mapper|eeprom|core).*|docs.api_names|docs.budgets|policy.claims_scoped'`: exit 0, 69 of 69 passed.
- `cmake --workflow --preset ci`: exit 0 (run again after the final edits: exit 0).
- `sh scripts/library.sh diagnose` from the repository root on a clean tree at head 97f345c (core source equal to eb8dfee): exit status 0, run_fail 104, run_fail_not_in_ledger 0 (compared as numbers).
- Task 1 acceptance greps and Task 3 verify command: exit 0. `git diff --exit-code origin/main` on the ledger, compatibility page and ci.yml: exit 0.
- Commands the plan forbids (`scripts/library.sh` with no argument or `run`, the library workflow) were not run.

## Issues Encountered

None.

## Known Stubs

None.

## Next Phase Readiness

- Plan 08 lists nothing under Gap round for ranks 1 and 8: both leaves are 0.
- Ranks 3, 4, 5 and 9 start from 36, 11, 8 and 2 rows, not 32, 8, 7 and 1.
- The Sega shift question (bits 1-0 of the written value) is open until a trace is taken on one former `$FFFC` cart.

## Self-Check: PASSED

- FOUND: `src/mapper.c`, `tests/bus/test_bus.c`, `docs/embedding.md`, `14-FIX-OUTCOMES.md`
- FOUND commits: `eb8dfee`, `cf2fda6`, `f439bae`

---
*Phase: 14-fix-the-causes*
*Completed: 2026-11-13*
