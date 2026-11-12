---
phase: 11-pal-and-japanese-fm
plan: "09"
subsystem: ci
tags:
  - timeouts
  - ci-durations
  - hosted-run
  - ledger
  - seeds
status: complete

requires:
  - 11-08
provides:
  - a per-test hosted time report (ci-durations.sh tests and tests-summarise) that counts only Passed ctest lines, with a hygiene control
  - the final head proved on the hosted pull request run with the migration bridge, the protected comparison, perf and hash equality shown
  - the migration repeated on the final ledger with the four bridge outcomes
  - every timeout of the phase settled as max(floor, 2 x the slowest hosted second), the be32 job at 15 minutes, floors at registered minus 2
  - SEED-012 and SEED-014, REG-02 complete, the CI speed note finished
affects: []

actuals:
  tokens: 3400
  tasks: 3
  commits: 4

plan_head_before: 0a42f8e
plan_head_after: b95e2ea19fea9fa10f312f5db6fe3ac8c26d3105
commits: 4

tech-stack:
  added: []
  patterns:
    - "A timeout is max(floor, 2 x the slowest passed hosted second of the test over the runs read), its comment names the lane, the number of runs and the slowest run, and a killed run (Timeout) is never a measurement"
    - "A closing hosted run that is red on a test timeout is read with the same report and fixed by a wider interim limit, so the next run can show the real time the settled value is derived from"

key-files:
  created:
    - .planning/seeds/SEED-012-hashless-audio-predicate.md
    - .planning/seeds/SEED-014-pal-fm-follow-ups.md
  modified:
    - scripts/ci-durations.sh
    - tests/hygiene/tests.cmake
    - tests/inventory/hygiene.txt
    - tests/runner/tests.cmake
    - tests/compat/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/library/tests.cmake
    - .github/workflows/ci.yml
    - docs/maintaining.md
    - quality.json
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - .planning/phases/11-pal-and-japanese-fm/deferred-items.md

key-decisions:
  - "Timeouts are derived from raw hosted seconds as in the previous phase, although ctest multiplies a limit by the preset's scale (8 on be32, 5 on wasm32, 3 on asan): the effective limit on those lanes is larger than the comment says"
  - "compat.public_update, compat.ratchet, compat.fixtures, runner.expect.audio and runner.hashes_file are raised because their slowest hosted time is more than half their old limit"
  - "The be32 job timeout is 15 minutes by the mechanical rule over the last 40 successful runs, with no exclusion of the 445 s run with the apt stall"
  - "The rehearsal cleanup race stays a seed: the plan does not allow a change to the release path, and gh run rerun --failed settles it"

requirements-completed:
  - REG-01
  - REG-02
  - REG-03
  - REG-04

coverage:
  - id: C1
    description: "ci-durations.sh tests-summarise counts only Passed ctest result lines (a test on two lanes gives the lane count and the slower seconds, a Failed line and a Timeout line are left out, slowest first) and tests rejects a run id that is not a number with exit 2"
    requirement: REG-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^ci\\.durations_tests$' exit 0; the same test exit 8 with the awk regex mutated to count Failed lines, exit 0 after restoring; sh scripts/ci-durations.sh tests abc exit 2"
        status: pass
    human_judgment: false
  - id: C2
    description: "The final functional head is green on the hosted pull request run: every build leg, hash equality, be32, wasm32, asan, nofp, retroarch, perf, title, rehearsal and CI required succeed, the log shows the migration line and the protected comparison, and perf passes with a delta of 0 basis points"
    requirement: REG-02
    verification:
      - kind: e2e
        ref: "gh run view 43119601937 --json conclusion exit 0 and success (attempt 2 after one failed-job rerun of the cleanup race); the log greps for the migration line, the protected comparison and the perf verdict exit 0; git diff --quiet of the run's head against HEAD outside .planning exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The migration holds on the final ledger: the five rows of the migrated version 1 ledger appear unchanged in the head ledger, which holds exactly five more rows (the audio rows); normalize reproduces the head ledger byte for byte; the bridge gives the migration line and exit 0 on the real files, non-zero for a flipped status, the old path for equal headers and a failure for a quoted or CR-damaged header"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "real-file loop exit 0, grep -c 'audio@300=' prints 5, normalize piped to cmp exit 0, five bridge simulations exit 0, 2, 0, 2, 2 as listed in the migration section, the mawk proxy in docker exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "Every provisional timeout the phase added and the pre-existing runner.hashes_file and compat ratchet comments are settled from hosted per-test times, with the lane, the number of runs and the slowest run in the comment; the be32 job timeout is re-derived by the mechanical rule"
    requirement: REG-02
    verification:
      - kind: integration
        ref: "grep counts of the settled comment form per registration file not below origin/main, provisional counts not above it; ctest --test-dir build/hygiene -R '^hygiene\\.timeout_comments' exit 0; cmake --workflow --preset ci exit 0 and cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "Floors, the CI speed note, the requirements, the roadmap and the seeds are current: tests.ci and tests.wasm32 equal the inventory counts minus 2, the CI speed section is 853 bytes, REG-01 to REG-04 are complete, SEED-012 and SEED-014 exist with no digest"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "jq floors compared with grep -vc of the inventories exit 0 twice; ctest -R '^(policy\\.quality|docs\\.)' exit 0; grep counts of the requirement and traceability rows print 4 and 4; the 40-hex grep over both seeds prints 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "Counted check code and docs sizes stay within the phase budgets and the public header and the sources are untouched"
    requirement: REG-04
    verification:
      - kind: integration
        ref: "cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 with 859 check lines against the ceiling 906; wc -c of the docs files; git diff --quiet origin/main -- include src exit 0"
        status: pass
    human_judgment: false

