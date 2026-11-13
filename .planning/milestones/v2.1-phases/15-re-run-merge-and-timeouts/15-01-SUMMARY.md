---
phase: 15-re-run-merge-and-timeouts
plan: 01
subsystem: compat-ledger
tags:
  - library-judge
  - ledger
  - ratchet
  - compatibility-page
  - evidence

requires:
  - phase: 14-fix-the-causes
    provides: the core fixes (behaviour revision 18) whose flipped rows this run makes public
provides:
  - the merged post-fix library rows in tests/compat/ledger.csv, every judged row stamped with one core commit
  - the regenerated docs/compatibility.md (pass 744 to 1029, fail 381 to 96)
  - 15-RUN-EVIDENCE.md with the library run, merge checks, transition counts, local gates and the hand-off to the reachability check
affects: [15-02, ship of phase 15]

actuals:
  tokens: 92000
  tasks: 3
  commits: 2
plan_head_before: 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4
plan_head_after: 60d8a7fe38ffbbff4efe760e14f088249e2cbe20

tech-stack:
  added: []
  patterns:
    - "accept an owner run by counts: the merge line, a two-file status check and a full-length core stamp read back from the working-tree ledger"
    - "a one-off awk transition count over the public ledger against origin/main, its exit status encoding that no pass row was lost"

key-files:
  created:
    - .planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md
  modified:
    - tests/compat/ledger.csv
    - docs/compatibility.md
    - .planning/STATE.md

key-decisions:
  - "The run was accepted on the merge line (ratchet ok, 0 rows added, 13813 page bytes), the two-file status check and the single full-length core stamp 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4, which is the commit the run judged and an ancestor of the ledger commit"
  - "The library: counts the owner did not paste are recorded as not reported; they are not derived from any other file of the results directory, and nothing in the plan depends on them"
  - "The 50 ntsc fail rows left in the ledger (48 on sms with fm off, 2 with fm on) are within one of the 49 default-profile fails of the Phase 14 diagnose, read as a sanity figure; no counts-only diagnose run is asked of the owner"

requirements-completed:
  - RUN-01
  - RUN-02

coverage:
  - id: D1
    description: "The library judge was run once at the phase head by the owner with the no-argument command and merged through the existing ratchet: ratchet ok, 0 rows added, 1116 rows rewritten, 285 fail-to-pass transitions"
    requirement: RUN-01
    verification:
      - kind: other
        ref: "sh scripts/library.sh run by the owner (about 644 s) and the merge line read from build/library/results/summary.txt, grep exit status 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The run is accepted by counts: the merge line, only the ledger and the page changed outside .planning, and exactly one full-length core stamp that is an ancestor of the ledger commit with nothing outside .planning changed since it"
    requirement: RUN-01
    verification:
      - kind: other
        ref: "grep -Eq on the merge line, the two-file git diff --quiet check and the stamp check with git merge-base --is-ancestor, each exit status 0; positive control on merge 032ebab exit status 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "No pass row of the origin/main ledger became fail, another status, or disappeared (pass_lost 0)"
    requirement: RUN-01
    verification:
      - kind: other
        ref: "the one-off awk transition count over the origin/main ledger and the merged ledger, exit status 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The compatibility page equals the regeneration of the merged ledger and no other tracked file quotes its totals"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "sh scripts/compat-common.sh docs tests/compat/ledger.csv piped to cmp against docs/compatibility.md exit status 0; ctest compat.exit, compat.whole_list.fresh and compat.page_bound 3 of 3 passed; grep of README.md and docs for 744, 381, 1029 and 96 beside pass or fail found no other file"
        status: pass
    human_judgment: false
  - id: D5
    description: "The old base judge (the origin/main copies of the ratchet and the common script) accepts the merged ledger at behaviour revision 18 and prints the protected-comparison pass line"
    requirement: RUN-01
    verification:
      - kind: integration
        ref: "sh of the origin/main compat-ratchet.sh over the base and merged ledgers with build/ci/tests/compat-fixtures/results.csv, exit status 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ci workflow passes on the commit that carries the merged ledger and page"
    requirement: RUN-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit status 0 (676 tests in the test step, 5 in the release test step); the hosted CI run URL is recorded at ship"
        status: pass
    human_judgment: false

duration: 7min
completed: 2026-11-13
status: complete
---

# Phase 15 Plan 01: Re-run, merge and publish the library results Summary

**The owner's library run at the phase head merged through the unchanged ratchet: the page moves from pass 744, fail 381 to pass 1029, fail 96 (285 fail-to-pass, no pass row lost), equals its regeneration, and both the ci workflow and the old base judge exit 0.**

