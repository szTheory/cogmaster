---
phase: 12-whole-list-in-the-ledger
plan: "10"
subsystem: ci-close
tags:
  - ci
  - timeouts
  - ledger
  - close
status: complete

requires:
  - 12-09
provides:
  - Hosted run D2 green on the complete code of plans 1 to 9
  - Every provisional timeout of the phase settled from two hosted runs, with run ids
  - The base-judge proof, the pinned re-run, the mawk proxy and the exit command repeated on the final tree
  - Floors tests.ci 669 and tests.wasm32 627
  - LEDG-01 to LEDG-05 marked complete (LEDG-05 as amended)
affects: []

actuals:
  tokens: 9000
  tasks: 2
  commits: 2

plan_head_before: 291d5f580cdfd57af3bbd45d5b6873c7753b8af7
plan_head_after: 64f76f25b7bbdd44d3e0c37e18a589b5d0caa910
commits: 2

key-decisions:
  - "A test timeout is max(floor, 2 x slowest hosted seconds) on raw seconds; compat.exit.self_test, registered at 120 s without a floor, takes a 30 s floor, the smallest limit any compat test carries"
  - "The build job limit follows the measured report (8 to 9 minutes) because D2 saw the windows-arm64 leg at 252 s; ci.yml changes in that one line only"

requirements-completed:
  - LEDG-03
  - LEDG-05

coverage:
  - id: C1
    description: "The complete code of plans 1 to 9 is green on every hosted lane in one dispatch (build legs on BWK, mawk and gawk awk, be32, wasm32, asan, nofp, retroarch, perf, hash-equality and the roll-up job), with no release rehearsal"
    requirement: LEDG-01
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/43158408243 concluded success on head 291d5f580cdfd57af3bbd45d5b6873c7753b8af7"
        status: pass
    human_judgment: false
  - id: C2
    description: "Every provisional timeout of the phase is twice a measured hosted time with its run id; no test the phase added or touched is left over half its limit"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "sh scripts/ci-durations.sh tests for runs 43153144670 and 43158408243 (exit 0 each); grep -c provisional is 0 in compat, policy and library registrations; cmake --workflow --preset ci exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The old base-branch judge, run over the final head ledger, passes and flips nothing, and the new judge passes on the same pair; no committed ledger row was changed or removed"
    requirement: LEDG-03
    verification:
      - kind: integration
        ref: "judge files from git show origin/main: over tests/compat/ledger.csv with revisions 16 and 17: exit 0 and compat: protected comparison passed; new judge exit 0; git diff -U0 origin/main -- tests/compat/ledger.csv holds no removed data line"
        status: pass
    human_judgment: false
  - id: C4
    description: "The committed table, catalog and pin counts are reproduced byte for byte by the generator over the three pinned lists, and the generated page is identical under mawk"
    requirement: LEDG-01
    verification:
      - kind: integration
        ref: "scripts/fetch-pin.sh mame-sms, mame-gamegear and mame-sg1000 then cogmaster-cartdb --counts: cmp of src/cartdb.inc, tests/compat/catalog.tsv and tests/compat/pin-counts.txt exit 0 each; docker ubuntu:26.04 docs piped to cmp with docs/compatibility.md exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The exit command over the committed ledger and the real libretro info template exits 0 with the judged-cart floors unmet and the experimental flag present; the floors in quality.json equal the registered counts minus 2"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "cmake -DLEDGER=tests/compat/ledger.csv -DINFO=libretro/cogmaster_libretro.info.in -P tests/compat/exit.cmake exit 0 (judged_cart_floors=unmet flag=present); ctest policy.quality exit 0; cmake --workflow --preset ci and hygiene exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "Counted check code and every docs file stay inside their budgets, ci.yml differs from the base branch in one timeout-minutes line only, and publish.sh and release-please-config.json are untouched"
    requirement: LEDG-04
    verification:
      - kind: integration
        ref: "cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (885 check lines, 9403 src lines); wc -c on the docs files against their caps exit 0; git diff -U0 origin/main -- .github/workflows/ci.yml holds the build timeout line only"
        status: pass
    human_judgment: false

# Phase 12 Plan 10: Close Summary

## Hosted run D2

- Run: https://github.com/szTheory/cogmaster/actions/runs/43158408243 (`gh workflow run ci.yml --ref phase/12-whole-list-in-the-ledger`, found by head SHA `291d5f580cdfd57af3bbd45d5b6873c7753b8af7`, `gh run watch 43158408243 --exit-status` exit 0). No apt DNS failure, no rerun. No other run was in progress or queued at dispatch.
- `jq -e '.conclusion == "success"' $TMPDIR/d2.json` exit 0; head SHA equals HEAD at dispatch. Jobs: hygiene, perf, be32, asan, six build legs (linux-arm64, linux-x64, macos-arm64, macos-x64, windows-arm64, windows-x64), two nofp, retroarch, wasm32, hash-equality and `CI required` all success; `title`, `rehearsal` and `cleanup` skipped (a dispatch, not a pull request; no job named rehearsal succeeded).
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

