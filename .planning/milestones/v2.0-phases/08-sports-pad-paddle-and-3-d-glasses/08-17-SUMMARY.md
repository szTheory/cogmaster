---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "17"
subsystem: documentation-and-closeout
tags:
  - docs
  - controllers
  - floors
  - hosted-run
  - closeout
status: complete

requires:
  - 08-06
  - 08-09
  - 08-15
  - 08-16
provides:
  - "docs/controllers.md, the one behaviour guide for the availability matrix, resolution rule, Sports Pad and Paddle protocols with their tick constants, input fields, delta sign, the 3-D glasses rule, the libretro mapping and the limits"
  - "PAD-04 and PAD-05 and ROADMAP criteria 3 and 4 amended to the built behaviour; the Phase 8 plan list is current"
  - "quality.json floors tests.ci 548 and tests.wasm32 511 equal to the inventories"
  - "Controller test timeouts settled from hosted runs; ports.state cases raised to 80 s"
  - "Hosted dispatch 3 of ci.yml and a nightly.yml dispatch green on the final head"
affects: []

actuals:
  tokens: 14000
  tasks: 3
  commits: 4

plan_head_before: 5ff85ab71798928acc8be7a42a0e2616244204d5
plan_head_after: fadcc8bcda461467d6a7ad2e70599983fede2f91
commits: 4

tech-stack:
  added: []
  patterns:
    - "A behaviour guide kept out of the docs.api_names list, with the header names carried once in embedding.md"
    - "A test timeout comment names the measured run (2 x slowest, run id) instead of provisional"

key-files:
  created:
    - docs/controllers.md
  modified:
    - docs/testing.md
    - README.md
    - quality.json
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - tests/ports/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/runner/tests.cmake
    - tests/cart/tests.cmake
    - tests/host/tests.cmake
    - tests/compat/tests.cmake
    - tests/libretro/tests.cmake

key-decisions:
  - "The controllers guide states each constant by name with its value once (SPORTS_IDLE_TICKS 1,536 ticks, PADDLE_HALF_TICKS 299 ticks) and cites the sweep rows of the Paddle fixture"
  - "The workflow job timeouts stay: dispatch 3 measured be32 at 166 to 196 s against 600 s and asan at 58 to 72 s against 300 s, so the new tests did not make them tight"
  - "The local library judge could not run in this session (see Local judge); the default of cogmaster_3d_view stays left and the measurement is carried to ship"

requirements-completed:
  - PAD-01
  - PAD-04
  - PAD-05

coverage:
  - id: C1
    description: "docs/controllers.md carries the controller behaviour once (matrix, resolution rule, protocols and constants, input fields, delta sign, glasses rule, adapter stereo rule, soft-reset pending axes, host mapping and option keys, the limit that timings are unverified on hardware), stays out of the docs.api_names list, uses no planning identifiers and is well under its size cap"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0 (35 of 35); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; wc -c docs/controllers.md prints 9521"
      status: pass
    human_judgment: false
  - id: C2
    description: "PAD-04 and PAD-05 and ROADMAP criteria 3 and 4 carry the amended wording (the eye is reported on every Master System profile whatever $3E says; a caller built against the 32-byte input of API versions 1 to 11), the superseded wording is gone and the Phase 8 plan list names 17 plans"
    requirement: PAD-05
    verification:
      kind: other
      ref: "the plan's grep chain over REQUIREMENTS.md and ROADMAP.md exit 0; git diff --stat origin/main -- .planning/ROADMAP.md shows only the Phase 8 entry among the Phase 8 lines"
      status: pass
    human_judgment: false
  - id: C3
    description: "The floors tests.ci and tests.wasm32 equal the non-comment line counts of the two inventories and policy.quality passes"
    requirement: PAD-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^policy\\.quality$' exit 0 with floors 548 and 511"
      status: pass
    human_judgment: false
  - id: C4
    description: "The one-shot unchanged-proof holds: tests/io, tests/testcard_pins.cmake and tests/audio have no diff against origin/main and no existing line of the three data files is removed"
    requirement: PAD-01
    verification:
      kind: other
      ref: "git diff --quiet origin/main -- tests/io tests/testcard_pins.cmake tests/audio exit 0; grep -c '^-[^-]' over the data diff prints 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "The whole head passes the ci and hygiene workflows locally and on a hosted dispatch of the final head, the nightly libFuzzer lane passes on the same head, and the perf gate control shows no movement"
    requirement: PAD-05
    verification:
      kind: e2e
      ref: "https://github.com/szTheory/cogmaster/actions/runs/42978923715 (ci.yml, success) and https://github.com/szTheory/cogmaster/actions/runs/42978927408 (nightly.yml, success), both on fadcc8bcda461467d6a7ad2e70599983fede2f91"
      status: pass
    human_judgment: false
  - id: C6
    description: "The agent's local judge runs over the owner's library (delta versus all-zero hashes for the seven device dumps, --eyes for the nine 3-D titles, and the false-positive count over the whole library)"
    requirement: PAD-04
    verification:
      kind: other
      ref: "test -n ${COGMASTER_ROM_DIR-} exit 1 in this session; the shell guard refused any read of .env.local; no dump was run"
      status: not_run
    human_judgment: true

