---
phase: 14-fix-the-causes
plan: 04
subsystem: diagnose-outcomes
tags: [diagnose, vdp, board-choice, decision-record]

requires:
  - phase: 14-fix-the-causes
    provides: the baseline vdpmode, wclass and size sub-rows and the post-plan-03 re-run counts
provides:
  - the D-07 gate, D-10 family reading and D-04 trigger with counts in the outcomes note
  - rank 2 decided at its rank position (close per D-07, closure written in plan 08)
  - rank 3 recorded as open for D-04 (needs new evidence), routed to the gap round
  - the D-04 trigger threshold as its own Decision table row
affects: [14-05, 14-08]

actuals:
  tokens: 1800
  tasks: 3
  commits: 2
plan_head_before: 4588d367fe2e1a3d03a751618b227bcabcb2f570
plan_head_after: e4afc9161f5976e2ed6fbeef71d6ad24cf43d0bb

tech-stack:
  added: []
  patterns:
    - "a header rule in guess_board only reaches images the database does not know; a database-sourced row needs different evidence"

key-files:
  created: []
  modified:
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

key-decisions:
  - "D-07 gate: no M4=1 rows over every unsupported_video_mode leaf (graphics1 13, graphics2 29), so rank 2 is decided as close per D-07 and no decode fix is written"
  - "D-10 families: no zemina, korean_nb or korean row has an M4=1 class, so no mapper check or trace was needed"
  - "D-04 trigger fired on reg-0 (11 rows of ranks 3 and 7 on the baseline), but every rom_write row of ranks 3 and 7 has board source db, so no header rule can reach it; recorded as open: D-04 needs new evidence for the gap round"

requirements-completed: []

coverage:
  - id: G1
    description: "The D-07 gate reads no M4=1 rows over the unsupported_video_mode leaves and rank 2 carries its decision at its rank position"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "14-FIX-OUTCOMES.md lines D-07 gate and the rank 2 row; plan Task 1 verify command, exit status 0"
        status: pass
    human_judgment: false
  - id: G2
    description: "The D-10 family reading records no family M4=1 rows for zemina, korean_nb and korean"
    requirement: FIX-03
    verification:
      - kind: other
        ref: "14-FIX-OUTCOMES.md line D-10 families; plan Task 1 verify command, exit status 0"
        status: pass
    human_judgment: false
  - id: G3
    description: "The D-04 trigger line prints every reg-class count with the threshold as its own Decision table row, fires on reg-0, and ends open for new evidence"
    requirement: FIX-04
    verification:
      - kind: other
        ref: "14-FIX-OUTCOMES.md lines D-04 trigger and D-04, and the Decision table threshold row; acceptance grep, exit status 0"
        status: pass
    human_judgment: false
  - id: G4
    description: "No code changed, so the vdp, mapper and cart fixtures are unchanged and still pass"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest -R vdp, mapper, core, cart, runner.diagnose, docs.api_names, docs.budgets, policy.claims_scoped: exit status 0"
        status: pass
    human_judgment: false
  - id: G5
    description: "The trace of D-10 Step B"
    requirement: FIX-03
    verification:
      - kind: other
        ref: "not taken: no family M4=1 row"
        status: unknown
    human_judgment: false
  - id: G6
    description: "The pin compare"
    requirement: FIX-04
    verification:
      - kind: integration
        ref: "not run: no code changed, so no hash can move"
        status: unknown
    human_judgment: false
  - id: G7
    description: "The diagnose re-run"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "not run: no code changed, counts are the post-plan-03 re-run's"
        status: unknown
    human_judgment: false
  - id: G8
    description: "The full workflow passes on the plan head"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit status 0"
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 04: Rank 2 and the three family gates Summary

**The D-07, D-10 and D-04 gates are read from the measured sub-rows and none leads to a code change: no M4=1 row exists under any unsupported_video_mode leaf, no family row is M4=1, and the board-family trigger fired on reg-0 but every affected row is database-sourced, so it goes to the gap round.**

## Performance

