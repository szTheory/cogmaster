---
phase: 14-fix-the-causes
plan: 05
subsystem: mapper-faults
tags: [mapper, fault, rom-write, diagnose]

requires:
  - phase: 14-fix-the-causes
    provides: the post-plan-03 diagnose counts and plan 04's D-04 record (board choice needs new evidence)
provides:
  - writes to a cartridge address that is no register and no enabled cartridge RAM record no fault on every board
  - fault.rom_write, the mapper.ext sweeps, eeprom.board.banks and the runner.diagnose ld-8000 line flipped, each with a register or marker positive control
  - docs/embedding.md row 3 and the never-faults list updated
  - outcome rows for ranks 3 and 7 with before and after counts
affects: [14-06, 14-08]

actuals:
  tokens: 5200
  tasks: 3
  commits: 3
plan_head_before: d6ae6aa099efb8b95cde89302a1cdc65cd53bfb1
plan_head_after: 03bf709411ebfdbe23772b8ad38c2623fc25700c

tech-stack:
  added: []
  patterns:
    - "a ROM has no write line: a write that is no register and no enabled RAM falls through the mapper switch and changes nothing"

key-files:
  created: []
  modified:
    - src/mapper.c
    - src/mapper_ext.c
    - tests/bus/test_bus.c
    - tests/runner/diagnose.cmake
    - tests/mapper/test_mapper_ext.c
    - tests/mapper/test_eeprom.c
    - docs/embedding.md
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

key-decisions:
  - "The ROM_WRITE raise is removed on all ten boards and the COGMASTER_FAULT_KIND_ROM_WRITE constant stays, since the kind list only grows"
  - "Rank 3 keeps open: D-04 needs new evidence in its outcome cell beside fixed in the core, so plan 08's gap round still sees the board question"

requirements-completed: []

coverage:
  - id: C1
    description: "A write to a non-register cartridge address records no fault on the Sega and ROM-only boards and the register write before it still sets bank 2 to the value modulo the page count"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "tests/bus/test_bus.c#test_fault_rom_write (ctest fault.rom_write), exit status 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "Codemasters, Korean, Hi-Com, Zemina, Nemesis, 4 PAK and Sega EEPROM ignore writes to non-register ROM addresses, registers still bank, and the bank-past-the-end marker still fires"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "tests/mapper/test_mapper_ext.c and tests/mapper/test_eeprom.c#board_banks (ctest mapper.* and eeprom.*), exit status 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The runner prints fault=none for the ld-8000 seed, so the dropped kind is absent end to end"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "tests/runner/diagnose.cmake (ctest runner.diagnose), exit status 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "The diagnose re-run on the clean plan head: ranks 3 and 7 no longer print, run_fail 104 to 69, run_fail_not_in_ledger 0"
    requirement: FIX-04
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose from the repository root at bd5bce4, exit status 0; counts in 14-FIX-OUTCOMES.md"
        status: pass
    human_judgment: false
  - id: C5
    description: "The full workflow passes on the plan head"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit status 0"
        status: pass
    human_judgment: false

duration: 40min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 05: Writes to ROM stop faulting Summary

**Eight ROM_WRITE raise sites are removed so a write to a cartridge address that is no register and no enabled RAM changes nothing on every board; the diagnose re-run drops ranks 3 (32 to 0) and 7 (4 to 0) and the total run_fail from 104 to 69.**

## Performance

- **Duration:** 40 min (most of it the diagnose run over the dump set)
- **Completed:** 2026-11-13T04:58:00Z
- **Tasks:** 3
- **Files modified:** 8

## Accomplishments

- Sega, Codemasters, Korean, Korean no-bank, Hi-Com, Zemina, Nemesis, 4 PAK, Sega EEPROM and ROM-only boards no longer latch a write to ROM as the first fault; a hostile first fault no longer hides a later real one.
- Negative controls: fault.rom_write (Sega and no-mapper image), the six non-register writes at the end of every mapper.ext sweep, the Korean no-bank `$A000` write, eeprom.board.banks (`$1000` and the closed window), and the runner.diagnose ld-8000 line (`fault=none dev=none where=none fault_frame=none`).
- Positive controls: the `$FFFF` register write sets bank 2 to `$12` modulo the page count, the sweeps still call check_banks after every register write, cartridge RAM at `$8000` still takes writes, and MAPPER_RANGE for a bank past a non-power-of-two image (mapper.ext and bank 5 of a 3-page image) stays `present == 1`.
- Re-run on a clean tree: run_fail 104 to 69, run_fail_not_in_ledger 0, ledger_fail_now_pass 101 to 136. Of the 40 rows that left ranks 3 and 7, 35 pass and 5 land on the next fault of the same cart (rank 2 36 to 40, rank 4 11 to 12); ranks 5, 6, 9 and 10 are unchanged, so no dark_no_fault or static_no_fault leaf grew.
- Rank 3 reads `fixed in the core; open: D-04 needs new evidence` so plan 08's gap round still sees the board question plan 04 recorded.

## Task Commits

1. **Task 1 (tracer): Sega and ROM-only writes stop faulting** - `e7fdb11` (fix) - src/mapper.c, src/mapper_ext.c, tests/bus/test_bus.c, tests/runner/diagnose.cmake
2. **Task 2: the remaining boards ignore writes to ROM** - `bd5bce4` (fix) - src/mapper.c, src/mapper_ext.c, tests/mapper/test_mapper_ext.c, tests/mapper/test_eeprom.c, docs/embedding.md
3. **Task 3: re-run and record ranks 3 and 7** - `03bf709` (docs) - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

**Plan metadata:** the commit that carries this file (docs: complete plan)

The tests of task 2 were flipped first and the six board tests (mapper.ext.korean, hicom, zemina, nemesis, hes_4pak and eeprom.board.banks) failed against the old core before the raises were removed (RED), then passed (GREEN); both steps are in the one commit the plan names.

## Files Created/Modified

- `src/mapper.c`, `src/mapper_ext.c` - the raise sites are gone; a comment on each branch says a write to ROM changes nothing
- `tests/bus/test_bus.c`, `tests/mapper/test_mapper_ext.c`, `tests/mapper/test_eeprom.c`, `tests/runner/diagnose.cmake` - assertions flipped with the controls above
- `docs/embedding.md` - row 3 says the kind is no longer recorded and the constant stays; the never-faults list and the revision-18 sentence gain the clause (36,863 bytes, under the 38,912 budget)
- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - rank 3 and rank 7 rows and a Plan 05 re-run paragraph, counts only

## Decisions Made

See key-decisions. No board choice changed and no scan threshold moved; src/cart.c is not touched by any commit of this plan.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. The behaviour revision stays 18, as the plan words it (the revision-18 sentence gained the clause).

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- Commits `e7fdb11`, `bd5bce4` and `03bf709` are ancestors of HEAD; `git rev-list --count` over the plan range measured 3.
- Task acceptance greps and the plan-level `cmake --workflow --preset ci` exited 0 (676 tests and 5 release tests pass).
- `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md .github/workflows/ci.yml` exits 0 and no commit names src/cart.c.
