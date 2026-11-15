# Phase 17 run evidence

Counts, commands, exit statuses and run URLs only. No library path, title or file name appears here.

## Pushed head

Head `1b8d4466ab72b4ca75e5c026270540a3f70165cc` on `phase/17-re-run-merge-and-clean-state` (code of plans 01 and 02 plus one fix to plan 02's script). Phase base `0a97075f34e48e2b1d0f72aa231006cc9366a398`.

| step | command | exit status |
|------|---------|-------------|
| format | clang-format 21 `--dry-run --Werror` over `git ls-files -z '*.c' '*.h' ':!:libretro/libretro.h'` | 0 |
| ci preset | `cmake --workflow --preset ci` | 0 |
| hygiene preset | `cmake --workflow --preset hygiene` | 0 |
| tree check | `sh scripts/hygiene.sh --tree` | 0 |
| push | `git push -u origin phase/17-re-run-merge-and-clean-state` | 0 |
| first dispatched run | `gh workflow run ci.yml --ref phase/17-re-run-merge-and-clean-state`, head e896d84, https://github.com/szTheory/cogmaster/actions/runs/43622165959 | failed: `policy.milestone_close` cases baseline, release_tag and pending on both Windows legs |
| fix | commit 1b8d446 passes `MSYS_NO_PATHCONV=1` and `MSYS2_ARG_CONV_EXCL='*'` to the one `git show origin/main:<path>` call in `scripts/milestone-close.sh`; `ctest --preset ci -R '^policy\.milestone_close$'` | 0 |
| second dispatched run | `gh run watch --exit-status`, head 1b8d446, https://github.com/szTheory/cogmaster/actions/runs/43657530824 | 0 (conclusion success, no failed job) |
| stamp control | `git merge-base --is-ancestor 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4 origin/main` (Phase 15 pre-squash stamp) | 1 (not an ancestor, as the control requires) |
| base is ancestor | `git merge-base --is-ancestor 0a97075f34e48e2b1d0f72aa231006cc9366a398 origin/main` | 0 |
| sources equal | `git diff --quiet 0a97075 HEAD -- src include CMakeLists.txt cmake tests/compat/ledger.csv docs/compatibility.md tests/compat/allow.txt quality.json` | 0 |
| worktree head | `git -C <worktree> rev-parse HEAD` equals the base | 0 |
| worktree clean | `git -C <worktree> status --porcelain` empty | 0 |
| no pull request | `gh pr list --head phase/17-re-run-merge-and-clean-state --state open` returns 0 | 0 |

The Windows failure was a fault in plan 02's script, not in the diagnose prune: the manifest read failed under the Windows shell's path rewriting, so the three cases that read the release manifest answered `unknown` (exit 2).

Code head for the owner session: `d2b8605cb654069bd6cbe33ff84d852c900549da`, one commit after the fix above.

| step | command | exit status |
|------|---------|-------------|
| dotenv fix | commit d2b8605 `fix(library): read the main checkout's dotenv from a linked worktree`: `scripts/library.sh` falls back to the common git directory's parent when the working tree is a linked worktree, and `tests/library/tests.cmake` gains a linked-worktree case; the case failed without the script change and passed with it | 0 |
| hygiene after fix | `sh scripts/hygiene.sh --tree` | 0 |
| third dispatched run | `gh workflow run ci.yml --ref phase/17-re-run-merge-and-clean-state`, head d2b8605, https://github.com/szTheory/cogmaster/actions/runs/43660745598 | 0 (conclusion success) |

Why that fix: the detached worktree of the phase base has no local settings file, and the library script reads the library setting from the root of the tree it runs in. The owner authorised the agent to run the library commands, so a linked worktree now reads the main checkout's local settings through the common git directory. The diagnose half runs from the main checkout and needed no change.

## Library run

Judged commit (the full stamp): `0a97075f34e48e2b1d0f72aa231006cc9366a398`, the phase base, an ancestor of `origin/main`. UTC date of the run: 2026-11-15. Behaviour revision: 19.

The judge ran once, in a detached worktree of the phase base, with `COGMASTER_LIBRARY_OUT` unset and the library script of the branch head invoked by absolute path from that worktree (so the dotenv fallback of d2b8605 applied): `sh scripts/library.sh` with no argument, which configures, builds the runner at the base commit and runs the library test. The owner authorised the agent to run it, because only the owner's machine holds the library (rule 6); the owner judged nothing. Run length about 11 minutes. The agent opened only the worktree's `build/library/results/summary.txt`.

| count | value | printed by | read from | exit status |
|-------|-------|------------|-----------|-------------|
| pal carts | 0 | sh scripts/library.sh | summary.txt | 0 |
| pal ran | 1 | sh scripts/library.sh | summary.txt | 0 |
| pal boots pass | 1 | sh scripts/library.sh | summary.txt | 0 |
| pal boots fail | 0 | sh scripts/library.sh | summary.txt | 0 |
| fm | skipped: not configured | sh scripts/library.sh | summary.txt | 0 |
| rows added | 0 | sh scripts/library.sh | summary.txt | 0 |
| ratchet | ok | sh scripts/library.sh | summary.txt | 0 |
| page bytes | 10261 | sh scripts/library.sh | summary.txt | 0 |
| rows | 1986 | sh scripts/library.sh | summary.txt | 0 |
| pass | 1116 | sh scripts/library.sh | summary.txt | 0 |
| fail | 9 | sh scripts/library.sh | summary.txt | 0 |
| unknown | 832 | sh scripts/library.sh | summary.txt | 0 |
| unsupported | 5 | sh scripts/library.sh | summary.txt | 0 |
| skipped | 24 | sh scripts/library.sh | summary.txt | 0 |
| infrastructure_error | 0 | sh scripts/library.sh | summary.txt | 0 |
| fixture pass | 9 | sh scripts/library.sh | summary.txt | 0 |
| pal judged | 1 | sh scripts/library.sh | summary.txt | 0 |
| fm judged | 2 | sh scripts/library.sh | summary.txt | 0 |
| cart pass | 598/20 | sh scripts/library.sh | summary.txt | 0 |
| cart pal judged | 511/1 | sh scripts/library.sh | summary.txt | 0 |
| cart fm judged | 51/1 | sh scripts/library.sh | summary.txt | 0 |
| judged cart floors | met | sh scripts/library.sh | summary.txt | 0 |
| experimental flag | absent | sh scripts/library.sh | summary.txt | 0 |
| unowned unknown | 832 | sh scripts/library.sh | summary.txt | 0 |
| cart pal rows | 511 | sh scripts/library.sh | summary.txt | 0 |
| cart fm rows | 51 | sh scripts/library.sh | summary.txt | 0 |
| exit verdict | pass | sh scripts/library.sh | summary.txt | 0 |
| library candidates, attempted, evaluated, predicates and per-status counts | not reported | not reported | not reported | not reported |

Status transitions of the carried ledger against the `origin/main` ledger (from the transition count in Merge checks):

| machine | tv | fm | from | to | rows |
|---------|----|----|------|----|------|
| sms | pal | off | fail | pass | 42 |
| sms | ntsc | off | fail | pass | 43 |
| sms | ntsc | on | fail | pass | 2 |

Sanity reading, never a gate: the ledger counts pass=1116 fail=9 (ntsc 5, pal 4), against pass=1029 fail=96 on the `origin/main` ledger and page, so the 87 flips from fail to pass account for the whole change in both totals. Every judged row was restamped with the judged commit (1116 ledger lines changed: 1107 pass and 9 fail).

## Merge checks

Run from the repository root of the main checkout after the carry; scratch files in the session scratch directory outside the tree.

| check | command | exit status |
|-------|---------|-------------|
| judge accepted | `grep -Eq '^merge: rows_added=-?[0-9]+ ratchet=ok page_bytes=[0-9]+$'` on the worktree's summary.txt | 0 |
| two-file diff | `git -C <worktree> diff --quiet` outside the two files, and `! git -C <worktree> diff --quiet` for each of `tests/compat/ledger.csv` and `docs/compatibility.md` | 0 |
| carry | `git -C <worktree> diff -- tests/compat/ledger.csv docs/compatibility.md > <scratch>`, `test -s <scratch>`, `git apply <scratch>` | 0 |
| carried bytes equal | `cmp tests/compat/ledger.csv <worktree>/tests/compat/ledger.csv` and `cmp docs/compatibility.md <worktree>/docs/compatibility.md` | 0 |
| one stamp equal to the base | the 40-hex `core=` values of the added ledger lines, `sort -u`, counted equal to 1 and equal to `git merge-base HEAD origin/main` | 0 |
| ancestry (positive) | `git merge-base --is-ancestor 0a97075f34e48e2b1d0f72aa231006cc9366a398 origin/main` | 0 |
| ancestry (control, the Phase 15 pre-squash stamp) | `git merge-base --is-ancestor 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4 origin/main` | 1 (not an ancestor, as the control requires) |
| reachability | `gh api repos/szTheory/cogmaster/commits/0a97075f34e48e2b1d0f72aa231006cc9366a398 --jq .sha` | 0 |
| transition count | the Phase 15 awk over the saved `origin/main` ledger and `tests/compat/ledger.csv`, no pipe after it (pass rows lost: 0) | 0 |
| page agreement | `sh scripts/compat-common.sh docs tests/compat/ledger.csv \| cmp - docs/compatibility.md` | 0 |
| compat tests | `cmake --preset ci`, `cmake --build --preset ci`, then `ctest --test-dir build/ci -R '^compat\.(exit\|whole_list\.fresh\|page_bound)$' --output-on-failure --no-tests=error` (3 of 3 passed) | 0 |
| old base judge | `sh <base copy>/scripts/compat-ratchet.sh <base ledger> tests/compat/ledger.csv tests/compat/allow.txt <date> 19 19 build/ci/tests/compat-fixtures/results.csv`, base copies from `git show origin/main:`, behaviour revisions read and equal (19 and 19), printed `compat: protected comparison passed` | 0 |
| fence | `git diff --quiet origin/main -- tests/compat/allow.txt quality.json src include` | 0 |

## Diagnose counts

Command: `unset COGMASTER_LIBRARY_OUT` then `sh scripts/library.sh diagnose`, run once from the repository root of the main checkout on the ledger commit (clean tree outside the planning directory), standard output to a scratch file; exit status 0; run length between 20 and 25 minutes. It stamps nothing and wrote no tracked file. The agent read only the header, the leaf group lines that have no sub-row, and the totals line.

Header: core `7da9ccc`, behaviour 19. `git rev-parse --short=7 HEAD` at the time of the run was `7da9ccc`, equal, and the header carries no dirty mark, so the runner was rebuilt at the head with the new line shape.

| leaf | rows |
|------|------|
| dark_no_fault.never_draws | 2 |
| static_no_fault.frozen | 2 |
| fault_alive.unsupported_video_mode.vdp | 1 |
| input_mismatch | 0 |

| total | value |
|-------|-------|
| judged | 573 |
| run_fail | 5 |
| ledger_fail | 5 |
| matched | 5 |
| ledger_fail_not_in_run | 0 |
| ledger_fail_now_pass | 0 |
| run_fail_not_in_ledger | 0 |
| groups_total | 5 |
| fail_total | 5 |

The groups add up to the fail total (2 + 2 + 1 + 0 = 5), so every failing row has a cause group. Diagnose covers the first NTSC profile only: its 5 failing rows are the 5 `tv=ntsc` fail rows of the carried ledger, matched one to one, and the other 4 fail rows of the ledger are the PAL rows. `run_fail_not_in_ledger` is 0, so no further question arises for SEED-016.

Input counts from the diagnose candidate line, for the record: candidates 696, attempted 573, evaluated 573, malformed 0, malformed_io 0, uncatalogued 82, skipped_excluded 30, noprofile 0, unsupported 11, infrastructure_error 0, title_withheld 0.

## Local gates

| gate | command | exit status |
|------|---------|-------------|
| worktree removed | `git worktree remove --force <worktree>`, then `git worktree prune`; the worktree is no longer listed | 0, 0 |
| ci workflow on the ledger head | `cmake --workflow --preset ci` (738 of 738 tests, then 5 of 5) | 0 |
| fence | `git diff --quiet origin/main...HEAD -- src include tests/compat/allow.txt quality.json` | 0 |
| no pull request | `gh pr list --head phase/17-re-run-merge-and-clean-state --state open` returns 0 | 0 |

Nothing was pushed after the ledger commit; plan 04 pushes this head before measuring.

## Timeouts

Sample: the ledger head `2cc8bb55027a116293a1707a583def093f416b90` was pushed once (`git push origin phase/17-re-run-merge-and-clean-state`, exit 0, hooks ran, no bypass) and `gh workflow run ci.yml --ref phase/17-re-run-merge-and-clean-state` was dispatched on it: https://github.com/szTheory/cogmaster/actions/runs/43664171483, conclusion success, no failed job. The gate before any timeout file was edited exited 0: `h=$(git rev-parse HEAD)`, the remote branch head equal to `h` by `git ls-remote`, and `gh run list --workflow ci.yml --branch phase/17-re-run-merge-and-clean-state --event workflow_dispatch --limit 20 --json headSha,conclusion` with one success on `h`. The code of that run equals the shipped code under the old limits, so the pull request's own run is never a sample of its own limits. Nothing was committed between that push and the workflow commit, so the workflow commit's parent is the sampled head.

Measurement (each command from the repository root, output to the scratch directory outside the tree):

| step | command | exit status |
|------|---------|-------------|
| job seconds | `sh scripts/ci-durations.sh ci.yml 40` (14 jobs; the runs per job are in the table below, 2 to 40 because some jobs are newer or conditional) | 0 |
| window run IDs | `gh api "repos/szTheory/cogmaster/actions/workflows/ci.yml/runs?status=success&per_page=40" --jq '.workflow_runs[].id'` (40 IDs, the sample run is the newest) | 0 |
| test seconds | `sh scripts/ci-durations.sh tests "$id"` for each of the 40 IDs | 0 for 40 of 40, none left out |
| slowest of each test | one awk over the 40 per-run files for `library.diagnose`, `hygiene.tree` and `runner.diagnose`: S, run ID and N, where N counts runs listing the test | 0 |
| lanes of the slowest runs | `gh run view <id> --log` for 43460663238, 43562888383 and 43514870188, the lane read from the job name in the line prefix | 0, 0, 0 |
| lane-aware read | `gh run view "$id" --log` for the 40 IDs and one awk keeping the job name, test and seconds of every passing line of the three tests (424 lines) | 0 for 40 of 40 |

CTest scales every `TIMEOUT` by the lane (asan 3, be32 8, wasm32 5, from `CMakePresets.json`), so a limit is compared with unscaled lane seconds. The lane-aware read exists for that: the slowest `runner.diagnose` line overall is the be32 lane at 450.48 s, which the be32 scale of 8 turns into a 5064 s limit, so it is not the figure to double. Per lane, slowest `runner.diagnose` in the window (the scaled lanes shown raw, then divided by their scale):

| lane | slowest s | divided by scale | run id |
|------|-----------|------------------|--------|
| macos-x64 | 179.23 | 179.23 | 43460663238 |
| be32 | 450.48 | 56.31 | 43514870188 |
| asan | 158.01 | 52.67 | 43513035368 |
| windows-x64 | 81.37 | 81.37 | 43460663238 |
| wasm32 | 67.22 | 13.44 | 43458609482 |
| linux-x64 | 60.23 | 60.23 | 43514913934 |
| macos-arm64 | 48.09 | 48.09 | 43462502049 |
| windows-arm64 | 43.41 | 43.41 | 43514870188 |
| linux-arm64 | 39.31 | 39.31 | 43460716508 |

Limits considered. Job rows are minutes in `ci.yml`; the lane column is the job itself unless a lane is named. A job row whose old and new values are equal had its comment refreshed to the measured text.

| file | job or test | old | new | slowest s | runs | run id | lane |
|------|-------------|-----|-----|-----------|------|--------|------|
| ci.yml | changes | 5 | 5 | 13 | 40 | 43460716508 | job |
| ci.yml | hygiene | 5 | 5 | 67 | 40 | 43169796348 | job |
| ci.yml | title | 5 | 5 | 17 | 19 | 43460716508 | job |
| ci.yml | build | 12 | 12 | 360 | 36 | 43525961902 | job, slowest matrix leg |
| ci.yml | asan | 9 | 9 | 248 | 36 | 43513035368 | job |
| ci.yml | nofp | 2 | 2 | 53 | 36 | 43158408243 | job, slowest matrix leg |
| ci.yml | be32 | 21 | 21 | 609 | 36 | 43458609482 | job |
| ci.yml | hash-equality | 5 | 5 | 18 | 36 | 43232709477 | job |
| ci.yml | wasm32 | 5 | 5 | 141 | 36 | 43341636921 | job |
| ci.yml | retroarch | 5 | 5 | 62 | 36 | 43232709477 | job |
| ci.yml | perf | 6 | 6 | 106 | 21 | 43334476928 | job (the rule gives 4; raise-only keeps 6) |
| ci.yml | rehearsal | 13 | 14 | 393 | 2 | 43513035368 | job |
| ci.yml | cleanup | 5 | 5 | 15 | 2 | 43164256628 | job |
| ci.yml | CI required | 5 | 5 | 5 | 40 | 43460663238 | job |
| tests/library/tests.cmake | library.diagnose (s) | 206 | 289 | 144.43 | 24 | 43460663238 | macos-x64 |
| tests/hygiene/tests.cmake | hygiene.tree (s) | 60 | 75 | 37.32 | 40 | 43562888383 | hygiene |
| tests/runner/diagnose.cmake | runner.diagnose (s) | 633 | unchanged | 179.23 | 24 | 43460663238 | macos-x64 |

Reading of the table. The rehearsal job is the one job limit that rose: twice 393 s is 786 s and the old limit was 780 s. The `be32`, `build` and `asan` limits already satisfied twice the fresh slowest (`build` exactly: 2 x 360 s is 720 s, the 12 minutes it has). `library.diagnose` and `hygiene.tree` follow the rule: twice 144.43 s is 288.86 s (limit 289) and twice 37.32 s is 74.64 s (limit 75). `runner.diagnose` holds: its largest unscaled-lane reading is 179.23 s, twice that is 358.46 s, under the 633 s it has; the be32 job limit of 21 minutes, not this test limit, is its real guard (the test took 450.48 s of a 589 s be32 job in run 43514870188). In the three CTest comments the `2 x S` text and the limit agree with the registered value; the `nofp` comment keeps its worst-lane note because the figure still comes from the slowest matrix leg.

Host load, not a limit: the first `cmake --workflow --preset hygiene` after the edits exited 8 because `hygiene.tree` timed out at its new 75 s limit while other processes on this machine held the load average near 69; `sh scripts/hygiene.sh --tree` alone took 34 s then. The re-run at a load average near 18 exited 0 (84 of 84, `hygiene.tree` 32.36 s). The hosted slowest is 37.32 s, so the local reading is not a reason to raise a limit (XFER-012).

Checks of the workflow commit `33bbfc2` (`ci: re-derive drifted job and test timeouts from live runs`):

| check | command | exit status |
|-------|---------|-------------|
| one workflow commit | `git log --oneline origin/main..HEAD -- .github/workflows/ci.yml` lists one line, counted with `grep -c .` equal to 1 | 0 |
| last non-planning commit | the newest commit of `git log -1 --format=%H origin/main..HEAD -- . ':(exclude).planning'` equals the newest commit touching `ci.yml` | 0 |
| the sample came first, on the pushed head | the parent of the workflow commit equals the remote branch head by `git ls-remote`, and a green dispatched run on it finished no later than the commit's author time | 0 |
| only timeout files | `git diff --name-only` of the commit lists `ci.yml`, `tests/hygiene/tests.cmake` and `tests/library/tests.cmake` only | 0 |
| raise-only | `paste` of the old and new `timeout-minutes` values from `origin/main` and the tree, then awk for any pair with a lower new value or a missing field (14 lines each, only rehearsal 13 to 14 differs) | 0 |
| window citation | every run ID added on a `timeout-minutes` line is found in the saved durations report | 0 |
| library.diagnose comment | the awk and `grep -Eq` of the plan on the last `# Timeout:` line above its registration | 0 |
| hygiene.tree comment | the same on the comment added above its registration | 0 |
| value and comment agree | `grep` for the comment text and for `TIMEOUT 289` and `TIMEOUT 75` in their files; `awk` check that 289, 75 and 633 are each at least twice 144.43, 37.32 and 179.23 | 0, 0, 0 |
| no `provisional` word | `test -f .github/workflows/ci.yml && ! grep -n 'provisional' .github/workflows/ci.yml` | 0 |
| timeout comments check | `hygiene.timeout_comments` and its self test inside `cmake --workflow --preset hygiene` | 0 (both passed in every run of the preset) |
| hygiene workflow | `cmake --workflow --preset hygiene` (84 of 84), on the phase head after the edits | 0 (first run exit 8 on host load, re-run exit 0) |
| ci workflow | `cmake --workflow --preset ci` on `33bbfc2` (738 of 738, then 5 of 5) | 0 |
| tracer gate | the whole Task 1 verify command re-run after the commit (the git checks, `cmake --workflow --preset hygiene` 84 of 84, `cmake --workflow --preset ci` 738 of 738 then 5 of 5) | 0 |
| untouched files | `git diff --quiet origin/main -- .github/workflows/nightly.yml .github/workflows/digest.yml .github/workflows/release.yml src include` | 0 |
| no pull request, nothing pushed after the workflow commit | `gh pr list --head phase/17-re-run-merge-and-clean-state --state open` returns 0, and the remote branch head is still the sampled head | 0 |

## Ship hand-off

For `/gsd-ship` and the shell after it. None of this ran in plan 04.

| step | command | exit status |
|------|---------|-------------|
| shipped tree matches what was judged where it matters | `git diff --quiet "$(git merge-base HEAD origin/main)" HEAD -- src include CMakeLists.txt cmake` (merge base `0a97075`, which is also the judged commit) | 0 |
| one workflow commit | `git log --oneline origin/main..HEAD -- .github/workflows/ci.yml` lists `33bbfc2` only | 0 |

After the pull request exists (steps for the shell, not for this plan):

- The pull request title is a Conventional Commit, for example `fix(compat): publish the library results judged on main and re-derive drifted timeouts`, with no BREAKING CHANGE text. The release is a patch release, expected `0.15.1`.
- In a shell, never inside GSD command text: `gh pr merge --auto --squash`.
- Confirm a CI run exists on the head commit with `gh run list --branch phase/17-re-run-merge-and-clean-state --limit 1 --json headSha,url` and that its rehearsal job runs, since a `ci.yml` edit is a release input.
- After the merge, confirm the release pull request is green and merged, and that the release has its assets (`gh release view --json tagName,assets --jq '.tagName, (.assets | length)'`, the 19 asset count of `docs/maintaining.md`).
- On the new `origin/main`, re-run `git merge-base --is-ancestor 0a97075f34e48e2b1d0f72aa231006cc9366a398 origin/main` for the verifier (it exits 0 now, and the squash merge does not change it).
- `sh scripts/milestone-close.sh` belongs to the milestone close that follows this phase's release, from a clean `main`.