## Final proofs

All on the final tree (head `64f76f2`), from the repository root, no `cd` chains.

- **Base judge (old).** `ctest --test-dir build/ci -R '^compat\.fixtures$'` first (exit 0, results file current); then the judge files `compat-ratchet.sh` and `compat-common.sh` and the ledger taken with `git show origin/main:` into a temporary directory, revisions read from both headers (16 and 17): `sh $t/scripts/compat-ratchet.sh $t/base.csv tests/compat/ledger.csv tests/compat/allow.txt <today> 16 17 build/ci/tests/compat-fixtures/results.csv` exit 0, `compat: protected comparison passed`.
- **New judge, same pair,** absolute `COMPAT_CATALOG` and `COMPAT_REASONS`: exit 0, same line. `git diff -U0 origin/main -- tests/compat/ledger.csv | grep -c '^-"'` prints 0: the phase flips nothing.
- **Pinned re-run.** `sh scripts/fetch-pin.sh mame-sms|mame-gamegear|mame-sg1000 <dir>` exit 0 each (the pin is the one in `pins.txt`, `1d7a454ed79e`); `build/ci/cogmaster-cartdb --pin <pin> --catalog C --counts N <three lists> --fixture tests/cart/fixture_facts.txt > T` exit 0 (`1673 candidate facts; 21 compound/incomplete records excluded`); `cmp T src/cartdb.inc`, `cmp C tests/compat/catalog.tsv`, `cmp N tests/compat/pin-counts.txt` exit 0 each.
- **mawk page proxy.** `docker run --rm -v "$PWD:/src" -w /src ubuntu:26.04 sh -c 'sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md'` exit 0.
- **Exit command.** `cmake -DLEDGER=tests/compat/ledger.csv -DINFO=libretro/cogmaster_libretro.info.in -P tests/compat/exit.cmake` exit 0, printing counts only: `rows=1472 pass=9 fail=0 unknown=1434 unsupported=5 skipped=24 infrastructure_error=0`; `fixture_pass=9 pal_judged=1 fm_judged=2`; `cart_pass=0/20 cart_pal_judged=0/1 cart_fm_judged=0/1 judged_cart_floors=unmet flag=present`; `unowned_unknown=1434 cart_pal_rows=0 cart_fm_rows=50`; `exit: pass`.
- **Floors.** `quality.json` `tests.ci` 654 to 669 and `tests.wasm32` 612 to 627 (non-comment lines 671 and 629 minus 2), no `reasons` entry; the six compat floors unchanged; `ctest --test-dir build/ci -R '^policy\.quality'` exit 0.
- **Whole workflows.** `cmake --workflow --preset ci` exit 0 (666 tests plus 5 release tests, all passed) and `cmake --workflow --preset hygiene` exit 0 (84 tests), run after the last tree change.

## Budgets

- Counted check code: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, **885 check lines / 9403 src lines** (860 at the phase start; plan caps +10, +12, +12, +8 give 902 and the final figure is 885, so the reserve of 3 was not used; ceiling 905, hard limit 940). By plan: plan 02 +9 (869), plan 03 +7 (876), plan 04 +3 (879), plans 05 to 07 +0, plan 08 +6 (885), plans 09 and 10 +0.
- Docs bytes (`wc -c`; caps in parentheses): embedding 36,521 (36,700), boards 9,221 (9,700), maintaining 19,866 (19,870), conformance 35,075 (35,600), testing 33,817 (34,300), compatibility 4,072 (12,288), README 4,908 (12,288); the largest docs file is 36,521 of 38,912.
- `git diff --name-only origin/main...HEAD | grep -E 'publish\.sh|release-please-config'` finds nothing (exit 1); `ci.yml` differs from the base in the one `timeout-minutes` line of the `build` job; `src` and `include` differ from the base only by the behaviour revision line (16 to 17), the Graphic Board table row, its one table fact and the `cart.c` comment; `git diff --numstat origin/main -- tests/inventory/ci.txt tests/inventory/wasm32.txt tests/inventory/hygiene.txt` prints 14, 14 and 10 added and 0 deleted lines.
- Requirement marks: `grep -cE '^- \[x\] \*\*LEDG-0[1-5]\*\*' .planning/REQUIREMENTS.md` prints 5 (LEDG-02 was already ticked by plan 06) and the five traceability rows read Complete; line 8 and every other phase's rows are untouched. ROADMAP hunks changed: the Phase 12 checkbox line (ticked, completed 2026-11-12), the plan count line and the 12-10 box, the plan-list amendments of plan 01 already in the diff against the base, and the Phase 12 progress row (set by `roadmap update-plan-progress 12`).