duration: about 100 minutes
completed: 2026-11-11
---

# Phase 11 Plan 09: Close the phase Summary

The phase head is green on the hosted pull request run with the ledger bridge, perf and hash equality shown; every timeout of the phase is twice a measured hosted time with its run, the be32 job takes 15 minutes by the repository's rule, and the deferrals are findable seeds.

## Accomplishments

- `sh scripts/ci-durations.sh tests <run-id>` and its pure half `tests-summarise` print each test with the number of lanes that passed it and its slowest seconds, slowest first. Added 30 lines to the script (class operation, 0 counted). `ci.durations_tests` feeds a four-line fixture log (a test on two lanes, one on one, a Failed and a Timeout line) and asserts the exact table; counting Failed lines in the awk turned it red.
- The hosted run of the final functional head is green (below). It also showed a red the plan had not seen: `compat.public_update` was killed at 30 s on Windows ARM64 once plan 08's adopt cases doubled its shell work (45 s there). A wider interim limit gave the real time; the settled limit is 91 s.
- The migration evidence was repeated on the final ledger and the bridge simulation gave the same outcomes as in the earlier plan.
- Fourteen comments and the limits of nine tests were settled from the hosted times of five runs; the be32 job timeout moved from 10 to 15 minutes; the floors moved to 654 and 612.
- SEED-012 (hash-less sound row) and SEED-014 (follow-ups, parser hardening table, CI items, open questions) were planted and REG-02 was marked complete.

## Commits

1. Task 1, the per-test report and its control: `e853838`
2. Fix, the interim 90 s limit for `compat.public_update` after the hosted red: `2104092`
3. Task 3, timeouts, be32, floors and the CI speed note: `56163b0`
4. Records, seeds, REG-02 and the cleanup race note: `b95e2ea`

## Hosted runs

