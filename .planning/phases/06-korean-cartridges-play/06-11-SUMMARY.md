---
phase: 06-korean-cartridges-play
plan: "11"
subsystem: phase-close
tags:
  - requirements
  - perf
  - hosted-ci
  - ship-list
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plans 06-04 perf control value, 06-05 size ceiling, 06-09 and 06-10 fixtures and tests
provides:
  - MAP-04, MAP-05 and Phase 6 criterion 2 worded as catalog row
  - quality.json floors tests.ci 438 and tests.wasm32 411 equal to the inventories
  - a green hosted run of the final head with eight-lane hash equality, boot_to_play delta_bp 0 and the asan job green
  - the MAP-08 local proof and the ship-time confirmation list
affects: [06-ship, 07]

actuals:
  tokens: 1500
  tasks: 3
  commits: 1

plan_head_before: 01af4b26352dff3f0c2ff7445c5759bc9a0abd5c
plan_head_after: 778376ae243da58317b8b83a80137126c89c9533
commits: 1

tech-stack:
  added: []
  patterns:
    - "Hosted evidence is a dispatched run on the branch read with gh, never a committed log"

key-files:
  created: []
  modified:
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - quality.json

key-decisions:
  - "No allow line: boot_to_play delta_bp is 0, so tests/perf/allow.txt is untouched"
  - "MAP-02 keeps its ledger side wording; the catalog lives in tests/compat beside the ledger"

requirements-completed:
  - MAP-08
  - MAP-04
  - MAP-05

coverage:
  - id: C1
    description: "MAP-04, MAP-05 and Phase 6 criterion 2 say catalog row; Phase 12 keeps its ledger wording"
    requirement: MAP-04
    verification:
      kind: other
      ref: "the task 1 verify command (grep over REQUIREMENTS.md and ROADMAP.md, jq over quality.json) exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "Floors tests.ci 438 and tests.wasm32 411 equal the non-comment inventory line counts and policy.quality passes"
    requirement: MAP-08
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^policy\\.quality$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "The final head has a green dispatched hosted run with hash-equality and asan success, and boot_to_play delta_bp 0 against main"
    requirement: MAP-08
    verification:
      kind: e2e
      ref: "https://github.com/szTheory/cogmaster/actions/runs/42787536409, gh run watch exit 0, jq over jobs exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "No audio or test card pin changed and no built pin line was removed or changed"
    requirement: MAP-08
    verification:
      kind: other
      ref: "git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0; git diff origin/main -- tests/built exit 0 with 0 removed lines"
      status: pass
    human_judgment: false
  - id: C5
    description: "Seo Jin and 188-in-1 appear as unsupported catalog rows and Janggun and 4PAK play or are unsupported catalog rows"
    requirement: MAP-05
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci exit 0 (434 of 434 tests pass, plus 4 release tests)"
      status: pass
    human_judgment: false
  - id: C6
    description: "Local ci and hygiene workflows pass and the check-line, src-line, docs and header budgets hold"
    requirement: MAP-04
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 25 min
completed: 2026-11-10
---

# Phase 6 Plan 11: Phase close Summary

The requirement wording now matches what the phase built (catalog row), the test floors hold the final counts (438 and 411), and a dispatched hosted run of the final head is green on all jobs with eight-lane hash equality, boot_to_play at delta_bp 0 and no shipped pin moved.

## Accomplishments

- MAP-04, MAP-05 and Phase 6 criterion 2 say `catalog row`; Phase 12's goal still says `ledger row`. MAP-02's `ledger side` is unchanged because the catalog lives in tests/compat beside the ledger.
- `quality.json` floors `tests.ci` 438 and `tests.wasm32` 411 equal the non-comment lines of `tests/inventory/ci.txt` and `wasm32.txt`, so a later drop of the new tests fails `policy.quality`.
- The final head is proven on hosted CI, the perf delta is taken, and the MAP-08 pins are shown unchanged.

## Final hosted run

Commands: `git push origin phase/06-korean-cartridges-play` (exit 0), `gh workflow run ci.yml --ref phase/06-korean-cartridges-play` (exit 0), `gh run watch 42787536409 --exit-status` (exit 0), `gh run view 42787536409 --log` and `--json conclusion,jobs`.

