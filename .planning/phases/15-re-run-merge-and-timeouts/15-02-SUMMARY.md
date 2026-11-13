---
phase: 15-re-run-merge-and-timeouts
plan: 02
subsystem: ci-timeouts
tags:
  - ci
  - timeouts
  - workflows
  - docs
  - seeds
  - ship-handoff

requires:
  - phase: 15-re-run-merge-and-timeouts
    provides: plan 01's ledger and page commit, so the workflow commit can be the last non-planning commit
provides:
  - the job timeouts of ci.yml, nightly.yml and digest.yml re-derived from a fresh 40-run read, each comment naming its run, in one commit
  - one CI-speed paragraph in docs/maintaining.md
  - SEED-015 and SEED-016 breadcrumbs for the timeout findings and the pad press and bank-shift record
  - the counts-only ship title and body, the timeouts table with run URLs and the final gates in 15-RUN-EVIDENCE.md
affects: [ship of phase 15, code review of phase 15]

actuals:
  tokens: 5500
  tasks: 2
  commits: 2
plan_head_before: aae26f11b5b20ed59398da29d50af4f86a9a829b
plan_head_after: de463f25e085b436afb212306bf9ecf78487ae2c

tech-stack:
  added: []
  patterns:
    - "read the live durations once into saved files, edit from the saved rows, then cross-check the edited lines against those files, because the hygiene arithmetic check cannot see drift"
    - "a squash body kept to counts, with workflows named by display name so it holds no file name"

key-files:
  created:
    - .planning/phases/15-re-run-merge-and-timeouts/15-02-SUMMARY.md
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - .github/workflows/digest.yml
    - docs/maintaining.md
    - .planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md
    - .planning/seeds/SEED-015-whole-list-follow-ups.md
    - .planning/seeds/SEED-016-failing-carts.md
    - .planning/STATE.md

key-decisions:
  - "The comment on each non-floor line is the script's comment alone (2 x S s, slowest of N runs, run ID); the old worst-lane suffix on the bench line went with the stale numbers, since the script already reports the slowest lane instance"
  - "fuzz and digest keep the 5-minute floor and its phrase, because the floor still sets their minutes (2 x 144 s is 288 s; 2 x 55 s is 110 s); asan drops the floor phrase, because the rule now gives 9"
  - "build and be32 were corrected in the same edit: both were below twice the fresh slowest (540 s against 664 s, 900 s against 1218 s), and the cause is test growth, not the runner"
  - "The ship title keeps the fix type (a patch release) and the body names each workflow by display name (CI, Nightly, Digest), never by file name"

requirements-completed:
  - TUNE-11

coverage:
  - id: D1
    description: "The durations were read fresh, immediately before the edit, for the three workflows, with nothing dispatched"
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "sh scripts/ci-durations.sh ci.yml 40, nightly.yml 40 and digest.yml 40, each redirected to a saved file under the temp directory, exit status 0 each time"
        status: pass
    human_judgment: false
  - id: D2
    description: "asan, bench and bench-wasm32 minutes and the fuzz and digest comments are re-derived by the twice-the-slowest rule, and build and be32 are corrected, each edited line carrying the saved read's minutes, seconds and run ID"
    requirement: TUNE-11
    verification:
      - kind: integration
        ref: "cmake -DDIR=.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake exit status 0, then the saved-read cross-check loop over the seven lines exit status 0, then the timeout-lines-only diff filter exit status 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "All three workflow files changed in exactly one commit, which is the newest commit touching anything outside .planning"
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "git log --format=%H origin/main..HEAD -- .github/workflows/ci.yml counts 1 and that commit also touches the other two files, exit status 0; the newest non-planning commit equals it, exit status 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The CI-speed paragraph landed in docs/maintaining.md with no planning label and under the docs cap"
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "grep -Eqi on the open-once and compares-the-ledger-with-itself phrases exit status 0; the planning-label grep counts 0; wc -c is 20372 against the 38912 cap"
        status: pass
    human_judgment: false
  - id: D5
    description: "SEED-016 no longer says the pad press was not taken and cites the Phase 14 UAT record and the Phase 15 evidence note; SEED-015 records the timeout findings and what stays open"
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "grep -c 'Not taken, for Phase 15' on SEED-016 prints 0; grep -Ec on SEED-016 prints 2 and on SEED-015 prints 3"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ci and hygiene workflows pass on the final head and every branch fence holds"
    requirement: TUNE-11
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit status 0 (676 tests in the test step, 5 in the release test step) and cmake --workflow --preset hygiene exit status 0 (84 tests), then the plan's Task 2 verify command exit status 0; the hosted CI run URL for the pushed head is recorded at ship"
        status: pass
    human_judgment: false
  - id: D7
    description: "The counts-only ship title and body are written once in the evidence note, with the fixes' flips kept apart from newly judged rows, and the body names no file"
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "the Task 2 greps over the Body region of the note for the flipped and newly judged lines and for file extensions, home paths, skip tokens and the forbidden phrase, exit status 0"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-13
status: complete
---

# Phase 15 Plan 02: Re-derive the job timeouts and write the ship hand-off Summary

**The seven drifted timeout lines were re-derived from a fresh 40-run read and landed in one workflow commit (asan 9, build 12, be32 21, bench 13, bench-wasm32 16 minutes, with the fuzz and digest comments re-cited), the hygiene check and a saved-read cross-check both exit 0, and the counts-only ship text is ready for the pull request.**

