---
phase: 07-saves-survive-a-restart
plan: "10"
subsystem: ci
tags:
  - saves
  - retroarch
  - hosted-ci
  - close-out
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: the EEPROM and battery fixtures, the libretro save memory, and the runner --save-dir oracle (07-04 to 07-09)
provides:
  - tests/retroarch/run_retroarch.cmake keeps the RetroArch directory when KEEP is set, so a second pass reads the first save
  - the hosted retroarch job restarts RetroArch on battery.sms and on eeprom.gg and compares each saves/cogmaster/<name>.srm with the runner's byte for byte, with a control that the runner's two passes differ
  - the SAVE-03 wording and ROADMAP criterion 2 say what was built and measured
  - tests.ci and tests.wasm32 floors follow the final inventories
affects: []

actuals:
  tokens: 9000
  tasks: 3
  commits: 4

plan_head_before: aeaa02812516ce5d4c947def56c12be83702b766
plan_head_after: cad977acee00628aeae56380bb563b307b8535eb
commits: 4

tech-stack:
  added: []
  patterns:
    - "A real-frontend restart is two passes on one RetroArch directory (-DKEEP=1 on the second) against the runner run twice on one empty directory, plus a control that the runner's two outputs differ"

key-files:
  created: []
  modified:
    - tests/retroarch/run_retroarch.cmake
    - .github/workflows/ci.yml
    - docs/hosted.md
    - quality.json
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md

key-decisions:
  - "RetroArch writes the save to saves/cogmaster/<name>.srm (a per-core folder), so the steps read that path, not saves/<name>.srm as the plan assumed"
  - "cmake -E compare_files prints nothing on a difference with this CMake, so the control passes REGEX=^ and asserts the exit status 1 only"
  - "FRAMES=30 is enough for RetroArch to write both saves at its --max-frames exit; no raise was needed"
  - "The retroarch timeout stays at the 5-minute download floor; the job took 36 s"

requirements-completed:
  - SAVE-06
  - SAVE-03

coverage:
  - id: R1
    description: "RetroArch on the hosted macOS job runs the production core on battery.sms and eeprom.gg, restarts with the directory kept, and after each pass the .srm equals the cogmaster-run --save-dir oracle byte for byte"
    requirement: SAVE-06
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/42945252988 job retroarch, steps Battery save survives a RetroArch restart and EEPROM save survives a RetroArch restart"
        status: pass
    human_judgment: false
  - id: R2
    description: "A control shows the runner's first and second save differ, through expect_fail.cmake, so the restart really read the first save"
    requirement: SAVE-06
    verification:
      - kind: e2e
        ref: "same two steps; the final command of each is expect_fail.cmake over cmake -E compare_files"
        status: pass
    human_judgment: false
  - id: R3
    description: "The hosted steps use cmake -P and cmake -E compare_files only, tests/retroarch is at most 300 lines, and the retroarch timeout carries a measured comment"
    requirement: SAVE-06
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.(e2e_budget|timeouts)'"
        status: pass
    human_judgment: false
  - id: R4
    description: "SAVE-03 and ROADMAP criterion 2 read: import refuses lengths over the span, EEPROM import accepts only 0 or 128 bytes, and the layouts of other cores are claimed only where measured"
    requirement: SAVE-03
    verification:
      - kind: other
        ref: "grep -c 'EEPROM import accepts only 0 or 128 bytes' .planning/REQUIREMENTS.md .planning/ROADMAP.md prints 1 each"
        status: pass
    human_judgment: false
  - id: R5
    description: "The final phase head is green on a hosted run with every job, no audio, test-card or built pin line changed or removed, and the perf delta within +1.00 percent"
    requirement: SAVE-06
    verification:
      - kind: integration
        ref: "https://github.com/szTheory/cogmaster/actions/runs/42945252988"
        status: pass
    human_judgment: false
---

# Phase 7 Plan 10: Hosted save round trip and phase close Summary