All runs are pull request runs of the one draft pull request (#31), which stayed a draft, unedited and unmerged (`gh pr view 31 --json isDraft,state` prints true and OPEN; `gh pr list --head phase/11-pal-and-japanese-fm --state all --json number --jq length` prints 1).

| Run | Head | What it was | Result |
|---|---|---|---|
| https://github.com/szTheory/cogmaster/actions/runs/43111533403 | 1c720da | plan 07 dispatch (read for times) | success |
| https://github.com/szTheory/cogmaster/actions/runs/43112269415 | 1c720da | plan 07 pull request run (read for times) | success after a failed-job rerun of the cleanup race |
| https://github.com/szTheory/cogmaster/actions/runs/43115877197 | 0a42f8e | plans 01 to 08 head, used instead of a duplicate run | failure: build (windows-arm64), compat.public_update killed at 30.02 s; the failed-job rerun was killed at 30.01 s; rehearsal, wasm32, retroarch and hash equality were skipped |
| https://github.com/szTheory/cogmaster/actions/runs/43117518796 | 2104092 | head with the interim 90 s limit | success after a failed-job rerun of the cleanup race |
| https://github.com/szTheory/cogmaster/actions/runs/43119601937 | b95e2ea | final functional head (all code, timeouts and be32 timeout) | success after a failed-job rerun of the cleanup race |

Evidence on the final run, each command from the repository root:

- `gh run view 43119601937 --json jobs,conclusion,headSha > d2.json` exit 0 and `jq -e '.conclusion == "success"'` exit 0 (attempt 2).
- Jobs, all `success`: build (linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64, windows-arm64), hash-equality, be32, wasm32, asan, nofp (both), retroarch, perf, title, rehearsal, cleanup, changes, hygiene and CI required. `jq -e '[.jobs[] | select(.name | test("^(build|hash-equality|be32|wasm32|asan|nofp|retroarch|perf|title|rehearsal|CI required)"))] | length >= 17 and all(.conclusion == "success")'` exit 0.
- `gh run view 43119601937 --log` exit 0; `grep -q 'compat: ledger format migration'` exit 0; `grep -q 'compat: protected comparison passed'` exit 0; the perf line is `perf boot_to_play head=12288951 base=12288943 delta_bp=0 verdict=PASS`.
- `git diff --quiet <run headSha> HEAD -- . ':(exclude).planning'` exit 0 (the run's head equals HEAD outside .planning).
- The cleanup failure of the first attempt of runs 43112269415, 43117518796 and 43119601937 is the read-after-delete race in `scripts/publish.sh` (`publish: tag rehearsal-<run>-1 still exists`); `gh api repos/szTheory/cogmaster/git/matching-refs/tags/rehearsal --jq length` prints 0 each time, so nothing leaked, and `gh run rerun <id> --failed` re-ran only cleanup and the roll-up. It is not a runner DNS fault, no workflow was edited for it, and it is recorded in the phase's deferred items and SEED-014.
- The pull request run for the head of the planning commits that follow this summary changes only `.planning`, so it runs no heavy job; its URL is in the final report of the plan.

## Migration evidence on the final ledger

All from the repository root, files in the session scratch directory:

| Command | Exit | Result |
|---|---|---|
| `git show origin/main:tests/compat/ledger.csv > v1.csv` | 0 | six lines, header and five rows |
| `sh scripts/compat-common.sh normalize v1.csv > v1.v2.csv` | 0 | format 2 header and five rows |
| loop over the five rows of `v1.v2.csv`, each `grep -Fxq` in `tests/compat/ledger.csv` (whole-row equality, so id, predicate, status, evidence and date are equal) | 0 | five of five present |
| `grep -c 'audio@300=' tests/compat/ledger.csv` | 0 | prints 5; the ledger has eleven lines, header, five migrated rows and five audio rows |
| `sh scripts/compat-common.sh normalize tests/compat/ledger.csv \| cmp - tests/compat/ledger.csv` | 0 | byte equal |
| `docker run --rm -v "$PWD:/src" -w /src ubuntu:26.04 sh -c 'sh scripts/compat-common.sh docs tests/compat/ledger.csv \| cmp - docs/compatibility.md'` | 0 | the page under mawk equals the tracked page |
| wasm32 compare | not run | `build/wasm32` holds no hash file; the hosted hash-equality job, green on the final run, covers all lanes |

Bridge simulation: origin/main's judge files and ledger under a `compat-base` directory with a sha file, the head from `git archive HEAD` in a throwaway repository with the fixture results of the last ci workflow, and the step extracted from the committed `ci.yml` (`bash -n` exit 0), run through `cmake -E chdir <head> env RUNNER_TEMP=... bash -eo pipefail step.sh`:

| Case | Exit | Migration line | Last line |
|---|---|---|---|
| real files (version 1 base, version 2 head) | 0 | yes | `compat: protected comparison passed` |
| head with the first `pass` changed to `fail` | 2 | yes | `compat: invalid ledger record 27` |
| base replaced by the head's own judge files and ledger (equal headers) | 0 | no | `compat: protected comparison passed` |
| head header quoted | 2 | no | `compat: invalid ledger record 7` |
| head header with a CR appended | 2 | no | `compat: invalid ledger record 7` |

The mutations were made in copies; `git status --porcelain -- . ':(exclude).planning'` printed nothing afterwards.

## Measured times

S is the slowest passed time over five runs (43111533403, 43112269415, 43115877197, 43117518796 and 43119601937), read by `sh scripts/ci-durations.sh tests <id>` (exit 0 for each id; run 43117518796 and 43119601937 were read after their last attempt finished). The lane comes from the job name in the log prefix. The limit is the registered value after this plan. The comments in the files cite the four runs read before the final run; the final run changed two of them in the safe direction (library.synthetic 54.22 s on macos-x64, 2 x that is 108 s, under the 120 s floor; compat.fixtures 44.30 s on be32, 2 x that is 89 s, equal to the limit) and a comment-only change would have started one more full run for no new fact.

| Test | S (s) | Lane | Slowest run | Runs | Limit (s) |
|---|---|---|---|---|---|
| runner.usage.fm | 1.34 | be32 | 43119601937 | 5 | 20 |
| runner.audio_cadence | 1.37 | be32 | 43115877197 | 5 | 20 |
| runner.audio_cadence.self_test | 0.16 | windows-arm64 | 43115877197 | 5 | 20 |
| runner.expect.audio | 56.72 | be32 | 43115877197 | 5 | 114 (was 20) |
| runner.hashes_file | 53.07 | be32 | 43115877197 | 5 | 107 (was 30) |
| compat.migration | 10.21 | windows-arm64 | 43111533403 | 5 | 60 |
| compat.fixtures | 44.30 | be32 | 43119601937 | 5 | 89 (was 60) |
| compat.ratchet | 48.90 | windows-arm64 | 43111533403 | 5 | 98 (was 60) |
| compat.public_update | 45.10 | windows-arm64 | 43117518796 | 5 | 91 (was 30) |
| hygiene.tree.header_offset | 0.06 | hygiene | 43119601937 | 5 | 20 |
| ci.durations_tests | 0.02 | hygiene | 43119601937 | 2 | 20 |
| library.synthetic | 54.22 | macos-x64 | 43119601937 | 5 | 120 |
| fixtures.vdp_vtable.* (closed_form is the slowest) | 1.50 | macos-x64 | 43111533403 | 5 | 20 |
| fixtures.pause_nmi.* (mailbox.pal.240 is the slowest) | 0.69 | be32 | 43115877197 | 5 | 20 |

Tests whose slowest time exceeds half of the limit they had before this plan, and were raised: runner.expect.audio, runner.hashes_file (47.43 s on be32 against its 30 s floor in run 43111533403, 53.07 s later), compat.fixtures, compat.ratchet and compat.public_update. After the raise none is over half its limit; library.synthetic at 54.22 s is at 45 percent of 120 s and the nearest.

Settled comments per registration file, the count of lines in the settled form `a` against origin/main `b` (every file that held a provisional comment of this phase or one settled here rose): tests/runner/tests.cmake 22 against 17, tests/compat/tests.cmake 5 against 1, tests/fixtures/tests.cmake 21 against 19, tests/library/tests.cmake 1 against 0, tests/hygiene/tests.cmake 3 against 1, tests/runner/expect.cmake 0 against 0 (it holds no timeout comment). The word provisional: runner 14 against 15, compat 0 against 1, fixtures 13 against 13, library 0 against 0, hygiene 2 against 2. The remaining ones are earlier-phase comments of the form `2 x 0.01 s, slowest of 2 runs, run 39376644025, provisional` and the hygiene fixture string for `hygiene.timeout_comments` itself; none was added by this phase and each sits at its floor with a time under a tenth of a second, so they were left.

Job times, `sh scripts/ci-durations.sh ci.yml 40` (exit 0): be32 slowest 445 s, 40 runs, run 43062734904, so 15 minutes; rehearsal 357 s over 4 runs, 12 minutes against its 13; build 180 s (run 43117518796), half of its 6 minutes, the rule still gives 6; perf 140 s, hygiene 138 s (run 43082065462, from before the tree scan and analyzer speedups; the job takes 54 s now, but that run is still in the 40-run window, so the rule gives 5 minutes). The be32 job of run 43117518796 took 205 s. Only the be32 timeout and the hygiene comment text (4 runs to the 40 runs the report counts) changed in `ci.yml`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] compat.public_update timed out on Windows ARM64**
- **Found during:** Task 2 (the hosted run of plans 01 to 08)
- **Issue:** plan 08's adopt cases added about forty shell launches to the update test; on Windows ARM64 it went from 21 s to 45 s and was killed at its 30 s limit (30.02 s and 30.01 s in the two attempts of run 43115877197).
- **Fix:** an interim limit of 90 s (commit `2104092`) so run 43117518796 could show its real time, then the settled 91 s (2 x 45.10 s) in `56163b0`. The test itself is unchanged: trimming its cases would drop an individual control.
- **Files modified:** tests/compat/tests.cmake
- **Commits:** `2104092`, `56163b0`

### Interpretations (no behaviour lost)

- **One push became three.** The plan pushes once and reads that run. Run 43115877197 (plans 01 to 08) was already running and was red, so the fix needed a second push and the settlement a third; the settled head was proved on run 43119601937. The runs read for times are the five listed, not only the plan 07 dispatch and one pull request run.
- **Run 43115877197 stood in for the "this plan's push" run** until the fix; its red job gave no Passed line for the killed test, which is why the interim run was needed.
- **Raw seconds.** The settled limits use raw hosted seconds (the plan's rule and the previous phase's table), although ctest multiplies each limit by the preset's scale (be32 8, wasm32 5, asan 3). The be32 effective limit of runner.expect.audio is therefore 912 s against a 56.7 s run; a scale-aware settle would be tighter.
- **REG-01 and REG-03 were already marked.** Plans 04, 05 and 08 had ticked REG-01, REG-03 and REG-04; this plan ticked REG-02 and its traceability row, so `grep -c` of the four requirements prints 4 and 4. No shared-ID hand edit was needed.
- **The hygiene job comment** read "slowest of 4 runs" for a value the report now prints with 40 runs; the text was refreshed, the 5 minutes did not move.
- **ci.durations_tests** exists only since this plan, so its comment cites the one run that held it when the comments were written (43117518796); the final run (43119601937) measured the same 0.02 s and the limit is the floor either way.
- **No wasm32 local compare**: `build/wasm32` has no hash file, the hosted hash-equality job covers it.

### Deferred

- The rehearsal cleanup race (`rehearsal_delete` read-after-delete) failed in the first attempt of three consecutive pull request runs that ran the rehearsal. The plan does not allow a change to the release path, so it is SEED-014 and the phase's deferred items; ship will need `gh run rerun --failed` when it recurs.

## Phase notes

- The context's plan 9 text says a final counted budget of 905 and its D14 says 907; the per-plan caps sum to 906. The cap of 907 governed and the real figure is far below: 859 counted check lines (plan 08 left 859, this plan added 0; the ceiling is 906, the limit 940).
- The context says the transitions and pcm pins hold 13 lines each where 11 plus one is 12; the files were not touched here.
- Docs bytes after the phase: testing.md 31,936 (cap 34,200), runner.md 34,054 (34,300), conformance.md 32,217 (33,000), controllers.md 21,358 (21,900), audio.md 16,539 (16,900), maintaining.md 19,870 (19,967; the CI speed section is 853 bytes, cap 950), README 4,493 (12,288), compatibility.md 4,489; every docs file is under 38,912 (the largest is embedding.md at 36,379, untouched). `include/cogmaster.h` is unchanged (API 14, behaviour 16, state 16): `git diff --quiet origin/main -- include src` exit 0. `git diff --numstat origin/main -- tests/retroarch tests/fixtures/play.h` prints nothing, so `policy.e2e_budget` is at its earlier figure. No nightly dispatch: no fuzz target or seed changed.
- `ci-durations.sh tests` ran against real logs (about 700 tests per run) and agrees with a hand grep: for example runner.hashes_file 47.43 s on be32 in run 43111533403.
- The cleanup race now affects every push that runs the rehearsal; a bounded read-back retry with a stale-answer control would close it and is the first item for an owner decision.

## Known Stubs

None.

## Threat Flags

None. The plan changes a report script, test limits and records; no endpoint, auth path or trust boundary was added.

## Self-Check: PASSED

- `scripts/ci-durations.sh`, `.planning/seeds/SEED-012-hashless-audio-predicate.md`, `.planning/seeds/SEED-014-pal-fm-follow-ups.md` exist.
- Commits `e853838`, `2104092`, `56163b0` and `b95e2ea` are ancestors of HEAD (`git merge-base --is-ancestor`).
- Run 43119601937 concluded success on the head whose tree outside `.planning` equals HEAD.
