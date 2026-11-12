---
phase: 12-whole-list-in-the-ledger
plan: "09"
subsystem: docs
tags:
  - docs
  - seeds
  - ci-speed
  - exit-command
status: complete

requires:
  - 12-08
provides:
  - the exit command, its floors and the tie to the flag documented in the whole-list ledger section of docs/conformance.md
  - a negative-control row for each of the 14 new tests in docs/testing.md
  - the pin-bump regeneration sentence and the standing CI-speed instruction in docs/maintaining.md
  - SEED-015, the findable list of every deferred ledger and CI item with the phase's hosted measurements
affects:
  - 12-10

actuals:
  tokens: 3600
  tasks: 2
  commits: 2

plan_head_before: 2790a3e
plan_head_after: ac0764d
commits: 2

tech-stack:
  added: []
  patterns:
    - "A maintainer-facing note that must outlive a chat (the owner's standing CI instruction, the phase's measured jobs) lives in a seed and a user-doc sentence points at it by directory, never by planning id"

key-files:
  created:
    - .planning/seeds/SEED-015-whole-list-follow-ups.md
  modified:
    - docs/conformance.md
    - docs/testing.md
    - docs/maintaining.md

key-decisions:
  - "The bytes for the new maintaining.md sentences came from the per-slot seen and emitted list of Pinned downloads (the generator prints it on stderr), because the CI speed paragraph held no stale figures left to trim"
  - "The measured hosted jobs and the owner's standing instruction (evaluate low-hanging CI fruit whenever CI runs or finishes, without hurting quality) are recorded in SEED-015 and pointed to from the CI speed section of docs/maintaining.md"

requirements-completed: []

coverage:
  - id: C1
    description: "docs/conformance.md Whole-list ledger section states the exit command line, its claim-integrity legs, the three judged-cart floors as proof a library run reached the ledger and not a pass rate, that cart fail rows print and never block, the tie both ways and that the flag leaves on the owner's evidence"
    requirement: LEDG-05
    verification:
      - kind: other
        ref: "grep -q exit.cmake docs/conformance.md and grep -qi 'never on a pass rate' docs/conformance.md exit 0; the file is 35,075 bytes against 35,600"
        status: pass
    human_judgment: false
  - id: C2
    description: "docs/testing.md has one negative-control row for each of the 14 new tests, each naming the fault it prevents"
    requirement: LEDG-05
    verification:
      - kind: other
        ref: "a shell loop over the 14 names finds a row starting with each name; the two grep -c acceptance counts print 5 and 9; the file is 33,817 bytes against 34,300"
        status: pass
    human_judgment: false
  - id: C3
    description: "docs/maintaining.md points to the whole-list section, says a pin bump regenerates the table, catalog and pin counts in one run and that the tests compare them with the committed files, and carries the standing CI-speed instruction"
    requirement: LEDG-01
    verification:
      - kind: other
        ref: "test $(wc -c < docs/maintaining.md) -le 19870 exits 0 (19,866 bytes); ctest -R '^(docs\\.|policy\\.compat_doc_fresh|policy\\.notices|policy\\.claims)' passes"
        status: pass
    human_judgment: false
  - id: C4
    description: "No docs file reaches 38,912 bytes and the added doc lines hold no planning id"
    requirement: LEDG-04
    verification:
      - kind: other
        ref: "wc -c docs/*.md piped to awk exits 0; git diff -U0 origin/main -- docs README.md filtered for LEDG-0, DEC., Phase N, Plan N and SEED-N on added lines exits 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "SEED-015 lists every deferred item with why it waits and what evidence decides it, holds the CI-speed rewrite with its three rules, the CI items and the rejected ones, the phase's hosted measurements and the owner's standing instruction, and names no digest, path, title or address of any library"
    requirement: LEDG-01
    verification:
      - kind: other
        ref: "the task verify command exits 0 (file present, dormant, no 40-hex string in SEED-015 or SEED-013); the item-word grep prints 11; the release-input and dispatch-compares greps exit 0; the seeds diff holds no path or address"
        status: pass
    human_judgment: false
  - id: C6
    description: "SEED-013 agrees with the built command and needed no edit: the block holds library-judge, pal, fm and merge lines and the exit lines, an unmet verdict leaves the flag, the removal pull request text is the one the library judge plan wrote, and nothing waits for the owner"
    requirement: LEDG-05
    verification:
      - kind: other
        ref: "scripts/library.sh prints library-judge 1, the merge line and the exit lines (grep of the script); SEED-013 read against it, no contradiction"
        status: pass
    human_judgment: false
  - id: C7
    description: "The owner's own-library run and the removal of is_experimental in the pull request that merges its rows stay the batched hand-off of SEED-013, never a gate"
    requirement: LEDG-05
    verification:
      - kind: manual_procedural
        ref: "sh scripts/library.sh run with the owner's library and BIOS folders, pasting the library-judge and exit blocks"
        status: not_run
    human_judgment: true
