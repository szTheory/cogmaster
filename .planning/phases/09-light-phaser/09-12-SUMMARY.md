---
phase: 09-light-phaser
plan: "12"
subsystem: docs-and-close
tags:
  - light-phaser
  - docs
  - requirements
  - hosted-dispatch
status: complete

requires:
  - 09-04
  - 09-07
  - 09-08
  - 09-10
  - 09-11
provides:
  - the Light Phaser section of docs/controllers.md with every constant by name and marked unmeasured
  - GUN-02 and GUN-03 and the ROADMAP criteria 3 and 4 carry the wording of what was built; GUN-01 to GUN-04 are complete
  - the SEED-010 local-judge hand-off for the gun and the run-frame sync finding
  - hosted dispatch 3 of ci.yml, a fresh main dispatch for perf and a nightly dispatch, all green, and every provisional timeout of the phase settled
affects: []

actuals:
  tokens: 6700
  tasks: 3
  commits: 3

plan_head_before: 2760e8da1e1ee3b98fee7c30e4674298cf7c8995
plan_head_after: bbc9abcc48f124db2791cf8f47b90795b5d9a60e
commits: 3

tech-stack:
  added: []
  patterns:
    - "A shared registration loop is split when a new family needs a different measured timeout, rather than keeping the older family's larger value for both"

key-files:
  created: []
  modified:
    - docs/controllers.md
    - .planning/seeds/SEED-010-pad-device-notes.md
    - quality.json
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - tests/host/tests.cmake
    - tests/libretro/tests.cmake
    - tests/ports/tests.cmake

key-decisions:
  - "A soft reset and a power reset keep the aim and only a load parks it, stated in the guide beside the Sports Pad's waiting motion (the planning text said a power reset clears it; the code, read in plan 03, does not)"
  - "The three libretro gun cases leave the 60 s adapter loop for their own loop at the 10 s floor, because the slowest hosted reading is 1.72 s and the loop's 60 s belonged to the older stereo cases"
  - "No job timeout in ci.yml is edited: the slowest job of the final dispatch, be32 at 183 s of 600 s, is under half its timeout, and so is every other job"

requirements-completed:
  - GUN-02
  - GUN-03
  - GUN-04

coverage:
  - id: D1
    description: "docs/controllers.md documents the gun once: the availability row, the table default and its escape, the last-wins aim and the reset rule, the runner script, the libretro sources and the crosshair, the model and each constant by name marked unmeasured, the TH rule with the sticky bit, the latch rule and where MAME agrees, the no-shot invariant, the ordering of edges, the cost and the limits, with no planning ID and 17224 bytes of a 38912 cap"
    requirement: GUN-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0 (39 tests); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; grep for planning IDs in docs/controllers.md prints 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "REQUIREMENTS GUN-02 and GUN-03 and ROADMAP criteria 3 and 4 carry the amended wording verbatim, the superseded wording is gone, GUN-01 to GUN-04 are marked complete with their traceability rows, the Phase 9 plan list is ticked and counted 12/12, and the floors tests.ci 588 and tests.wasm32 548 equal the inventories"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^policy\\.quality' exit 0 (10 tests); the plan's grep chain over REQUIREMENTS.md and ROADMAP.md exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "The head runs green on hosted lanes: ci.yml dispatched on the branch (every job success, 8ed0a52), a fresh main dispatch for the perf base, and a nightly dispatch whose fuzz job is green; delta_bp on boot_to_play is 0"
    requirement: GUN-04
    verification:
      - kind: e2e
        ref: "gh run watch 43015673654 --exit-status exit 0; gh run watch 43015683127 --exit-status exit 0; gh run watch 43015678841 --exit-status exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "Nothing pinned moved: tests/io, tests/vdp, tests/bus, tests/audio, tests/testcard_pins.cmake and tests/ports/test_ports.c are byte-equal to origin/main, no line of the built, diag or manifest data is removed or changed, the perf allow list and ci.yml are unchanged"
    requirement: GUN-04
    verification:
      - kind: integration
        ref: "git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c exit 0; removed-line count over tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt prints 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "Every provisional timeout of the phase is settled from the hosted per-test times and no test the phase added or changed took more than half its timeout"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "grep -rn 'floor; provisional' tests/ports/tests.cmake tests/host/tests.cmake tests/libretro/tests.cmake tests/fixtures/tests.cmake tests/runner/tests.cmake prints nothing; cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
---

# Phase 9 Plan 12: Close of the Light Phaser phase Summary