- **Duration:** 15 min
- **Completed:** 2026-11-13T04:22:04Z
- **Tasks:** 3 (one tracer, two conditional tasks that made no edit)
- **Files modified:** 1 (the outcomes note)

## Accomplishments

- D-07 gate: the `vdpmode=<class>:<machine>:<mapper>` sub-rows of ranks 2 and 6 sum to graphics1 13 and graphics2 29, all on the SMS 1 profile family; mode4, invalid and sms2only are 0. Rank 2 reads `decided: close per D-07` with its counts; plan 08 writes the closure.
- D-10 families: zemina, korean_nb and korean have counts by class and no M4=1 row, so neither the mapper check nor the one-exemplar trace applied.
- D-04 trigger: the 5-row threshold is its own Decision table row (assumption, owner can change). Counts over ranks 3 and 7 on the baseline: reg-0 11, reg-cm 4, reg-zm 3, reg-korean 0, reg-4pak 0 (post-plan-03: 12, 4, 4, 0, 0). reg-0 fires.
- D-04 outcome: the cross-cut `source=db leaf=...` shows every rom_write row (36 of 36 on rank 3, 4 of 4 on rank 7) took its board from the database row, which decides before `guess_board` reaches the Codemasters header or any other header rule. A header rule cannot move these rows, a table row would be keyed on a digest, and the retained output joins no write class to a header state. Rank 3 reads `open: D-04 needs new evidence`; plan 05 still lands the drop D-04 decides, and plan 08 lists the board question under Gap round.

## Task Commits

1. **Task 1: gates and rank-2 decision (tracer)** - `8b683d1` (docs)
2. **Task 2: M4=1 rows** - no commit: no M4=1 decode or family row, no edit
3. **Task 3: D-04 header rule and re-run** - no rule written (no shared header property reachable); the pins line in `e4afc91` (docs)

**Plan metadata:** the commit that carries this file (docs: complete plan)

## Files Created/Modified

- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - Decision table threshold row, rank 2 and rank 3 rows, a Gates (plan 04) section, a Pins line

## Decisions Made

- The tracer feedback gate: the tracer's verify is automated-only and human_verify_mode is end-of-phase, so the verify was re-run (exit 0) and the plan expanded without a checkpoint.
- The board-family question for reg-0 is not a header-rule question while the database decides the board; it needs new evidence (for example a per-row join of write class, header state and database row) and goes to the one gap round.
- The post-plan-03 counts are quoted beside the baseline where they differ (reg-0 12 against 11, reg-zm 4 against 3); the trigger result does not depend on which is used.

## Deviations from Plan

None - plan executed exactly as written. Tasks 2 and 3 took their documented no-edit branches.

## Verification

- Task 1 verify command and the threshold grep: exit 0.
- `cmake --preset ci`, build, then `ctest -R` over the vdp, mapper, core, cart, runner.diagnose, docs.api_names, docs.budgets and policy.claims_scoped groups: exit 0.
- Task 2 and 3 result greps: exit 0. `git diff --quiet HEAD -- src tests`: exit 0. `git diff --exit-code origin/main` on the ledger, compatibility page and ci.yml: exit 0. `git diff --quiet origin/main -- src/cart.c`: exit 0. No new `src/vdp_*.c` file against origin/main.
- `docs/hardware/video.md` is 14,571 bytes, under 38,912.
- `cmake --workflow --preset ci`: exit 0.
- The dump-dependent measurement was not re-run (no code changed); the retained aggregate output of the plan 03 re-run was read for class counts only. `scripts/library.sh` with no argument or `run`, and the library workflow, were not run.

## Issues Encountered

None.

## Known Stubs

None.

## Next Phase Readiness

- Plan 05 works ranks 3 and 7 from 36 and 4 rows (post-plan-03), with the D-04 drop still to decide and the board question open.
- Plan 08 writes rank 2's closure from the D-07 gate line and lists the reg-0 board question under Gap round, with the below-threshold classes reg-cm and reg-zm as seed candidates for SEED-016.

## Self-Check: PASSED

- FOUND: `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md`
- FOUND commits: `8b683d1`, `e4afc91`