## Deviations

1. **[Plan text] The self test got a 30 s floor.** `compat.exit.self_test` was registered at 120 s with no floor and a measured 4.10 s; `max(floor, 2 x S)` needs a floor, so it takes 30 s, the smallest TIMEOUT a compat test carries. All other floors are the ones registered.
2. **[Plan text] Raw-second comparison, three over-half raises.** `compat.public_update` (104), `compat.fixtures` (90, D1 be32 44.66 s against 89; D2 was 32.95 s) and `compat.csv` (45, windows-arm64 22.21 s against 30, a test this phase extended) were raised because they ran over half their limit; `compat.csv` was not on the plan's list but the over-half rule names every test the phase touched. `compat.ratchet` (150 to 142) and `library.synthetic` (180 to 160) fall to twice their measured seconds, as the rule says.
3. **[Plan text] The build job limit went 6 to 9, not 8.** Plan 05 had set 8 from run 43153144670 (214 s); D2 saw the windows-arm64 leg at 252 s, so the report calls for 9 minutes. The line is the only ci.yml difference against the base. D2 passed with the old 8-minute line and did not exercise the 9-minute one, so the single pull request run verifies it (`ci.yml` is a release input, so that run also runs the rehearsal, about 6 extra minutes); no second dispatch was made.
4. **[Plan text] The traceability baseline.** The acceptance text counts 0 ticked requirements as the baseline; LEDG-02 was already `[x]` and Complete from plan 06, so this plan changed four lines and four rows and the totals are the 5 and 5 the criterion asks for.
5. **[Planner reading] Task 1's SUMMARY started as a draft** and was completed in task 2, as the plan's output section says; the `commits` count is measured at 2 before the summary commit (a third commit carries this summary and the STATE and ROADMAP progress).
6. **[Plan text] The ROADMAP diff regex.** The acceptance regex for "no Phase 11 line touched" matches the words "PAL and Japanese FM" in two diff lines; both are Phase 12 success criterion 5 (the amended LEDG-05 wording from plan 01), and no Phase 11 line differs from the base.

No auto-fixed bug: D2 was green on the first dispatch, no apt DNS failure, no code fix.

## Known Stubs

None.

## Threat Flags

None. No new endpoint, authentication path, file access pattern or schema; the plan edits timeouts, floors and planning records.

## Hand-off carried (never a gate)

The owner's own-library run (`sh scripts/library.sh run`, then pasting the `library-judge` and `exit:` blocks; SEED-013, with the optional SEED-009/010/011 commands) follows the release. The pull request that merges its rows must also remove `is_experimental` and its wording once the exit block says the judged-cart floors are met; `compat.exit` and `policy.claims_scoped` force that in the same pull request. Its coverage entry (status `not_run`, `human_judgment: true`) is in the plan 06 summary.

## Next

The orchestrator's steps, none performed here: code review, then verification, then phase completion, then `/gsd-ship`: open the single pull request with a Conventional Commit title (`feat: list every Master System and Game Gear entry in the ledger`), run `gh pr merge --auto --squash` in a shell, confirm a CI run exists on the head commit (it verifies the build job's 9-minute line and runs the release rehearsal), and after the merge confirm the release pull request is green and lists 19 assets. Then the milestone close: audit, complete, cleanup, each its own session.

## Evidence (commands and exit statuses; no log committed)

- `gh workflow run ci.yml --ref phase/12-whole-list-in-the-ledger` exit 0; `gh run watch 43158408243 --exit-status` exit 0; `gh run view 43158408243 --json conclusion,headSha,jobs` and `jq -e '.conclusion == "success"'` exit 0.
- `sh scripts/ci-durations.sh tests 43153144670` exit 0, `sh scripts/ci-durations.sh tests 43158408243` exit 0, `sh scripts/ci-durations.sh ci.yml 40` exit 0.
- `ctest --test-dir build/hygiene -R '^hygiene\.timeout_comments' --output-on-failure` exit 0 after the build line change.
- Settled-form counts (`a` at HEAD, `b` at the base): compat 17 and 3, cart 4 and 3, policy 2 and 0, library 1 and 1; each `a` at least `b`.
- `git diff --quiet 291d5f5 HEAD -- . ':(exclude).planning' ':(exclude)tests' ':(exclude)quality.json' ':(exclude).github/workflows/ci.yml'` exit 0 (only settled timeouts, the floors and the build line differ after D2).
- No C file touched, so no gcc:13 container check was due.

## Self-Check: PASSED

- `tests/compat/tests.cmake`, `tests/cart/tests.cmake`, `tests/policy/tests.cmake`, `tests/library/tests.cmake`, `quality.json` and `.github/workflows/ci.yml` are modified and committed; commits `cbfb474` and `64f76f2` are ancestors of HEAD; run 43158408243 concluded success.
