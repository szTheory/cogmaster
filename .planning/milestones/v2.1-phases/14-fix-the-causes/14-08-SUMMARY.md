---
phase: 14-fix-the-causes
plan: 08
subsystem: diagnostics
tags:
  - diagnose
  - outcomes
  - boots-check
  - pins

requires:
  - phase: 14-fix-the-causes
    provides: the core fixes of plans 03 to 06, the rank 2 and rank 6 decisions of plans 04 and 07, the baseline hashes file and the outcomes note
provides:
  - the final diagnose at the phase head (run_fail 205 to 49, not in ledger 0) and every rank outcome in 14-FIX-OUTCOMES.md
  - the Families table (zemina, korean_nb, korean), the Gap round list and the boots-check record
  - the base-versus-head pin compare (exit 0) and the SEED-006 and SEED-016 breadcrumbs
affects: [15-confirm-and-publish]

actuals:
  tokens: 14000
  tasks: 3
  commits: 4
plan_head_before: 230cbc9a0667de89a7c8ea0fadf7d358b84f1c33
plan_head_after: 0878577ed90161e04787baf5584c8e1dafef664b

tech-stack:
  added: []
  patterns:
    - "a counts-only closure with four parts per closed row (class, source, why fail is honest, what reopens it)"

key-files:
  created: []
  modified:
    - .planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md
    - .planning/seeds/SEED-006-sg1000-mode.md
    - .planning/seeds/SEED-016-failing-carts.md
    - .planning/STATE.md
    - runner/diag.c
    - runner/diag_class.h
    - src/mapper.c
    - tests/runner/test_runner.c

key-decisions:
  - "Rank 2 (40 rows) and rank 6 (6 rows) are closed as not emulated by design (SMS-VDP section 7, DEC.03), reopened by SEED-006; the library shows 46 rows needing legacy modes, all M4=0 on machine sms"
  - "The boots check is recorded sound: 3 no-fault rows at the head (never_draws 2, frozen 1), no leaf of 5, no late-picture row among 49 failing rows, runner/expect.c and the ledger key untouched"
  - "Rank 3 stays open on D-04 (the reg-0 board choice needs new evidence) and is the single Gap round entry, so FIX-01 and FIX-03 are left out of requirements-completed"
  - "The one pad-press run for static_no_fault was not taken: finding the cart needs the dump listing, which the agent may not read; rank 10 is closed on its own distinct1800 and the run goes to Phase 15's confirmation batch"

requirements-completed:
  - FIX-02
  - FIX-04

coverage:
  - id: D1
    description: "Final diagnose at the phase head: run_fail 49, run_fail_not_in_ledger 0, and an outcome row for every rank, ranks 2, 6, 9, 10 and 11 filled and closed with their four parts"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose from the repository root at head 230cbc9, exit status 0; run_fail_not_in_ledger compared as a number, 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "Families table for zemina, korean_nb and korean with cause, outcome and the four closure parts; the two korean alive rows counted once as passing after the rank 3 drop"
    requirement: FIX-03
    verification:
      - kind: other
        ref: "the task 1 verify command (three anchored family rows of eight cells each), exit status 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "Gap round list: rank 3 open on D-04 with its counts, no escalated leaf, no M4=1 vdpmode class, not-taken items named"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "the open-cell versus Gap-round-entry awk check from the task 1 acceptance criteria, exit status 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "Boots check recorded sound over the post-fix no-fault population: never_draws 2 and frozen 1, no late-picture row over 49 failing rows, the cap and the distinct rule untouched"
    requirement: FIX-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R runner.expect.predicates and runner.diagnose, exit status 0"
        status: pass
      - kind: other
        ref: "git diff --quiet origin/main -- runner/expect.c scripts/compat-common.sh scripts/library.cmake, exit status 0; the classification counts in the note; the pad-press run not taken"
        status: pass
    human_judgment: false
  - id: D5
    description: "Base-versus-head pin compare over a freshly written head hashes file: 2 identical files, no pin moved, behaviour revision 18"
    requirement: FIX-04
    verification:
      - kind: integration
        ref: "cmake -DFILES=build/p14-base-hashes.txt;build/ci/hashes.txt -DCOUNT=2 -DMODE=compare -P tests/cmake/hashes.cmake, exit status 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "Both workflows are green on the phase head and the ledger, compatibility page, ci.yml and every timeout are unchanged from origin/main"
    requirement: FIX-04
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (676 tests and 5 release tests) and cmake --workflow --preset hygiene (84 tests), exit status 0 each; CI run URL once pushed at ship"
        status: pass
    human_judgment: false
  - id: D7
    description: "SEED-006 gains the legacy-mode row count (46) and SEED-016 gains the closed groups, candidates, gap round entry and not-taken items"
    requirement: FIX-01
    verification:
      - kind: other
        ref: "grep for needing legacy modes in SEED-006 and for 14-FIX-OUTCOMES.md in SEED-016, exit status 0"
        status: pass
    human_judgment: false

duration: 75min
completed: 2026-11-13
status: complete
---

# Phase 14 Plan 08: Close the phase record Summary

**The final diagnose at the phase head reads run_fail 205 to 49 with none outside the ledger; the outcomes note now closes ranks 2, 6, 9, 10 and 11, the three mapper families and the boots check, leaves rank 3's reg-0 board question as the single gap round entry, and proves no hash pin moved.**

## Performance

- **Duration:** 75 min (13 of them the diagnose run over the dump set)
- **Completed:** 2026-11-13
- **Tasks:** 3
- **Files modified:** 8

## Accomplishments