The Light Phaser is documented once in `docs/controllers.md` with its constants marked unmeasured, the requirement and roadmap wording says what was built, GUN-01 to GUN-04 are complete, and the head ran green on a hosted dispatch, a fresh main perf base and the nightly fuzz lane with a perf delta of 0 basis points.

## Performance

- **Duration:** about 20 min of work plus the hosted waits
- **Completed:** 2026-11-11T12:01Z
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- `docs/controllers.md` (9867 to 17224 bytes): the matrix row and the Game Gear and Mark III sentence; step 1 and the probe words naming `lphaser`, with the bare-run default and the `--port1 pad` escape; the note beside "send each increment once" that a later axis-free call parks the aim; the reset rule (soft and power reset keep the aim, only a load parks it); a "Light Phaser" section with the aim encoding and the one aim given up, the runner script and its strict checks, the libretro sources, the cursor path and the crosshair, the model with `GUN_RADIUS`, `GUN_LUMA`, `GUN_DELAY_TICKS` and the weights, the TH rule and the sticky bit against MAME's latch-gated bit, the latch rule and the three sources, where MAME's chord-end release and the pixel-following release agree and differ, the no-shot invariant and its exception, ordering, cost and the tolerance; and the limits.
- GUN-02 and GUN-03, and ROADMAP criteria 3 and 4, carry the exact wording of the two amendments; GUN-01 to GUN-04 and their traceability rows read Complete; the Phase 9 plan line reads 12/12 and the 12 boxes and the phase box are ticked.
- `quality.json` floors `tests.ci` 588 and `tests.wasm32` 548, equal to the non-comment lines of the two inventories.
- `.planning/seeds/SEED-010-pad-device-notes.md`: "The Light Phaser local judge", naming what a local-library run of the 13 phaser dumps can change (the delay constant of 38 ticks, the sticky bit, the one edge predicate, radius and luma, never a hash) and the unmeasured edge rule; and the review leftover that a core sync at the start of `run_frame` would remove the fuzz harness workaround and is unreachable through the API.
- Hosted dispatch 3 and the settled timeouts (below).

## Task Commits

1. Task 1, the guide and the seed hand-off: `79c4021`
2. Task 2, the amended wording, the completed requirements, the floors and the STATE row: `8ed0a52`
3. Task 3, the last timeouts: `bbc9abc`

## Local judge

