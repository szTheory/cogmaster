---
phase: 12-whole-list-in-the-ledger
plan: "10"
status: draft
---

# Phase 12 Plan 10: Close Summary (draft, task 1)

## Hosted run D2

- Run: https://github.com/szTheory/cogmaster/actions/runs/43158408243 (`gh workflow run ci.yml --ref phase/12-whole-list-in-the-ledger`, found by head SHA `291d5f580cdfd57af3bbd45d5b6873c7753b8af7`, `gh run watch 43158408243 --exit-status` exit 0). No apt DNS failure, no rerun. No other run was in progress or queued at dispatch.
- `jq -e '.conclusion == "success"' $TMPDIR/d2.json` exit 0; head SHA equals HEAD at dispatch. Jobs: hygiene, perf, be32, asan, six build legs (linux-arm64, linux-x64, macos-arm64, macos-x64, windows-arm64, windows-x64), two nofp, retroarch, wasm32, hash-equality and `CI required` all success; `title`, `rehearsal` and `cleanup` skipped (no pull request, no release input in the diff at that head).
- Job seconds seen by `sh scripts/ci-durations.sh ci.yml 40` (exit 0): build 252 (windows-arm64), asan 151, nofp 53; the other rows are older runs.

## Measured times

`sh scripts/ci-durations.sh tests 43153144670` (D1) and `sh scripts/ci-durations.sh tests 43158408243` (D2) both exit 0. The lane of each time comes from the job logs (`gh run view <id> --log`); seconds are raw hosted seconds (ctest scales deadlines per preset: be32 by 8, wasm32 by 5, asan by 3, so a raw time is compared with the registered value). D1 held the body of every test below except `compat.exit`, `compat.exit.self_test`, `policy.claims_scoped` (+ `.self_test`) and the current `library.synthetic`, which only D2 held (N 1).

| Test | S (s) | Lane | Run | N | Old TIMEOUT | New TIMEOUT |
|---|---|---|---|---|---|---|
| cart.generator.catalog | 0.38 | windows-arm64 | D1 | 2 | 60 | 60 floor |
| compat.reasons | 0.22 | windows-arm64 | D2 | 2 | 30 | 30 floor |
| compat.hardening | 16.10 | windows-arm64 | D1 | 2 | 90 | 90 floor |
| compat.page_bound | 1.86 | macos-x64 | D2 | 2 | 60 | 60 floor |
| compat.page_bound.self_test | 2.07 | macos-x64 | D2 | 2 | 60 | 60 floor |
| compat.whole_list.count | 2.24 | windows-arm64 | D1 | 2 | 60 | 60 floor |
| compat.whole_list.fresh | 6.88 | windows-arm64 | D2 | 2 | 90 | 90 floor |
| compat.whole_list.fresh.self_test | 9.66 | windows-arm64 | D2 | 2 | 90 | 90 floor |
| compat.whole_list.reason | 4.16 | windows-arm64 | D2 | 2 | 60 | 60 floor |
| compat.whole_list.runnable | 3.09 | macos-x64 | D2 | 2 | 60 | 60 floor |
| compat.exit | 5.06 | macos-x64 | D2 | 1 | 60 | 60 floor |
| compat.exit.self_test | 4.10 | windows-arm64 | D2 | 1 | 120 | 30 floor |
| policy.claims_scoped | 1.22 | windows-arm64 | D2 | 1 | 30 | 30 floor |
| policy.claims_scoped.self_test | 11.39 | windows-arm64 | D2 | 1 | 60 | 60 floor |
| compat.ratchet | 70.79 | windows-arm64 | D2 | 2 | 150 | 142 |
| library.synthetic | 79.87 | windows-arm64 | D2 | 1 | 180 | 160 |
| compat.public_update | 51.56 | windows-arm64 | D2 | 2 | 91 | 104 (over half) |
| compat.fixtures | 44.66 | be32 | D1 | 2 | 89 | 90 (over half) |
| compat.csv | 22.21 | windows-arm64 | D1 | 2 | 30 | 45 (over half) |

Every row's comment reads `Timeout: T s [floor]; 2 x S s (lane), slowest of N runs, run <id>.` in its registration.

Per-lane slowest seconds of the tests the plan names (D1 / D2):

| Test | be32 | wasm32 | windows-arm64 |
|---|---|---|---|
| compat.fixtures | 44.66 / 32.95 | 3.94 / 4.43 | 4.68 / 4.98 |
| cart.generator | 0.76 / 0.57 | 0.20 / 0.17 | 0.48 / 0.44 |
| compat.public_update | not run | not run | 47.73 / 51.56 |
| compat.ratchet | not run | not run | 68.16 / 70.79 |
| library.synthetic | not run | not run | 51.07 / 79.87 |
| compat.hardening | not run | not run | 16.10 / 16.02 |
| compat.migration | not run | not run | 14.71 / 12.95 |

Tests over half their registered TIMEOUT (raw seconds), and what was done: `compat.public_update` (51.56 of 91 on windows-arm64) raised to 104; `compat.fixtures` (44.66 of 89 on be32, D1 only; D2 32.95) raised to 90; `compat.csv` (22.21 of 30 on windows-arm64, a test this phase extended) raised to 45; `library.synthetic` 79.87 of 160 and `compat.ratchet` 70.79 of 142 sit just under half by construction (twice the measured seconds). Existing tests this phase did not touch also exceed half of their nominal 10 s or 30 s raw: `libretro.host.fm_option`, `libretro.host.audio.psg_sweep.{sms,gg}`, `mapper.ext.{hes_4pak,nemesis,zemina}`, `cart.probe.pure`, `fm.detect`, `docs.readme_play`, `core.no_alloc_after_load`, `audio.cadence`, `fm.lockstep` and `runner.expect.audio` (57.17 s of 114 on be32). They run on be32, wasm32 or asan, where the deadline is scaled by 8, 5 or 3, so they are not near their effective limits; they are out of scope and left (scope boundary).

Registration counts of the settled form (`a` at HEAD, `b` at origin/main): tests/compat/tests.cmake 17 / 3, tests/cart/tests.cmake 4 / 3, tests/policy/tests.cmake 2 / 0, tests/library/tests.cmake 1 / 1. `grep -c provisional`: compat 0, policy 0, library 0, cart 1 (the pre-existing `cart.probe.*` comment at origin/main, untouched).

## Job timeouts

`sh scripts/ci-durations.sh ci.yml 40` exit 0 after D2. One job qualified: `build` reports 252 s (the windows-arm64 leg of D2) and 9 minutes against `timeout-minutes: 8`; the line became `timeout-minutes: 9  # 2 x 252 s, slowest of 40 runs, run 43158408243` (branch: a widened line; no job was cancelled, nothing was lowered). Every other job's minutes are at or below its line. `ctest --test-dir build/hygiene -R '^hygiene\.timeout_comments'` exit 0. `git diff -U0 origin/main -- .github/workflows/ci.yml` holds that one line only (6 to 9; plan 05 had set 8 in this same branch). `ci.yml` is a release input (`scripts/classify-changes.sh`), so the single pull request also runs the release rehearsal (about 6 extra minutes) and that run verifies the widened line; no second dispatch was made.

<!-- gsd:write-continue -->