---

# Phase 12 Plan 09: Docs and seeds Summary

**The exit command, its floors and the flag tie are documented where a maintainer looks, every new test has its negative-control row, and every deferred ledger and CI item (with the phase's measured hosted jobs and the owner's standing CI-speed instruction) is a findable seed.**

## Performance

- Tasks: 2, commits: 2 (`f8d33a7`, `ac0764d`), files: 4 (one created)
- Completed: 2026-11-12

## Accomplishments

- `docs/conformance.md` (35,075 bytes, cap 35,600): the Whole-list ledger section now carries the exit command line, its integrity legs in one sentence, the three floors, the cart `fail` rule, the tie both ways and "never on a pass rate".
- `docs/testing.md` (33,817 bytes, cap 34,300): 14 rows in the negative-controls table, each naming the fault its test prevents, written from each test's own registration comment.
- `docs/maintaining.md` (19,866 bytes, cap 19,870): a pointer to the whole-list section, the pin-bump regeneration sentence and the standing CI-speed instruction, with the stale per-slot count list trimmed.
- SEED-015 plants the deferred items, the CI-speed rewrite with its three rules, the nightly freshness step, the CI items (accepted and rejected) and the phase's hosted measurements.

## Docs sizes

| File | Bytes | Cap |
|------|-------|-----|
| docs/conformance.md | 35,075 | 35,600 |
| docs/testing.md | 33,817 | 34,300 |
| docs/maintaining.md | 19,866 | 19,870 |
| largest docs file (embedding.md) | 36,521 | 38,912 |

## Deviations from Plan

### Auto-fixed Issues

**1. [Plan text] The CI speed paragraph had no stale figures to trim**
- **Found during:** Task 1
- **Issue:** The plan paid for the new maintaining.md sentences by trimming the measured hash-file and sound-expectation seconds of the CI speed paragraph, but an earlier phase had already removed them; the file sat at 19,862 against 19,870.
- **Fix:** Trimmed the per-slot seen and emitted list in Pinned downloads (the generator prints the same counts on stderr and no test reads the doc's copy) and shortened one sentence of the CI speed paragraph; the paragraph was not rewritten.
- **Files modified:** docs/maintaining.md
- **Commit:** f8d33a7

**2. [Orchestrator instruction] The CI-efficiency record and the standing instruction**
- **Found during:** Task 2
- **Issue:** The orchestrator asked that the phase's hosted measurements (windows-arm64 build 214 s, be32 293 s, wasm32 55 s, hygiene about 45 s, the build timeout widened from 6 to 8 minutes by the mechanical rule) and the owner's standing instruction be recorded discoverably; the plan names no such section.
- **Fix:** Recorded both in a CI items section of SEED-015 and added one sentence to the CI speed section of docs/maintaining.md that points at the seeds directory (no planning id in a user doc). The plan's own run for D1 gives hygiene 38 s; SEED-015 says 38 to 45 s.
- **Files modified:** .planning/seeds/SEED-015-whole-list-follow-ups.md, docs/maintaining.md
- **Commit:** ac0764d, f8d33a7

**Total deviations:** 2 (one plan text, one added instruction). **Impact:** none on behaviour or budgets; no counted check file, test or data file changed.

## Verification

- `cmake --preset ci` exit 0, then `ctest --test-dir build/ci -R '^(docs\.|policy\.compat_doc_fresh|policy\.notices|policy\.claims|compat\.exit|compat\.whole_list\.count)'` exit 0 (20 of 20).
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0: 885 check lines / 9403 src lines, unchanged by this plan.
- `cmake --workflow --preset ci` exit 0 and `cmake --workflow --preset hygiene` exit 0 after the docs commit.
- No hosted run in this plan.

## Known Stubs

None.

## Threat Flags

None.

## Next Phase Readiness

Plan 10 closes the phase: dispatch D2, timeout settlement, floors, requirement marks, final budget and sizes.

## Self-Check: PASSED

- `.planning/seeds/SEED-015-whole-list-follow-ups.md` exists; commits `f8d33a7` and `ac0764d` are ancestors of HEAD.