code review: pending (/gsd-code-review 15 runs next)

## Performance

- **Duration:** about 10 min (the start time was not captured at spawn; the first read ran at 2026-11-13T19:04Z)
- **Started:** about 2026-11-13T19:02:00Z
- **Completed:** 2026-11-13T19:12:00Z
- **Tasks:** 2
- **Files modified:** 8 outside the SUMMARY (three workflows, the maintaining guide, the evidence note, two seeds, STATE.md)

## Accomplishments

- The three reads (`sh scripts/ci-durations.sh ci.yml 40`, `nightly.yml 40`, `digest.yml 40`) each exited 0, with no workflow dispatched, and were saved so the edit and its cross-check work from the same rows.
- Timeouts, before to after in minutes, each with the run the comment names: CI `asan` 5 to 9 (2 x 243 s, run 43462502049), CI `build` 9 to 12 (2 x 332 s, run 43460663238), CI `be32` 15 to 21 (2 x 609 s, run 43458609482), Nightly `bench` 10 to 13 (2 x 368 s, run 42764095033), Nightly `bench-wasm32` 15 to 16 (2 x 456 s, run 42768407261). Nightly `fuzz` (2 x 144 s, run 43020467034) and Digest `digest` (2 x 55 s, one run, 42769805382) keep 5 minutes and now cite a live run instead of a stale one.
- No job's minutes fell; every edit loosens a limit or re-cites a run. `build` and `be32` were below the rule (9 minutes against 664 s; 15 against 1218 s) and are corrected in the same commit; the cause is test growth in the diagnose tests, not the runner. wasm32, retroarch, perf, hygiene and nofp keep their limits and older run references, because the rule holds for them against the same read (the narrowest margin is the rehearsal, 780 s against 770 s, untouched).
- `hygiene.timeout_comments` (`cmake -DCHECK=timeouts`) exits 0, the cross-check of each edited line against the saved rows exits 0, and only `timeout-minutes:` lines changed in the three files. Exactly one commit touches `ci.yml`, and it is the newest commit on the branch outside `.planning/`.
- `docs/maintaining.md` gained one three-sentence paragraph (open the pull request once, the rehearsal cost of a release-input edit, a dispatch comparing the ledger with itself), with no planning label, at 20372 bytes.
- SEED-016 now cites the Phase 14 UAT pad press and says why the Sega bank-shift trace stays out; SEED-015 records the timeout outcome and the open items (per-test timeouts, a cheaper be32 lane, the stale critical-path sentence).
- The note holds the timeouts table with run URLs, the final gates and the counts-only ship title and body: 285 rows flipped from fail to pass, 0 newly judged, no other transition.

## Task Commits

1. **Task 1: Fresh durations to one workflow commit** - `afd413e` (ci)
2. **Task 2: Seed breadcrumbs, ship hand-off text, final gates and fences** - `de463f2` (docs)

**Plan metadata:** the docs commit that carries this file, STATE.md and ROADMAP.md

## Files Created/Modified

- `.github/workflows/ci.yml` - `asan`, `build` and `be32` timeout lines only
- `.github/workflows/nightly.yml` - `bench`, `bench-wasm32` and `fuzz` timeout lines only
- `.github/workflows/digest.yml` - the `digest` timeout line only
- `docs/maintaining.md` - one paragraph in the CI speed section
- `.planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md` - `## Timeouts`, `## Final gates` and `## Ship hand-off`
- `.planning/seeds/SEED-015-whole-list-follow-ups.md`, `.planning/seeds/SEED-016-failing-carts.md` - breadcrumbs
- `.planning/STATE.md` - the Runtime Log row for this step

## Decisions Made

The four decisions in the frontmatter. The squash body calls the workflows CI, Nightly and Digest, never by file name, which is the settled reading of the no-file-names rule.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- A scratch awk loop that checked the rule for every job first keyed on the job name with leading spaces (the field separator was a tab) and reported "no fresh row" for every job; the key extraction was fixed in the scratch command and the loop then reported "holds" for all 20 timeout lines. Nothing in the tree was involved.

## Known Stubs

None.

## Threat Flags

None - only `timeout-minutes:` lines and their comments changed in the workflows; no new endpoint, auth path, file access or schema at a trust boundary.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Review status: code review: pending (`/gsd-code-review 15` runs next and records run or not applicable in its own review record), then `/gsd-verify-work 15`, then `/gsd-ship 15`.
- At ship the branch is pushed once and the pull request opened with the title and body in the note's `## Ship hand-off`. Nothing was pushed or dispatched in this plan; the workflow edits are exercised by the pull request's own CI run (the release rehearsal runs once because `ci.yml` is edited, and Nightly runs once on its own path filter).
- After the push: the core-commit reachability check from plan 01, the CI run on the head commit, then `gh pr merge --auto --squash` in a shell, and after the merge the release pull request and the `autorelease: pending` label check.

## Self-Check: PASSED

The three workflow files, the maintaining guide, the evidence note, both seeds and this file exist; commits afd413e and de463f2 are ancestors of HEAD; the measured commit count since the plan base is 2, matching `commits:`; every acceptance criterion of Task 1 and Task 2 was re-run after its commit and exited 0, and the Task 2 verify command (both workflows plus the fences) exited 0 on the final head.

---
*Phase: 15-re-run-merge-and-timeouts*
*Completed: 2026-11-13*
