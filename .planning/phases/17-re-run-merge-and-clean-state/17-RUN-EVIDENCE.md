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