A real RetroArch 1.22.2 now restarts on `battery.sms` and `eeprom.gg` on the hosted macOS job, reads the first run's save back through the production core, and leaves a `.srm` that equals the runner's after each pass; the wording, floors and final hosted run close the phase.

## Accomplishments

- `tests/retroarch/run_retroarch.cmake`: `file(REMOVE_RECURSE)` of the directory is skipped when `KEEP` is set (two added lines; the directories are re-created idempotently and `test.cfg` is rewritten).
- `.github/workflows/ci.yml`: the build job uploads `battery.sms` and `eeprom.gg` with the owned input; the retroarch job gains two steps, each plain commands. Per image: make an empty oracle directory, run RetroArch (pass 1), run `cogmaster-run --frames 30 --save-dir`, `cmake -E compare_files` the RetroArch `.srm` with the runner's, copy the runner's file aside, run RetroArch again with `-DKEEP=1`, run the runner again on the same directory, compare again, then `expect_fail.cmake` over a `compare_files` of the runner's first and second save (exit 1). The existing `RetroArch log` step still runs on failure only.
- `docs/hosted.md`: one paragraph on the save round trip and its control (5,445 bytes).
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`: SAVE-03 and criterion 2 amended as the plan states.
- `quality.json`: floors `tests.ci` 465 and `tests.wasm32` 434, the non-comment line counts of the two inventories.

## Task Commits

1. **Task 1: KEEP switch and the two hosted save round trips** - `6922b18` (test)
2. **Task 1 fix after the first hosted attempt: the per-core save folder** - `5e1f216` (test)
3. **Task 2: measured retroarch timeout** - `0f8ed3b` (ci)
4. **Task 3: wording and floors** - `cad977a` (docs)

## First hosted run

- Attempt 1: https://github.com/szTheory/cogmaster/actions/runs/42944235132, head `6922b18d082f963e97880d4827f96c2361d0c46f`, conclusion failure. Only `retroarch` (and so `CI required`) failed: RetroArch's log said `Saved successfully to .../ra-save-battery/saves/cogmaster/battery.srm` while the step compared `saves/battery.srm`, and `cmake -E compare_files` printed nothing. Every other job succeeded, including `wasm32` and the Windows legs (`build (windows-x64)`, `build (windows-arm64)`), which ran the 07-05 to 07-09 tests on a hosted run for the first time. No test needed a fix.
- Attempt 2: https://github.com/szTheory/cogmaster/actions/runs/42944763909, head `5e1f216a169c2e6239f32ff4f5ac97876fe475e2`, conclusion success, every job success (`rehearsal`, `cleanup` and `title` skipped on dispatch, as always).
- retroarch job duration from `gh run view 42944763909 --json jobs`: 36 s (02:17:47Z to 02:18:23Z); the two save steps took 2 s each. 2 x 36 s is under the 5-minute floor, so `timeout-minutes` stays 5 with the comment `5-minute floor (download); 2 x 36 s, slowest of 1 run, run 42944763909`. The acceptance grep prints 1 and `policy.timeouts` passes.
- e2e_budget: `e2e_budget: 275 lines under tests/retroarch, at most 300` (the policy test also counts the shared oracle); `wc -l tests/retroarch/*` totals 261 physical lines.
- Perf: the run's control line is `perf control boot_to_play runs=12302854,12302854,12302854 spread=0` (H=12302854). `origin/main` is still `1e2dfbf3d090e0072252992126563fcf50daed09`, 07-04's main SHA, so its value B=12302847 is reused. `delta_bp=0` by shell integer arithmetic (the difference is 7 instructions of 12,302,847). `git diff --quiet origin/main -- tests/perf/allow.txt` exits 0: no allow line.

## Final hosted run

- https://github.com/szTheory/cogmaster/actions/runs/42945252988, head `cad977acee00628aeae56380bb563b307b8535eb`, workflow_dispatch of `ci.yml` on `phase/07-saves-survive-a-restart`, conclusion success.
- Jobs, all success except three skipped on dispatch: hygiene, changes, perf, be32, asan, nofp (ubuntu-26.04), nofp (ubuntu-26.04-arm), build (linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64, windows-arm64), wasm32, retroarch, hash-equality, CI required; title, rehearsal and cleanup skipped. The fuzz regression tests (`fuzz.regress.*`, including the EEPROM entry) run inside the build and asan jobs; there is no separate fuzz job in this workflow, and the local libFuzzer cannot run on this Mac.
- `git diff --quiet cad977acee00628aeae56380bb563b307b8535eb HEAD -- . ':(exclude).planning'` exits 0: the run's head equals HEAD outside `.planning`.
- Perf control line of this run: `perf control boot_to_play runs=12302854,12302854,12302854 spread=0`, `delta_bp=0`.

## Pin proof

From the repository root:

| Command | Exit |
| --- | --- |
| `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` | 0 |
| `git diff origin/main -- tests/built > "$TMPDIR/built.diff"` | 0 |
| `grep -c '^-[^-]' "$TMPDIR/built.diff"` | prints 0 (grep exit 1 for no match, as expected) |

The one added line in `tests/built` is `eeprom 1f7ed7d1...`; no line was changed or removed.

## Budgets

| Command | Result |
| --- | --- |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | exit 0; 723 check lines of at most 748, 8270 src lines (no check code was added in this plan) |
| `cmake --workflow --preset ci` | exit 0, 461 tests passed |
| `cmake --workflow --preset hygiene` | exit 0 |
| `ctest --test-dir build/ci -R '^policy\.'` | exit 0 |
| `actionlint .github/workflows/ci.yml` | only the known `ubuntu-26.04` runner-label metadata findings |

`include/cogmaster.h` is 9,194 bytes (cap 24,576). The largest document, `docs/embedding.md`, is 34,011 bytes against the 38,912 cap, 4,901 bytes under; `docs/hosted.md` is 5,445.

## Phase notes

- The libretro_saves flip landed with its guard in plan 07-07, not in the hosted plan (CONTEXT's libretro decision overrides the process list).
- The host guard uses the owned 32 KiB battery fixture because no 8 KiB fixture is owned.
- The runner plan followed the import plan.
- The kill test's control is the same interrupt build without `--save-interval`, not a separate exit-after-replace fault.
- The directory-sync fault test is registered on every lane with the same name (CONTEXT's platform-gated registration is superseded because the `ci` inventory must match on every lane); only its expected behaviour differs per platform.
- The runner durability work was split into two plans: 07-08 interval, 07-09 sweep, directory sync and kill.
- The `.srm` measurement verdicts (plan 07-07): Gearsystem writes a 128-byte little-endian EEPROM `.srm` and Genesis Plus GX a 3-byte file for the battery fixture; both wrote under a per-core folder of the save directory, the same folder RetroArch used for the production core here; nothing was left unmeasured.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The save path of the plan's steps was wrong**
- **Found during:** Task 2, first hosted run 42944235132
- **Issue:** RetroArch writes `saves/cogmaster/<name>.srm`, not `saves/<name>.srm`, so the first compare failed silently.
- **Fix:** the four `compare_files` commands read `build/ra-save-<name>/saves/cogmaster/<name>.srm`.
- **Files modified:** `.github/workflows/ci.yml`
- **Commit:** `5e1f216`

**2. [Rule 1 - Bug] compare_files prints nothing on a difference**
- **Found during:** Task 1, local check of the control
- **Issue:** `expect_fail.cmake` requires a matching output and this CMake prints none for a differing pair.
- **Fix:** the control uses `REGEX=^`, which matches empty output, so it asserts exit status 1 only. The preceding compare steps already require both files to exist.
- **Files modified:** `.github/workflows/ci.yml`
- **Commit:** `6922b18`

**Total deviations:** 2 auto-fixed (both Rule 1). **Impact:** none on scope; FRAMES was never raised, one extra hosted attempt was spent.

## Known Stubs

None.

## Ship-time confirmations

`/gsd-ship` (or the session after the merge) runs these from the repository root and records each command, its exit status and the run URL. Items 0 to 3 need the phase pull request; item 4 needs the merge.

0. **Hash equality on the pull request head.** `gh pr view phase/07-saves-survive-a-restart --json headRefOid --jq .headRefOid > "$TMPDIR/head.txt"`; `gh run list --workflow ci.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId,url > "$TMPDIR/head-runs.json"` and `jq -e 'length > 0' "$TMPDIR/head-runs.json"`; `gh run watch <id> --exit-status`; `gh run view <id> --json jobs > "$TMPDIR/head-jobs.json"`; `jq -e '[.jobs[] | select(.name | test("hash-equality|retroarch"))] | length >= 2 and all(.conclusion == "success")' "$TMPDIR/head-jobs.json"` (exit 0). Append the pin-proof commands with their exit statuses and the hash-equality run URL to the pull request body (`gh pr view ... --json body --jq .body > "$TMPDIR/body.md"`, edit, `gh pr edit ... --body-file "$TMPDIR/body.md"`).
1. **Perf gate on the pull request.** `gh run view <id> --log > "$TMPDIR/pr.log"` and `grep -Eq 'perf boot_to_play head=[0-9]+ base=[0-9]+ delta_bp=-?[0-9]+ verdict=PASS' "$TMPDIR/pr.log"` (exit 0); record the line in the pull request body.
2. **AGENTS rule 10: title, auto-merge, CI on the head.** `gh pr view phase/07-saves-survive-a-restart --json title --jq .title > "$TMPDIR/title.txt"` and `grep -Eq '^(feat|fix|docs|ci|test|refactor|perf|build|chore)(\([a-z0-9-]+\))?!?: ' "$TMPDIR/title.txt"`; if it exits 1, `gh pr edit phase/07-saves-survive-a-restart --title "feat: keep Game Gear EEPROM and battery saves across restarts"` and re-check. In a shell, never inside GSD command text: `gh pr merge phase/07-saves-survive-a-restart --auto --squash` (exit 0).
3. **Fuzz lane on the head.** `jq -e '[.jobs[] | select(.name | test("fuzz"))] | all(.conclusion == "success")' "$TMPDIR/head-jobs.json"` (exit 0); the dispatch runs have no job named fuzz, so if the pull request run has none either the check passes vacuously and the fuzz regression tests inside the build and asan jobs are the evidence.
4. **Release pull request green after the merge** (every phase ends in a release; never held). `gh pr list --label 'autorelease: pending' --json number,url,statusCheckRollup > "$TMPDIR/release-pr.json"` and `jq -e 'length == 1 and (.[0].statusCheckRollup | all((.conclusion // .state) | IN("SUCCESS", "SKIPPED", "NEUTRAL")))' "$TMPDIR/release-pr.json"` (exit 0); afterwards `gh run list --workflow release.yml --branch main --limit 1 --json conclusion,url` shows success.
5. **Allow line, if any.** No `tests/perf/allow.txt` line was added in this phase, so Phase 8 has nothing to delete.

## Self-Check

- Task 1 acceptance: policy, repository and docs ctest exit 0; `grep -c KEEP` prints 1; `git diff --numstat origin/main -- tests/retroarch/run_retroarch.cmake` shows 3 added and 1 deleted (the delete is the line rewrapped in `if`; 2 net lines); `grep -c compare_files` prints 6; the artifact path grep prints 2; `actionlint` reports only the known runner-label findings.
- Task 2 acceptance: the timeout grep prints 1; both save steps concluded success in run 42944763909 (`jq` over its jobs file: the `retroarch` job conclusion is success).
- Task 3 acceptance: wording greps print 1, 1 and 0; the final run concluded success; pin proof and budgets above.

## Self-Check: PASSED