- `echo "${COGMASTER_ROM_DIR:-}"` prints an empty line: no library path is set, so the library runs of the 13 dumps are not made and the hand-off in SEED-010 carries them.
- One local RetroArch observation. `cmake -DRETROARCH=/Applications/RetroArch.app/Contents/MacOS/RetroArch -DCORE=build/ci/libretro/cogmaster_libretro.dylib ... -DCONTENT=build/ci/fixtures/light_phaser.sms -P tests/retroarch/run_retroarch.cmake` exit 0 (the repository's own harness, 60 frames, a screenshot written). Then RetroArch run directly with `input_libretro_device_p1 = "4"` (the Light Gun device) and `cogmaster_port1 = "lphaser"`: exit 0, 60 frames, the log shows `SET_CONTROLLER_INFO` and `SET_INPUT_DESCRIPTORS` accepted. What this session cannot see is the pointer coordinates `RETRO_DEVICE_LIGHTGUN` returns or what TRIGGER reports off screen: no pointer can be driven and the core's polling is not logged. It decides nothing in the CI path and no fallback was needed; the cursor path stays the guaranteed one.

## Unchanged proof

- `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c`: exit 0.
- `git diff origin/main -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt` then `grep -c '^-[^-]'`: prints 0.
- `git diff --quiet origin/main -- tests/perf/allow.txt`: exit 0 (no allow line). `git diff --quiet origin/main -- .github/workflows/ci.yml`: exit 0.

## Budgets

- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, 735 counted check lines over 9225 src lines (limit 737 for the phase, 922 by the rule).
- `wc -c include/cogmaster.h` prints 9774 (under 24576). The largest docs file is `docs/testing.md` at 35785 bytes and then `docs/embedding.md` at 35521, both more than 1 KiB under the 38912 cap; `docs/controllers.md` is 17224. `cat tests/retroarch/* | wc -l` prints 261 (cap 300). `tests/fuzz` holds 68911 bytes (cap 524,288). `ctest -R '^docs\.api_names$'` passes.
- `cmake --workflow --preset ci`: exit 0. `cmake --workflow --preset hygiene`: exit 0 (50 tests). Both re-run after the timeout edits. The asan preset needs clang-21, which is not on this Mac; the hosted asan job decided (success).

## Final hosted run

All run from the repository root. Head `8ed0a5284a3587dc3596c4519ca297cfa9c1ba3b` was pushed with `git push origin phase/09-light-phaser` (exit 0) and dispatched three times.

| Command | Exit |
| --- | --- |
| `gh workflow run ci.yml --ref phase/09-light-phaser`, run found with `jq` on `headSha` equal to the head: 43015673654 | 0 |
| `gh run watch 43015673654 --exit-status` | 0 |
| `gh workflow run ci.yml --ref main` (fresh perf base, head `54631cbc5504a25816021cb08acf21c8cce9ee95` equal to `origin/main`): 43015683127; `gh run watch 43015683127 --exit-status` | 0 |
| `gh workflow run nightly.yml --ref phase/09-light-phaser`, run found by head SHA: 43015678841; `gh run watch 43015678841 --exit-status` | 0 |

- Branch ci run: https://github.com/szTheory/cogmaster/actions/runs/43015673654, conclusion success. Jobs success: hygiene 56 s, perf 69 s, asan 88 s, be32 183 s, nofp on x64 and arm64, build on linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64 and windows-arm64 (the slowest 156 s), retroarch 41 s, wasm32 72 s, hash-equality 9 s and CI required; title, rehearsal and cleanup skipped as allowed.
- Main ci run (perf base): https://github.com/szTheory/cogmaster/actions/runs/43015683127, conclusion success.
- Nightly run: https://github.com/szTheory/cogmaster/actions/runs/43015678841 at head `8ed0a52`, conclusion success; jobs vectors, bench (macos-15), bench-wasm32, fuzz, bench (ubuntu-26.04), bench (ubuntu-26.04-arm) and report all success, so the libFuzzer lane on `pad` with the gun ops is green.
- `perf control boot_to_play runs=12303209,12303209,12303209 spread=0` on the branch and `perf control boot_to_play runs=12303147,12303147,12303147 spread=0` on main.
- perf dispatch boot_to_play H=12303209 B=12303147 delta_bp=0 (shell: `(H - B) * 10000 / B`, limit 100).
- The dispatched head `8ed0a52` differs from the final head `bbc9abc` in comments and `TIMEOUT` values of `tests/host/tests.cmake`, `tests/libretro/tests.cmake` and `tests/ports/tests.cmake` only (`git diff --stat 8ed0a52 HEAD -- . ':(exclude).planning'`); the three files are registration files and the ci and hygiene workflows exit 0 on the final head.

### Measured test durations

From the branch run's log, every case passed on every lane that runs it (9 lanes for the core tests, 8 for the libretro ones; be32 was the slowest for all but a few). No test the phase added or changed took more than half its timeout; the largest ratio is `ports.gun.noshot` at 23.67 s of 69 s.

| Case | Slowest (s) | Timeout set |
| --- | --- | --- |
| `host.aim` | 0.07 (be32) | 10 s floor |
| `libretro.host.gun.aim`, `.cursor`, `.crosshair` | 0.92, 0.86, 1.72 (be32) | 10 s floor |
| `ports.gun.identity` | 2.39 (be32) | 20 s floor |
| `ports.gun.model`, `ports.gun.hostile_aim` | 8.62, 7.67 (be32) | 60 s |
| `ports.gun.noshot` | 23.67 (be32) | 69 s |
| `ports.state.gun` | 3.35 (be32) | 20 s floor |
| `fixtures.light_phaser.*` | 0.25 to 0.28 (be32) | 10 s floor |
| `runner.gun.*` | 0.12 to 0.94 (be32) | 30 s floor |
| `fuzz.regress.pad` | 2.58 (be32) | 10 s |

The runner, fixture and `ports.gun.noshot` comments were already measured in plan 07 from a larger reading and are kept (a lower reading on this run does not shrink a timeout). The job timeouts of ci.yml are untouched: no job took half its `timeout-minutes` (be32 183 s of 600 s, windows-arm64 156 s of 360 s, hygiene 56 s of 120 s).

## Deviations from Plan

### Auto-fixed Issues

None.

### Clarifications

- **The libretro gun loop was split.** The three `libretro.host.gun.*` cases shared a foreach with the older stereo and device cases at 60 s; their own loop now carries the 10 s floor and a measured comment, so the plan's acceptance count of settled comments rises by one in that file. The older cases keep the 60 s and their comment.
- **The reset rule is stated as the code does it.** Plan 03 found that a power reset keeps the aim (the planning text said it clears it); the guide says a soft or power reset keeps it and only a load parks it, as the orchestrator directed.
- **The phase box in ROADMAP is ticked** (as the plan asks) with the completion date; `phase.complete` was not run.
- **The RetroArch observation is partial.** RetroArch ran the core with the gun device on port 1; the pointer and off-screen TRIGGER values were not observable from this session.

**Total deviations:** 0 auto-fixed, 4 clarifications. **Impact:** none on behaviour or on a requirement.

## Phase notes

- The amendments land here, in the closing plan, as the Phase 7 and 8 wording amendments did.
- The device ceiling stayed SPORTS_PAD until plan 03.
- The testing-guide rows and the README line landed in plan 11; the last timeouts of the host, libretro and identity tests are settled here, to keep every plan at ten files or fewer.
- The edge rule, the delay and the sticky bit are emulator agreement and unmeasured; the local judge verdicts are not made (no library path) and are in SEED-010.
- The finding of plan 05 (a core sync at the start of `run_frame` would remove the fuzz harness workaround; unreachable through the API) is in SEED-010 as a review leftover.

## Ship-time confirmations

`/gsd-ship` (or the session after the merge) runs these from the repository root and records each command, its exit status and the run URL. Items 0 to 3 need the phase pull request; item 4 needs the merge.

0. Hash equality on the pull request head. `gh pr view phase/09-light-phaser --json headRefOid --jq .headRefOid > "$TMPDIR/head.txt"`; `gh run list --workflow ci.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId,url > "$TMPDIR/head-runs.json"` and `jq -e 'length > 0' "$TMPDIR/head-runs.json"`; `gh run watch <id> --exit-status`; `gh run view <id> --json jobs > "$TMPDIR/head-jobs.json"`; `jq -e '[.jobs[] | select(.name | test("hash-equality|retroarch"))] | length >= 2 and all(.conclusion == "success")' "$TMPDIR/head-jobs.json"` (exit 0). Append the unchanged-proof commands with their exit statuses and the hash-equality run URL to the pull request body.
1. Perf gate on the pull request. `gh run view <id> --log > "$TMPDIR/pr.log"` and `grep -Eq 'perf boot_to_play head=[0-9]+ base=[0-9]+ delta_bp=-?[0-9]+ verdict=PASS' "$TMPDIR/pr.log"` (exit 0); record the line in the pull request body.
2. AGENTS rule 10: title, auto-merge, CI on the head. `gh pr view phase/09-light-phaser --json title --jq .title > "$TMPDIR/title.txt"` and `grep -Eq '^(feat|fix|docs|ci|test|refactor|perf|build|chore)(\([a-z0-9-]+\))?!?: ' "$TMPDIR/title.txt"`; if it exits 1, `gh pr edit phase/09-light-phaser --title "feat: play Light Phaser titles"` and re-check. In a shell: `gh pr merge phase/09-light-phaser --auto --squash` (exit 0); then confirm a CI run exists on the head commit (item 0).
3. Fuzz lane on the head. `gh run list --workflow nightly.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId > "$TMPDIR/nightly-runs.json"`, `jq -e 'length > 0' "$TMPDIR/nightly-runs.json"`, `gh run watch <id> --exit-status`, `gh run view <id> --json jobs > "$TMPDIR/nightly-jobs.json"` and `jq -e '[.jobs[] | select(.name == "fuzz")] | length >= 1 and all(.conclusion == "success")' "$TMPDIR/nightly-jobs.json"` (exit 0). If the pull request has no nightly run, use the dispatched nightly run 43015678841 above when its head equals the pull request head outside `.planning`. The job is `continue-on-error`: a failure is triage, recorded with its URL.
4. Release pull request green after the merge. `gh pr list --label 'autorelease: pending' --json number,url,statusCheckRollup > "$TMPDIR/release-pr.json"` and `jq -e 'length == 1 and (.[0].statusCheckRollup | all((.conclusion // .state) | IN("SUCCESS", "SKIPPED", "NEUTRAL")))' "$TMPDIR/release-pr.json"` (exit 0); afterwards `gh run list --workflow release.yml --branch main --limit 1 --json conclusion,url` shows success.
5. Allow line, if any: none was added in this phase (`git diff --quiet origin/main -- tests/perf/allow.txt` exit 0), so nothing to delete.

## Known Stubs

None.

## Threat Flags

None. The plan edits documentation, planning files and test timeouts; no source, endpoint or file access is added.

## Next Phase Readiness

The branch is shippable: review and verification run next, then ship with the list above. The local library judge of the 13 phaser dumps waits in SEED-010 for a library path.

## Self-Check: PASSED

- docs/controllers.md, quality.json, .planning/seeds/SEED-010-pad-device-notes.md, tests/host/tests.cmake, tests/libretro/tests.cmake and tests/ports/tests.cmake exist; 79c4021, 8ed0a52 and bbc9abc are ancestors of HEAD.