- Run URL: https://github.com/szTheory/cogmaster/actions/runs/42787536409 (run ID 42787536409, exported as `FINAL_RUN`)
- Head SHA: 778376ae243da58317b8b83a80137126c89c9533; `git diff --quiet 778376ae243da58317b8b83a80137126c89c9533 HEAD -- . ':(exclude).planning'` exit 0
- Conclusion success; every job success (hygiene, changes, nofp on both architectures, asan, be32, build on seven legs, perf, wasm32, retroarch, hash-equality, the required-check job); title, cleanup and rehearsal skipped by design.
- hash-equality downloaded `hashes-*`: 8 artifacts (windows-x64, windows-arm64, linux-x64, linux-arm64, macos-x64, macos-arm64, wasm32, be32), each 2906 bytes, and concluded success.
- asan concluded success (it runs the mapper.ext tests under ASan and UBSan).
- Perf control line: `perf control boot_to_play runs=12302847,12302847,12302847 spread=0`.
- Main value used: B=12302839 from plan 06-04 (main run https://github.com/szTheory/cogmaster/actions/runs/42781713344 at head 8c97f8e1d073d22b69e615f37c917886d06d73eb); `git rev-parse origin/main` after `git fetch origin main` is still 8c97f8e1d073d22b69e615f37c917886d06d73eb, so the value is reused and main was not dispatched again.
- H=12302847, B=12302839, delta_bp = (H - B) * 10000 / B = 0 (8 instructions per frame). The limit is 100, so no allow line was added: `git diff --quiet origin/main -- tests/perf/allow.txt` exit 0.
- size.cartdb on the Linux x64 leg: `quality: size.cartdb measured=65168 ceiling=102400`.
- Verify command: `jq -e` over the jobs JSON exit 0 (conclusion success, hash-equality and asan success).

## MAP-08 local proof

From the repository root, after `git fetch origin main` (origin/main 8c97f8e1d073d22b69e615f37c917886d06d73eb):

- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0
- `git diff origin/main -- tests/built > "$TMPDIR/built.diff"` exit 0, then `grep -c '^-[^-]' "$TMPDIR/built.diff"` prints 0 (3 added pin lines, none removed or changed)
- The same commands run again at ship time go into the pull request body (list item 0).

## Budgets and local runs

- `cmake --workflow --preset ci` exit 0 (434 of 434 tests, plus 4 release tests); `cmake --workflow --preset hygiene` exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: 723 check lines / 7957 src lines, strict one tenth (the plan's ceiling was 754 check lines).
- Largest doc is docs/embedding.md at 33761 bytes against the 38912 cap (5151 bytes of room, more than 2 KiB); `include/cogmaster.h` is 9114 bytes against 24576.

## Ship-time confirmations

`/gsd-ship` (or the session after the merge) runs these from the repository root and records each command, its exit
status and the run URL. Items 0 to 3 need the phase pull request; items 4 and 5 need the merge.

0. **MAP-08 proof in the pull request body.** After `/gsd-ship` opens the pull request:
   `gh pr view phase/06-korean-cartridges-play --json headRefOid --jq .headRefOid > "$TMPDIR/head.txt"`;
   `gh run list --workflow ci.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId,url > "$TMPDIR/head-runs.json"`
   and `jq -e 'length > 0' "$TMPDIR/head-runs.json"`; wait with `gh run watch <id> --exit-status`; then
   `gh run view <id> --json jobs > "$TMPDIR/head-jobs.json"` and
   `jq -e '[.jobs[] | select(.name | test("hash-equality"))] | length >= 1 and all(.conclusion == "success")' "$TMPDIR/head-jobs.json"`
   (exit 0). Append to the body (`gh pr view phase/06-korean-cartridges-play --json body --jq .body > "$TMPDIR/body.md"`,
   add the lines, `gh pr edit phase/06-korean-cartridges-play --body-file "$TMPDIR/body.md"`): the command
   `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` with exit 0, the command
   `git diff origin/main -- tests/built > "$TMPDIR/built.diff"` with exit 0 (git's own status), then
   `grep -c '^-[^-]' "$TMPDIR/built.diff"` with its output 0, and the hash-equality run URL on the head commit. Confirm with `gh pr view phase/06-korean-cartridges-play --json body --jq .body | grep -q 'hash-equality'`.
   Any removed or changed pin line is a stop.
1. **Perf gate on the pull request.** From the same run's log (`gh run view <id> --log > "$TMPDIR/pr.log"`),
   `grep -Eq 'perf boot_to_play head=[0-9]+ base=[0-9]+ delta_bp=-?[0-9]+ verdict=PASS' "$TMPDIR/pr.log"`
   exits 0 (an allow line raises the limit, so an allowed delta still prints PASS); record the line in the pull
   request body. The dispatch run printed only the control line (no base to compare), so this line first appears on the pull request run.
2. **size.cartdb hosted value.** From the Linux x64 build leg's Size ceilings step in that log,
   `grep -Eq 'quality: size.cartdb measured=[0-9]+ ceiling=102400' "$TMPDIR/pr.log"` exits 0; record the measured
   value (the dispatch run above measured 65168). After the merge, a `ci:` follow-up sets the ceiling to that value plus about 15 percent, rounded up to a
   whole kilobyte (65168 gives 75 KiB, 76800 bytes), with a reasons entry naming the run (a tightening needs no reason, but the source run is kept).
3. **AGENTS rule 10: title, auto-merge, CI on the head.** `gh pr view phase/06-korean-cartridges-play --json title --jq .title > "$TMPDIR/title.txt"`
   and `grep -Eq '^(feat|fix|docs|ci|test|refactor|perf|build|chore)(\([a-z0-9-]+\))?!?: ' "$TMPDIR/title.txt"`;
   if it exits 1, `gh pr edit phase/06-korean-cartridges-play --title "feat: play Korean-board cartridges with no setting"`
   and re-check. In a shell, never inside GSD command text: `gh pr merge phase/06-korean-cartridges-play --auto --squash`
   (exit 0). The asan job on the head run concluded success
   (`jq -e '[.jobs[] | select(.name | test("asan"))] | all(.conclusion == "success")' "$TMPDIR/head-jobs.json"`).
4. **Release pull request green after the merge** (every phase ends in a release; never held).
   `gh pr list --label 'autorelease: pending' --json number,url,statusCheckRollup > "$TMPDIR/release-pr.json"` and
   `jq -e 'length == 1 and (.[0].statusCheckRollup | all((.conclusion // .state) | IN("SUCCESS", "SKIPPED", "NEUTRAL")))' "$TMPDIR/release-pr.json"`
   (exit 0); afterwards `gh run list --workflow release.yml --branch main --limit 1 --json conclusion,url` shows success.
5. **Allow line, if any.** No `tests/perf/allow.txt` line was added (delta_bp 0), so Phase 7 has nothing to delete.

## Phase notes

- The eeprom slot stays unsupported until the saves phase (planner's choice).
- The scan thresholds are 3 with the Sega margin rule; no private library was configured, so real dumps did not measure them and the seed `.planning/seeds/SEED-009-scan-thresholds.md` records the measurement to take.
- The title-on-hit check lives in `cart.fixture_identity`, not `tests/api/v3.c`, because v3.c takes no image and no ROM byte may be committed.
- The size.cartdb reason in `quality.json` names no planning ID (style rule); `grep -cE 'MAP-[0-9]' quality.json` prints 0.
- Hi-Com, Korean no-bank, Seo Jin and 188-in-1 are hash-only (no committed image).
- `cogmaster_cart_info` carries `unsupported_reason[128]`, a zero-terminated `<slot>: <reason>` sized for a 15-byte slot name plus a 95-byte reason (planner's choice).
- The unsupported id is the generator's stable slot id (0 for none), not a separate hand-written enum of none, seo_jin and 188_in_1, so it covers every named-unsupported slot (planner's reading of the CONTEXT rule that the id is generated from the slot table).
- The Nemesis signature is the byte at body minus 8 KiB being $F3 while byte 0 is not (planner's choice, from MAME's power-on slot 0 on the last 8 KiB bank).
- The 8 KiB Game Gear refusal is ordered before the header rule so the 8 KiB cheat device stays refused whatever its header says (planner's ordering; CONTEXT lists the refusal under the size rule).
- The ci inventory has 438 lines: 434 run by `cmake --workflow --preset ci` plus 4 release tests; both floors equal their inventories.

## Deviations from Plan

None - plan executed exactly as written. Task 2 produced no tree change (delta_bp 0, no allow line), so it carries no commit of its own; its evidence is this file.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. No network endpoint, auth path or trust-boundary schema was added; the plan edits planning text and two floor numbers.

## Next Phase Readiness

The phase is ready for code review and then `/gsd-ship`. Hosted evidence is on the head; the pull request run, the size.cartdb follow-up and the release pull request remain for ship.

## Self-Check: PASSED

Modified files found (`.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `quality.json`); commit 778376a is an ancestor of HEAD; run URL 42787536409 recorded; the task 2 verify command exits 0.
