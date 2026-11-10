---
phase: 05-tune-up
plan: "09"
subsystem: planning-records
tags: [review-dispositions, seeds, tune-up]
status: complete

requires:
  - phase: 05-tune-up
    provides: the size-rule fix d9f3a2e (plan 05-07) and the comment fixes 0227433 (plan 05-08)
provides:
  - the phase 1, 2 and 4 review dispositions settled in place, open 0 in all three
  - each deferred finding written as a bullet in the seed its disposition names
  - SEED-007 holding the two tooling findings, the accepted key risk, the sound ROM deferral and the next tune-up's ideas
affects: [05-14]

actuals:
  tokens: 4000
  tasks: 2
  commits: 2

plan_head_before: 581865d343866077c9bdb2d0d65a73cfc721b33c
plan_head_after: b50ce5b41a58b25994b18d4f5b1c6f80974e5b1b
commits: 2

tech-stack:
  added: []
  patterns:
    - "A deferred finding names its seed in the Source cell and appears as a bullet in that seed's Notes"

key-files:
  created: []
  modified:
    - .planning/milestones/v1.0-phases/01-a-test-frame-in-retroarch/01-REVIEW-DISPOSITION.md
    - .planning/milestones/v1.0-phases/02-the-cpu-matches-the-public-vectors/02-REVIEW-DISPOSITION.md
    - .planning/milestones/v1.0-phases/04-sound/04-REVIEW-DISPOSITION.md
    - .planning/seeds/SEED-001-whole-library.md
    - .planning/seeds/SEED-002-states-and-movies.md
    - .planning/seeds/SEED-004-tools-observability-ai.md
    - .planning/seeds/SEED-007-milestone-tune-up.md

key-decisions:
  - "P1 WR-02 and P1 IN-04 are deferred to SEED-007 under AGENTS rule 7 and no script changed"
  - "P4 WR-02 is recorded fixed from 04-REVIEW-FIX.md, matching its frontmatter"
  - "Third-party sound ROM conformance stays deferred with no lane, recorded in SEED-007"

requirements-completed:
  - TUNE-07

coverage:
  - id: DISPOSITIONS
    description: "No finding in the phase 1, 2 and 4 review dispositions reads open; every deferred row names a SEED file and every skipped row has a reason, and both tooling findings are deferred to SEED-007"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "the disposition awk check exits 1 before editing and 0 after; grep -q '^open: 0' exits 0 for all three files; the WR-02 and IN-04 awk count exits 0; cmake --workflow --preset hygiene exits 0 with 43 of 43 tests"
      status: passed
    human_judgment: false
  - id: SEED-NOTES
    description: "Each deferred finding, the sound ROM conformance deferral and the CONTEXT deferred ideas are bullets in SEED-001, SEED-002, SEED-004 and SEED-007"
    requirement: TUNE-07
    verification:
      kind: other
      ref: "the task 1 grep chain over the four seed files exits 0 (baseline grep -c WR-02 on SEED-007 was 0)"
      status: passed
    human_judgment: false
---

# Phase 5 Plan 09: Settle the v1.0 review tail Summary

The 29 findings in the phase 1, 2 and 4 review dispositions are all settled in place (fixed with a commit SHA, skipped with a reason, or deferred naming a seed), and every deferred one is a bullet in that seed's Notes, so none reads open.

## Accomplishments

- Task 1 (`9f5db5b`): bullets appended to four seeds. SEED-007 gained P1 WR-02 and P1 IN-04 (tooling findings, AGENTS rule 7, not fixed), P1 WR-04 (accepted risk), the third-party SN76489 and YM2413 sound ROM conformance deferral, the unmeasured per-test ctest timeouts, and an absolute `ir_per_frame` ceiling with a coverage lane. SEED-002 gained P1 IN-07, P2 IN-02, P2 IN-06 and P4 IN-04; SEED-004 gained P2 IN-01 (and a Notes section); SEED-001 gained P2 IN-05, which rides the Phase 6 plan because it touches `probe.c`.
- Task 2 (`b50ce5b`): the three disposition files, each with `open: 0`.

| Phase | Fixed | Skipped | Deferred |
|---|---|---|---|
| 1 (12) | IN-01, IN-02, IN-03 (`0227433`) | WR-01, WR-03, IN-05, IN-06, IN-08 | WR-02, WR-04, IN-04 (SEED-007); IN-07 (SEED-002) |
| 2 (11) | the five warnings as before, plus IN-03 (`d9f3a2e`) | IN-04 | IN-01 (SEED-004); IN-02, IN-06 (SEED-002); IN-05 (SEED-001) |
| 4 (6) | WR-01, WR-02 (04-REVIEW-FIX.md), IN-01 (`0227433`) | IN-02, IN-03 | IN-04 (SEED-002) |

- P1 WR-03 cites release workflow run 42741490115 on main (https://github.com/szTheory/cogmaster/actions/runs/42741490115), conclusion success.

## Evidence

All commands ran from the repository root.

| Command | Exit |
|---|---|
| `grep -c 'WR-02' .planning/seeds/SEED-007-milestone-tune-up.md` before editing | baseline 0 (grep exits 1) |
| task 1 grep chain over the four seed files, after editing | 0 |
| the disposition awk check over the three files, before editing | 1 (open rows in the phase 1 and phase 4 files) |
| the disposition awk check, after editing | 0 |
| `grep -q '^open: 0'` on each of the three files | 0, 0, 0 |
| `awk` count of WR-02 and IN-04 rows deferred to SEED-007 in the phase 1 file | 0 (count is 2) |
| `gh run list --workflow release.yml --status success --limit 1` | 0, run 42741490115 |
| `cmake --workflow --preset hygiene` | 0, 43 of 43 tests |

## Decisions

- P1 WR-02 and P1 IN-04 were moved from fix-now to deferred (SEED-007) under AGENTS rule 7, per the orchestrator's revision of the dispositions decision; no script changed in this phase.
- P4 WR-02 is recorded as fixed on the evidence of 04-REVIEW-DISPOSITION.md's frontmatter and 04-REVIEW-FIX.md. This is a planner decision (the CONTEXT dispositions list does not name it), so the phase verification does not read it as an unplanned change.
- Third-party sound ROM conformance stays deferred with no lane, recorded in SEED-007.
- P2 IN-03 cites `d9f3a2e`, the fix commit of plan 05-07 (its summary names that commit); P1 IN-01 to IN-03 and P4 IN-01 cite `0227433`, plan 05-08's single commit.

## Deviations from Plan

None - the plan executed as written. Two small additions: the P4 IN-02 and IN-03 reasons say each guard costs one compare, and the P2 table footer notes that the remaining findings were settled in the v2.0 tune-up.

## Observations

- `hygiene.tree` can exceed its 60 second ctest limit under heavy machine load (observed in plans 05-03 and 05-07; it passes alone in 37 to 57 seconds). It passed in this plan's hygiene run. It is recorded as a per-test ctest timeout seed in SEED-007; nothing was changed here.

## Known Stubs

None.

## Threat Flags

None. Planning records only; no code, workflow or script changed.

## Self-Check: PASSED

- FOUND: the three disposition files and four seed files with the edits.
- FOUND: commits `9f5db5b` and `b50ce5b` (ancestors of HEAD).