- Ran `sh scripts/library.sh diagnose` once from the repository root at head 230cbc9 (clean tree, exit status 0): judged 573, run_fail 49, run_fail_not_in_ledger 0, ledger_fail_now_pass 156. Leaves at the head: rank 2 40, rank 6 6, rank 9 2, rank 10 1, rank 11 0. No leaf of ranks 7 to 11 reaches 5 rows and no new leaf printed.
- Closed rank 2 and rank 6 as not emulated by design (legacy TMS modes, SMS-VDP section 7, DEC.03), with the final `vdpmode` counts (graphics1 13, graphics2 33, all M4=0, all machine sms). Closed rank 9 (2 rows) and rank 11 (0 rows) and rank 10 (1 row) with their four parts.
- Families table: zemina 8 rows, korean_nb 5 and korean 2 are all the legacy-mode cause; the two korean alive rows (rank 3's reg-cm writes) now pass and are counted once in the passing group.
- Boots check recorded sound: 3 no-fault rows (never_draws 2, frozen 1), 0 late-picture rows over all 49 failing rows, the passing histogram ends at 361-420 with nothing from 421 to 600, `boots@600` and `distinct >= 2` untouched.
- Pin proof: `hashes_file` rewritten on the head, then the compare against the plan 01 base file exits 0 ("2 identical files"); behaviour revision stays 18.
- SEED-006 gains "the library shows 46 rows needing legacy modes"; SEED-016 gains a Breadcrumbs section.

## Task Commits

1. **Task 1 (tracer): final diagnose, closures, families, gap round** - `d3b67c8` (docs)
2. **Task 2: boots check and pin proof** - `5b83e42` (docs)
3. **Task 3: seeds, gates, note header** - `a4ad03a` (style, the gate fix) and `0878577` (docs)

**Plan metadata:** the commit that carries this file (docs: complete plan)

## Files Created/Modified

- `.planning/phases/14-fix-the-causes/14-FIX-OUTCOMES.md` - final run, outcomes, Families, Gap round, Boots check, Pins, gates; counts only
- `.planning/seeds/SEED-006-sg1000-mode.md`, `.planning/seeds/SEED-016-failing-carts.md` - breadcrumbs
- `.planning/STATE.md` - the session row for this step
- `runner/diag.c`, `runner/diag_class.h`, `tests/runner/test_runner.c` - clang-format only; `src/mapper.c` - one comment reworded

## Decisions Made

See key-decisions. D-13's `tests/core/test_state.c` and `tests/diag/expected.csv` needed no edit (RESEARCH premise check 9: they assert no dropped kind), and `tests/mapper/test_mapper.c` needed none (it asserts only memory-side cartridge and RAM `unmapped_write` faults, which stay). The docs/embedding.md revision-18 sentence, its Faults rows (36791 bytes, under 38912) and the tests/core/test_core.c comment already matched what landed, so neither changed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] hygiene workflow failed on code from earlier plans**
- **Found during:** Task 3 (full gates)
- **Issue:** `hygiene.format` flagged runner/diag.c, runner/diag_class.h and tests/runner/test_runner.c (clang-format 21), and `hygiene.no_float` read "sections 2.1 and 2.2" in a src/mapper.c comment as a decimal number. Neither was caught by the earlier plans' ci-only runs.
- **Fix:** ran clang-format 21 on the three files and wrote the comment as "section 2.1 and section 2.2"; shortened one comment in runner/diag.c that the formatter would have wrapped. No behaviour change.
- **Files modified:** runner/diag.c, runner/diag_class.h, src/mapper.c, tests/runner/test_runner.c
- **Verification:** `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit status 0 after the fix
- **Committed in:** a4ad03a

**2. [Instruction - not taken] the D-14 pad-press run**
- **Found during:** Task 2
- **Issue:** the run needs the static_no_fault row's cart found among the dumps, which prints file names the agent may not read (orchestrator note 2).
- **Fix:** recorded as not taken with that reason in rank 10 and the Boots check section; rank 10 is closed on its own `distinct1800` of 1 (D-09). With 1 row there is no rerun campaign to defer. The D-06 Sega bank-shift trace is likewise recorded not taken, for Phase 15's confirmation run.

---

**Total deviations:** 1 auto-fixed (1 blocking), 1 step not taken for a stated reason
**Impact on plan:** no behaviour or pin moved; the formatting fix was needed for the hygiene gate to pass.

## Issues Encountered

- Rank 3's `fixed in the core; open: D-04 needs new evidence` is the one open cell, so FIX-01 and FIX-03 are left out of `requirements-completed`; the Gap round entry for it names D-04 with its counts (32 rows in the 13 note, 0 at head).
- Rank 6 is closed per D-07 as plan 07 decided. Its 6 rows are alive carts (5 at one program counter, all before the first picture); SEED-016 lists the leaf as a first candidate to trace if a later phase wants more than the legacy-mode closure.

## Known Stubs

None.

## Threat Flags

None.

## Next Phase Readiness

- Phase 14 plans are all executed; the next step is the phase code review and verification, then ship. Phase 15's confirmation run should take the D-06 trace and the pad press if the owner passes a dump path.

## Self-Check: PASSED

- Commits `d3b67c8`, `5b83e42`, `a4ad03a` and `0878577` are ancestors of HEAD; `git rev-list --count` over the plan range measured 4 before this file.
- Task 1 verify command, the open-cell awk check and the task 2 verify command exit 0; the note holds no file name, dotted machine token, title field, home path or input-script text.
- `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0; the ledger, page, ci.yml and TIMEOUT fences hold; budgets exit 0.