## Performance

- **Duration:** 7 min for this continuation (Task 2 and Task 3); the owner's library run took about 644 s and the Task 1 hand-off came before it, neither counted
- **Started:** 2026-11-13T18:51:25Z (continuation)
- **Completed:** 2026-11-13T19:00:00Z
- **Tasks:** 3 (Task 1 was the owner's run, no commit)
- **Files modified:** 4 outside the SUMMARY (ledger, page, evidence note, STATE.md)

## Accomplishments

- The ledger and the page were rewritten by `scripts/library.sh` only, never by hand: 1116 rows carry new evidence and date, 0 rows were added, the ledger still has 1986 rows, and every judged row carries the single stamp `core=0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4`.
- The public transitions are 285 fail-to-pass rows (18 on gg ntsc, 126 on sms ntsc with fm off, 16 on sms ntsc with fm on, 125 on sms pal) and no other change of status; the base pass rows kept their status (pass lost 0).
- The ntsc fail rows left are 50 (48 with fm off, 2 with fm on) against the 49 recorded by the Phase 14 diagnose; 46 pal rows fail.
- `compat.exit`, `compat.whole_list.fresh` and `compat.page_bound` pass; the exit verdict is pass with the judged-cart floors met (554 cart passes against 20, 511 PAL against 1, 51 FM against 1) and the experimental flag absent.
- `cmake --workflow --preset ci` passed (676 plus 5 tests) and the old base judge passed at revision 18; the only non-planning change on the branch is the ledger and the page.

## Task Commits

1. **Task 1: The owner runs the library judge once at the phase head** - no commit (a checkpoint; the owner ran `sh scripts/library.sh` and reported done)
2. **Task 2: Accept the run by counts, prove the page agrees, commit ledger, page and note** - `8964eef` (fix)
3. **Task 3: Local gates on the merged head** - `60d8a7f` (docs)

**Plan metadata:** the docs commit that carries this file, STATE.md, ROADMAP.md and REQUIREMENTS.md

## Files Created/Modified

- `tests/compat/ledger.csv` - the merged post-fix library rows (written by the library script through the ratchet)
- `docs/compatibility.md` - the regenerated page with the new totals
- `.planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md` - counts-only evidence: run table, merge checks, transitions, local gates, hand-off
- `.planning/STATE.md` - the Runtime Log row for the step (Claude Code, claude-sonnet-5-5)

## Decisions Made

The three decisions in the frontmatter. The ntsc fail count is a sanity reading against the 49 of Phase 14, never a gate (the ratchet is the gate).

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The owner pasted no `library:` lines and no `merge:` line, so the `library:` counts (candidates, attempted, evaluated, predicates, status counts) are recorded as `not reported`, as the plan's resume signal provides. The `pal:` and `fm:` lines of the summary are small (one derived PAL profile run, no derived FM profile) because, per `scripts/library.cmake`, a derived profile is counted only when the ledger-driven loop did not already run it; the PAL and FM rows that flipped (125 and 16) came through that loop.
- The inline loop of the one-time count grep could not be parsed by the interactive shell (`bad math expression`); the same grep ran from a scratch sh script outside the tree and found no other file quoting 744, 381, 1029 or 96.
- One run of the composite judged-commit ancestor check, made right after the compat tests, ended with exit status 139 (a crashed process); the same check passed stepwise and on three repeated runs (exit status 0 each time), and the committed state did not change in between. Nothing in the tree was affected; it is recorded here because the cause was not found.

## Known Stubs

None.

## Threat Flags

None - no new network endpoint, auth path, file access pattern or schema at a trust boundary; the only files read from the run were `build/library/results/summary.txt` and the public ledger and page.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02 (the job timeouts, TUNE-11) can run: the ledger commit is in, so the workflow commit can be the last non-planning commit (D-12).
- At `/gsd-ship` the branch is pushed; the verifier then runs `gh api repos/szTheory/cogmaster/commits/0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4` and records its status (the stamp must be a public commit), and the hosted CI run URL for the pushed head is recorded.

## Self-Check: PASSED

The ledger, the page, the evidence note and this file exist; commits 8964eef and 60d8a7f are ancestors of HEAD; the judged commit is an ancestor of the ledger commit; every acceptance criterion of Task 2 and Task 3 was re-run after its commit and exited 0.

---
*Phase: 15-re-run-merge-and-timeouts*
*Completed: 2026-11-13*
