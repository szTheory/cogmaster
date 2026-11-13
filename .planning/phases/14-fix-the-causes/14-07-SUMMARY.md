---
phase: 14-fix-the-causes
plan: 07
subsystem: vdp-faults
tags:
  - vdp
  - fault
  - diagnose
  - decision

requires:
  - phase: 14-fix-the-causes
    provides: the post-plan-06 diagnose counts (run_fail 49, rank 6 at 6) and plan 04's D-07 gate (no M4=1 row in any unsupported-mode leaf)
provides:
  - the D-08 decision for rank 6, made from counts at the plan 06 head
  - the rank 6 outcome row, decided close per D-07, with the order and vdpmode counts
affects: [14-08]

actuals:
  tokens: 1500
  tasks: 2
  commits: 2
plan_head_before: e6311f22bbc39b6816d9a5cdf3728fd617236b6d
plan_head_after: b1655ad635756893f824ea859acb52ec0cc1d1b1

tech-stack:
  added: []
  patterns:
    - "a decision gate read from the retained diagnose output by class words only, with no title or path copied"

key-files:
  created: []
  modified:
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md

key-decisions:
  - "D-08 shape absent: all 6 rank-6 rows are order=before (0 after; the shape needs 5 or more), vdpmode graphics1 6 of 6 (M4=0). No restart transient to fix, so the raise stays in render_until and no file in src or tests changed; rank 6 closes in plan 08 with the D-07 reason"

requirements-completed: []

coverage:
  - id: C1
    description: "The D-08 reading: rank 6 rows with order=after 0 of 6 and vdpmode M4=0 for 6 of 6, so the shape is absent and no core edit is made"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose from the repository root at e6311f2, exit status 0; run_fail 49, run_fail_not_in_ledger 0; order and vdpmode counts in 14-FIX-OUTCOMES.md"
        status: pass
    human_judgment: false
  - id: C2
    description: "The rank 6 outcome row is filled (decided close per D-07, before 6 and after 6) and the note holds no file name, dotted machine token, title field or home path"
    requirement: FIX-04
    verification:
      - kind: other
        ref: "the task 2 verify command (grep of the rank 6 row, grep for leak classes, docs/controllers.md size under 38912), exit status 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "vdp.background.unsupported and ports.gun.fault_neutral still pass unchanged: a persistent unsupported mode still faults and gun and pad fault records are byte-equal"
    requirement: FIX-01
    verification:
      - kind: unit
        ref: "ctest vdp.*, ports.gun.*, runner.trace.fault, runner.diagnose and core.* (112 tests), exit status 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "The full workflow passes on the plan head"
    requirement: FIX-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit status 0 (676 tests and 5 release tests pass)"
        status: pass
    human_judgment: false
  - id: C5
    description: "The pin compare was not run: D-08 did not fire, no file in src or tests changed, so no hash can have moved"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "git diff --quiet HEAD -- src tests at the end of task 1, exit status 0; the compare command is not run"
        status: pass
    human_judgment: false

duration: 35min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 07: Rank 6 decision Summary

**The D-08 gate read 0 of 6 rank-6 faults after the first non-blank picture (all 6 before it, all graphics1, M4=0), so the shape is absent, no core file changed, and rank 6 is recorded as decided: close per D-07 for plan 08 to write.**

## Performance

- **Duration:** 35 min (13 of them the diagnose run over the dump set)
- **Completed:** 2026-11-13
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- Read plan 04's D-07 gate first: no M4=1 row in any unsupported_video_mode leaf, and rank 6 is not open on D-07, so D-08 was evaluated rather than skipped.
- Ran `sh scripts/library.sh diagnose` once from the repository root at head e6311f2 on a clean tree (exit status 0, run_fail 49, run_fail_not_in_ledger 0, ledger_fail_now_pass 156). Counted rank 6 by class words only: `order=before` 6, `order=after` 0, `order=nopicture` 0; vdpmode graphics1 6 of 6.
- D-08 needs 5 or more `order=after` rows (a supported mode-4 picture shown first, then a restart transient). With 0, there is no transient to move the raise past, so nothing in `src/` or `tests/` changed and the latch move was not made.
- Recorded the decision ("D-08: shape absent, closed in plan 08") with counts in a new "Video mode decision (plan 07)" section of 14-FIX-OUTCOMES.md, and filled the rank 6 row: outcome `decided: close per D-07`, before 6, after 6. Rank 2 is untouched (still `decided: close per D-07`, closure in plan 08).
- The re-run is the same as the plan 06 re-run, as expected for a plan with no code change: ranks 2, 6, 9 and 10 at 40, 6, 2 and 1.

## Task Commits

1. **Task 1 (tracer): read the D-08 shape** - `98e9bc7` (docs) - 14-FIX-OUTCOMES.md. The shape was absent, so the tracer's expansion (the latch raise, tests, rename of the mode helper) was not started; the tracer's verify command (build and 112 tests over vdp, ports.gun, runner.trace.fault, runner.diagnose and core) exits 0.
2. **Task 2: the rank 6 outcome row** - `b1655ad` (docs) - 14-FIX-OUTCOMES.md

**Plan metadata:** the commit that carries this file (docs: complete plan)

## Files Created/Modified

- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - the D-08 decision section with counts and the rank 6 outcome row, counts and class words only

## Decisions Made

See key-decisions. The plan's `order=after` reading (5 or more rows, every vdpmode class M4=0) was applied as written; the rows were not close to the threshold (0 of 6), so the 5-row figure, which the plan flags as an assumption, did not decide anything.

## Deviations from Plan

None - plan executed exactly as written. The orchestrator's note applied: the counts the plan quotes for rank 6 are unchanged by plans 03 to 06 (6 rows throughout), and the D-08 reading was made at the post-plan-06 head.

## Issues Encountered

None. The plan 04 `D-07 gate:` line and the baseline `vdpmode=` sub-row (graphics1:sms:sega 6) agree with the fresh reading.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- Commits `98e9bc7` and `b1655ad` are ancestors of HEAD; `git rev-list --count` over the plan range measured 2.
- Task 1 acceptance: the verify command exits 0; the D-08 grep exits 0; `git diff --quiet HEAD -- src tests` exits 0.
- Task 2 verify command exits 0; `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md .github/workflows/ci.yml` exits 0; no new `src/vdp_*.c` file against origin/main.
- `cmake --workflow --preset ci` exits 0 (676 tests and 5 release tests pass).
- The note holds no file name, dotted machine token, title field or home path.