duration: 95min
completed: 2026-11-11
---

# Phase 8 Plan 17: The controller guide and the closing evidence Summary

**`docs/controllers.md` now holds every controller rule of the phase once, the PAD-04 and PAD-05 wording and the roadmap criteria match what was built, the test-count floors are 548 and 511, the controller test timeouts name measured runs, and the final head is green on a hosted ci dispatch and a nightly dispatch with the perf gate control unmoved; the local library judge could not run in this session and is carried to ship.**

## What was built

- Task 1: `docs/controllers.md` (9,521 bytes): availability matrix, the resolution rule and the region trap, input fields and the per-call delta rule, the Sports Pad read sequence with `SPORTS_IDLE_TICKS` and the unmodelled 60 microsecond resync, the Paddle with `PADDLE_HALF_TICKS`, its derivation, the 199 and 398 near-lock lengths and the sweep rows, the glasses rule with the cut at active line 96 as the core's own choice and the SMS2 modelling choice, the adapter's stereo rule and the five views, soft-reset pending axes, host mapping and option keys, and the limits line. It is not in the `docs.api_names` list. `docs/testing.md` gained the `ports` label row and a table of controller test families; `README.md` one link line; `quality.json` floors 548 and 511.
- Task 2: PAD-04 and PAD-05 in `REQUIREMENTS.md` and criteria 3 and 4 in `ROADMAP.md` carry the wording of CONTEXT decisions 1 and 5; the Phase 8 plan list already named the 17 files and ticked 08-01 to 08-16; the plan count line was set to 17 plans; one Runtime Log row in `STATE.md`.
- Task 3: the unchanged-proof, the local workflows, budgets, the timeout settlement, and hosted dispatch 3 with the nightly dispatch.

## Local judge

The plan's local judge reads the owner's library of dumps. In this session `COGMASTER_ROM_DIR` is not in the environment (`test -n "${COGMASTER_ROM_DIR-}"` exit 1) and the only other source of its location is `.env.local`, which the shell's secret-read guard refused to let the agent read (the PreToolUse hook rejected the command). Reading it by another route was not attempted: that would be routing around the guard. So none of these ran: the delta versus all-zero frame hash pairs and end images for the three Sports Pad and four Paddle dumps, `--eyes` and `--png` for the nine 3-D titles, and the false-positive count over the library. Consequences, stated rather than assumed:

- The default of `cogmaster_3d_view` stays `left` in `libretro/options.h` (no measurement forced a change), and `docs/controllers.md` says `left`.
- The 60 microsecond resync stays unmodelled (no judge showed a desync) and no sign was refuted.
- What did run, as a smoke of the same commands on the owned images: `build/ci/cogmaster-run --frames 600 --eyes build/ci/fixtures/glasses.sms` exit 0 printed `eyes frames=600 none=2 left=584 right=14 flips=15 last=left`; `--frames 120 --input <75-byte delta script> --hash build/ci/fixtures/sports_pad.sms` and the all-zero script of equal length both exit 0 with the same hash, because that image reports through the mailbox and draws nothing (its motion is judged by `fixtures.sports_pad.mailbox.*` and `fixtures.sports_pad.replay`). The Paddle smoke was a quoting mistake of the agent (exit 2) and was not repeated; `fixtures.paddle.*` cover it.
- To close it: set `COGMASTER_ROM_DIR` for the agent (or run the commands on the owner's machine) and run, per dump, `cogmaster-run <dump> --frames N --input <delta script> --hash` against an all-zero script, then `--frames 600 --eyes` over the library, and change the one default line only if a title that is not a 3-D title flips.

## Unchanged proof

- `git diff --quiet origin/main -- tests/io tests/testcard_pins.cmake tests/audio` exit 0.
- `git diff origin/main -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt` exit 0 and `grep -c '^-[^-]'` over it prints 0 (three files, 64 insertions, no removed line).

## Budgets

- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: 723 counted check lines against 8793 `src` lines (at most 735).
- `wc -c include/cogmaster.h` 9708 (under 24576). Docs margins to the 38,912 cap: embedding 4,288 and testing 4,753 are the smallest, all above 2 KiB; `controllers.md` 9,521 bytes. `cat tests/retroarch/* | wc -l` prints 261 (the `policy.e2e_budget` count with its shared oracle is 275, at most 300). `ctest -R '^(docs\.api_names|policy\.e2e_budget|policy\.quality)$'` exit 0.
- `cmake --workflow --preset ci` exit 0 (544 tests, plus 4 release) and `cmake --workflow --preset hygiene` exit 0 (50 tests). The `asan` preset needs clang-21, which is not installed here; the hosted asan job passed (58 s and 72 s).

## Timeouts

The repository check `hygiene.timeout_comments` reads only the workflow `timeout-minutes` comments, so the test `TIMEOUT` comments of earlier phases keep their `provisional` wording; the new controller tests were settled anyway from run 42973268415 and run 42978311844 (per-test seconds read from the ctest lines of the be32, wasm32, asan, Windows, Linux and macOS job logs):

- `ports.state.*` rose from 60 to 80 s: `ports.state.mutation` took 38.44 s in run 42973268415 and 32.01 s in run 42978311844 on the be32 lane (4.7 s native). The other `ports.*`, `fixtures.sports_pad|paddle|glasses.*`, `runner.device.*`, `runner.input.axes_*`, `cart.*`, `host.eyes`, `compat.device_*` and `libretro.host.devices|stereo.*` comments name `2 x slowest, slowest of 1 run, run <id>` with floors that already hold (the slowest in each is `libretro.host.devices.sources` at 15.76 s of 60 and `runner.device.default` at 13.41 s of 30).
- Workflow jobs: `scripts/ci-durations.sh ci.yml` lists be32 at 409 s as its slowest over the last 40 successful runs (run 42789733748, before this phase) against `timeout-minutes: 10`, whose comment still cites 275 s from run 42741490263; dispatch 3 measured be32 at 166 and 196 s and asan at 58 and 72 s, hygiene at 45 and 58 s against 2 minutes. The phase's tests did not make any job tight, so no workflow line was changed; the 409 s be32 run is a pre-existing drift and is listed under the ship-time notes.

## Final hosted run

- Branch ci.yml dispatch (head `fadcc8bcda461467d6a7ad2e70599983fede2f91`, which equals `HEAD` outside `.planning`): https://github.com/szTheory/cogmaster/actions/runs/42978923715, conclusion success. Jobs: hygiene, nofp (ubuntu-26.04), nofp (ubuntu-26.04-arm), asan, be32, wasm32, hash-equality, perf, retroarch, build on linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64 and windows-arm64, changes and CI required all success; title, rehearsal and cleanup skipped as for any dispatch. `jq -e` over the jobs matching `hash-equality|asan|nofp|retroarch` exit 0 (5 jobs, all success).
- An earlier dispatch on `bd1d6c4`, https://github.com/szTheory/cogmaster/actions/runs/42978311844 (success), measured the per-test times above; the head moved only by test comment edits afterwards, so the run on `fadcc8b` above is the final one.
- Nightly nightly.yml dispatch on the same head: https://github.com/szTheory/cogmaster/actions/runs/42978927408, conclusion success; jobs vectors, bench x3, bench-wasm32, fuzz and report all success; the `fuzz` job passed `fuzz.libfuzzer.pad` (61.22 s).
- Perf control lines: branch `perf control boot_to_play runs=12303147,12303147,12303147 spread=0`; main, run https://github.com/szTheory/cogmaster/actions/runs/42963279510 on `bb7d15874cb357a366521e72046ad279441decb1`, which `git rev-parse origin/main` still prints: `perf control boot_to_play runs=12302854,12302854,12302854 spread=0`.
- `perf dispatch boot_to_play H=12303147 B=12302854 delta_bp=0` (shell integer arithmetic; the exact ratio is 0.02 hundredths of a percent). `git diff --quiet origin/main -- tests/perf/allow.txt` exit 0, so no allow line.

## Ship-time confirmations

`/gsd-ship` (or the session after the merge) runs these from the repository root and records each command, its exit status and the run URL. Items 0 to 3 need the phase pull request; item 4 needs the merge.

0. **Hash equality on the pull request head.** `gh pr view phase/08-sports-pad-paddle-and-3-d-glasses --json headRefOid --jq .headRefOid > "$TMPDIR/head.txt"`; `gh run list --workflow ci.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId,url > "$TMPDIR/head-runs.json"` and `jq -e 'length > 0' "$TMPDIR/head-runs.json"`; `gh run watch <id> --exit-status`; `gh run view <id> --json jobs > "$TMPDIR/head-jobs.json"`; `jq -e '[.jobs[] | select(.name | test("hash-equality|retroarch"))] | length >= 2 and all(.conclusion == "success")' "$TMPDIR/head-jobs.json"` (exit 0). Append the unchanged-proof commands with their exit statuses and the hash-equality run URL to the pull request body.
1. **Perf gate on the pull request.** `gh run view <id> --log > "$TMPDIR/pr.log"` and `grep -Eq 'perf boot_to_play head=[0-9]+ base=[0-9]+ delta_bp=-?[0-9]+ verdict=PASS' "$TMPDIR/pr.log"` (exit 0); record the line in the pull request body.
2. **AGENTS rule 10: title, auto-merge, CI on the head.** `gh pr view phase/08-sports-pad-paddle-and-3-d-glasses --json title --jq .title > "$TMPDIR/title.txt"` and `grep -Eq '^(feat|fix|docs|ci|test|refactor|perf|build|chore)(\([a-z0-9-]+\))?!?: ' "$TMPDIR/title.txt"`; if it exits 1, `gh pr edit phase/08-sports-pad-paddle-and-3-d-glasses --title "feat: play Sports Pad, Paddle and 3-D glasses titles"` and re-check. In a shell, never inside GSD command text: `gh pr merge phase/08-sports-pad-paddle-and-3-d-glasses --auto --squash` (exit 0).
3. **Fuzz lane on the head.** `gh run list --workflow nightly.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId > "$TMPDIR/nightly-runs.json"`, `jq -e 'length > 0' "$TMPDIR/nightly-runs.json"` (exit 0), `gh run watch <id> --exit-status`, `gh run view <id> --json jobs > "$TMPDIR/nightly-jobs.json"` and `jq -e '[.jobs[] | select(.name == "fuzz")] | length >= 1 and all(.conclusion == "success")' "$TMPDIR/nightly-jobs.json"` (exit 0). If the pull request has no nightly run, use run 42978927408 above when its head `fadcc8bcda461467d6a7ad2e70599983fede2f91` equals the pull request head outside `.planning`. The job is `continue-on-error`: a failure is triage, recorded with its URL.
4. **Release pull request green after the merge.** `gh pr list --label 'autorelease: pending' --json number,url,statusCheckRollup > "$TMPDIR/release-pr.json"` and `jq -e 'length == 1 and (.[0].statusCheckRollup | all((.conclusion // .state) | IN("SUCCESS", "SKIPPED", "NEUTRAL")))' "$TMPDIR/release-pr.json"` (exit 0); afterwards `gh run list --workflow release.yml --branch main --limit 1 --json conclusion,url` shows success.
5. **Allow line, if any.** None was added in this phase (`git diff --quiet origin/main -- tests/perf/allow.txt` exit 0), so there is nothing to delete.
6. **The local library judge** of this summary's Local judge section, once `COGMASTER_ROM_DIR` is available to the agent or run by the owner on their machine; it can change one default line and one doc sentence at most, and never a hash.
7. **be32 job timeout.** `scripts/ci-durations.sh ci.yml` reports a 409 s be32 run (42789733748) against `timeout-minutes: 10`; consider re-deriving that line with the repository's twice-the-slowest rule in a later pipeline change. It is not caused by this phase.

## Task Commits

1. `2e16df7` docs(08-17): controller guide, test family rows, README link and raised floors
2. `e7c6c2d` docs(08-17): amend the glasses and input-struct wording and record the step
3. `bd1d6c4` test(08-17): settle the controller test timeouts from the hosted run
4. `fadcc8b` test(08-17): name the measured run in the last controller test timeouts

## Deviations from Plan

**1. [Rule 3 - Blocker] The local library judge could not run**
- **Found during:** Task 3
- **Issue:** the library's location is only in `.env.local`, which the secret-read guard refused; `COGMASTER_ROM_DIR` is unset in the agent's environment.
- **Fix:** recorded the command and exit status, ran the same commands on the owned fixtures as a smoke, left the one-line default unchanged and listed the judge as ship-time item 6.
- **Files modified:** none.

**2. [Rule 1 - Mechanism] The ROADMAP plan count line**
- **Found during:** Task 2
- **Issue:** the plan's acceptance wants `**Plans**: 17 plans`, while `roadmap update-plan-progress` rewrites that line to the progress form of earlier phases once this SUMMARY exists.
- **Fix:** the line was set to `17 plans` in Task 2 and the progress tool's form (`17/17 plans executed`) is what the tool left after the SUMMARY; the orchestrator's phase completion changes it.
- **Files modified:** `.planning/ROADMAP.md`.

**3. Test timeouts settled in two steps.** The timeouts of plans 08-13 to 08-16 had no hosted measurement until dispatch 3, so the comments were settled in two commits and the final dispatch ran on the second. Only comments and the `ports.state` limit changed, outside `.planning`.

**Total deviations:** 3 (one blocker recorded, one tool-form difference, one ordering).

## Known Stubs

None.

## Threat Flags

None. The plan changes documents, floors and test comments, and one test timeout.

## Phase notes

- The PAD-04 and PAD-05 wording amendments land here, in the closing plan, as the Phase 7 wording amendment did.
- The highest accepted device stayed NONE until plan 08-05 introduced the one shared constant.
- The delta-to-hash replay differential landed with the first device fixture (plan 08-10).
- The ares resync of 60 microseconds stays unmodelled: no local judge ran, so no desync was shown.
- The 16-title judge verdicts: not run in this session (see Local judge); no seed was raised.

## Next Phase Readiness

All 17 plans have a SUMMARY. The orchestrator runs review and verification, then phase completion; no `phase.complete` was run here. The branch is pushed through `fadcc8b`; the `.planning` records of this plan are committed after it.

## Self-Check: PASSED

- `docs/controllers.md` exists; commits `2e16df7`, `e7c6c2d`, `bd1d6c4` and `fadcc8b` are ancestors of HEAD; `git rev-list --count 5ff85ab71798928acc8be7a42a0e2616244204d5..HEAD` printed 4 before this SUMMARY was committed.
- `gh run view 42978923715 --json conclusion --jq .conclusion` prints success and its `headSha` equals `fadcc8bcda461467d6a7ad2e70599983fede2f91`.
