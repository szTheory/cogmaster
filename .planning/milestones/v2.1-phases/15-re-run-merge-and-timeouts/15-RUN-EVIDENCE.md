# Phase 15 run evidence

Counts only. Each count is a table cell tied to the command that printed it and its exit status; no run output is pasted.

- judged commit (the single full-length `core=` value on the merged rows): 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4
- date (UTC): 2026-11-13
- behaviour revision: 18 (`include/cogmaster.h`, unchanged against origin/main)
- base: origin/main at 621dd02936f93c5d19b7878c344060a8518a296f

## Library run

status: owner run accepted. The owner ran `sh scripts/library.sh` once, with no argument, from the repository root in their own terminal, at the judged commit above. The owner's report said the run was done and pasted the workflow's test step (2 of 2 tests passed); it pasted no `library:` lines and no `merge:` line, so the `library:` counts are `not reported`. The only file of the results directory that was opened is `build/library/results/summary.txt`, written at 13:01 local time on the run day, the same minute as the ledger and the page.

| count | value | printed by | read from | exit status |
| ----- | ----- | ---------- | --------- | ----------- |
| library test step, tests passed | 2 of 2 | sh scripts/library.sh (owner) | owner paste | not reported |
| library test step, wall time (s) | 644.2 | sh scripts/library.sh (owner) | owner paste | not reported |
| candidates | not reported | sh scripts/library.sh (owner) | not reported | not reported |
| attempted | not reported | sh scripts/library.sh (owner) | not reported | not reported |
| evaluated | not reported | sh scripts/library.sh (owner) | not reported | not reported |
| predicates | not reported | sh scripts/library.sh (owner) | not reported | not reported |
| status counts | not reported | sh scripts/library.sh (owner) | not reported | not reported |
| derived default-profile carts run (the PAL line's carts count) | 0 | sh scripts/library.sh (owner) | summary.txt | not reported |
| PAL profiles run | 1 | sh scripts/library.sh (owner) | summary.txt | not reported |
| PAL boots pass | 1 | sh scripts/library.sh (owner) | summary.txt | not reported |
| PAL boots fail | 0 | sh scripts/library.sh (owner) | summary.txt | not reported |
| FM profiles (derived) | skipped, not configured | sh scripts/library.sh (owner) | summary.txt | not reported |
| rows added | 0 | sh scripts/library.sh (owner) | summary.txt | not reported |
| ratchet verdict | ok | sh scripts/library.sh (owner) | summary.txt | not reported |
| page bytes | 13813 | sh scripts/library.sh (owner) | summary.txt | not reported |
| ledger rows | 1986 | sh scripts/library.sh (owner) | summary.txt | not reported |
| pass | 1029 | sh scripts/library.sh (owner) | summary.txt | not reported |
| fail | 96 | sh scripts/library.sh (owner) | summary.txt | not reported |
| unknown | 832 | sh scripts/library.sh (owner) | summary.txt | not reported |
| unsupported | 5 | sh scripts/library.sh (owner) | summary.txt | not reported |
| skipped | 24 | sh scripts/library.sh (owner) | summary.txt | not reported |
| infrastructure_error | 0 | sh scripts/library.sh (owner) | summary.txt | not reported |
| fixture pass | 9 | sh scripts/library.sh (owner) | summary.txt | not reported |
| fixture PAL judged | 1 | sh scripts/library.sh (owner) | summary.txt | not reported |
| fixture FM judged | 2 | sh scripts/library.sh (owner) | summary.txt | not reported |
| cart pass against floor | 554 against 20 | sh scripts/library.sh (owner) | summary.txt | not reported |
| cart PAL judged against floor | 511 against 1 | sh scripts/library.sh (owner) | summary.txt | not reported |
| cart FM judged against floor | 51 against 1 | sh scripts/library.sh (owner) | summary.txt | not reported |
| judged-cart floors | met | sh scripts/library.sh (owner) | summary.txt | not reported |
| experimental flag | absent | sh scripts/library.sh (owner) | summary.txt | not reported |
| unowned unknown | 832 | sh scripts/library.sh (owner) | summary.txt | not reported |
| cart PAL rows | 511 | sh scripts/library.sh (owner) | summary.txt | not reported |
| cart FM rows | 51 | sh scripts/library.sh (owner) | summary.txt | not reported |
| exit verdict | pass | sh scripts/library.sh (owner) | summary.txt | not reported |

The rows of the ledger that the run re-judged were rewritten in place: 1116 ledger rows carry a new evidence and date (passes included), 0 rows were added.

## Merge checks

All commands ran from the repository root, with no change of directory. A literal pipe inside a command is written `\|`.

| check | command | exit status |
| ----- | ------- | ----------- |
| acceptance, merge line | `grep -Eq '^merge: rows_added=-?[0-9]+ ratchet=ok page_bytes=[0-9]+$' build/library/results/summary.txt` | 0 |
| two-file status | `git diff --quiet HEAD -- . ':(exclude).planning' ':(exclude)docs/compatibility.md' ':(exclude)tests/compat/ledger.csv' && ! git diff --quiet HEAD -- tests/compat/ledger.csv && ! git diff --quiet HEAD -- docs/compatibility.md` | 0 |
| stamp, one full-length core value, an ancestor, nothing outside .planning changed since | `d=$(git diff -U0 -- tests/compat/ledger.csv) && r=$(printf '%s\n' "$d" \| sed -n 's/^+.*core=\([0-9a-f]\{40\}\).*/\1/p' \| sort -u) && test "$(printf '%s\n' "$r" \| grep -c .)" -eq 1 && git merge-base --is-ancestor "$r" HEAD && git diff --quiet "$r" HEAD -- . ':(exclude).planning'` | 0 |
| stamp positive control, merge 032ebab against its parent | the same body over `git show --format= 032ebab -- tests/compat/ledger.csv` and `032ebab^` | 0 |
| transition count | `awk` over the base ledger (`git show origin/main:tests/compat/ledger.csv` saved under `$TMPDIR`) and `tests/compat/ledger.csv`, keyed by id, region, tv and predicate, exiting 1 when any base pass row is not pass now (the one-off program of Task 2, output kept under `$TMPDIR`) | 0 |
| page equals its regeneration | `sh scripts/compat-common.sh docs tests/compat/ledger.csv \| cmp - docs/compatibility.md` | 0 |
| configure | `cmake --preset ci > /dev/null` | 0 |
| build | `cmake --build --preset ci` | 0 |
| compat tests (3 of 3 passed) | `ctest --test-dir build/ci -R '^compat\.(exit\|whole_list\.fresh\|page_bound)$' --output-on-failure --no-tests=error` | 0 |
| totals quoted elsewhere | `grep -En` for each of 744, 381, 1029 and 96 beside pass or fail over `README.md` and `docs/*.md`, matches in `docs/compatibility.md` dropped (run from a scratch sh script, because the interactive shell could not parse the loop inline); 0 other-file matches for each total | 0 |
| allow-list, floors and behaviour header untouched | `git diff --quiet origin/main -- tests/compat/allow.txt quality.json include/cogmaster.h` | 0 |

Page totals: the base page read pass 744 and fail 381, the merged page reads pass 1029 and fail 96; the difference of 285 equals the sum of the transitions below. No base pass row was lost (0). No other tracked file quotes the page's pass or fail totals.

Transitions by the transition count (rows whose status changed; there is no other kind of change):

| machine | tv | fm | from | to | rows | printed by |
| ------- | -- | -- | ---- | -- | ---- | ---------- |
| gg | ntsc | off | fail | pass | 18 | transition count |
| sms | ntsc | off | fail | pass | 126 | transition count |
| sms | ntsc | on | fail | pass | 16 | transition count |
| sms | pal | off | fail | pass | 125 | transition count |

Fail rows left in the merged ledger by the same program: 50 with tv ntsc (48 with fm off on sms, 2 with fm on) and 46 with tv pal. The 50 ntsc fail rows are within one of the 49 default-profile fails of the Phase 14 diagnose (the count includes the Japanese FM profile rows); this is a sanity reading, not a gate, so no counts-only diagnose run is asked of the owner.

## Local gates

Run on the commit that carries the merged ledger and page, from the repository root.

| check | command | exit status |
| ----- | ------- | ----------- |
| the ci workflow (676 tests in its test step and 5 in its release test step passed) | `cmake --workflow --preset ci` | 0 |
| the old base judge (the origin/main copies of the ratchet and the common script) over the merged ledger, behaviour revisions 18 and 18, printing the protected-comparison pass line | `sh "$t/scripts/compat-ratchet.sh" "$t/base.csv" tests/compat/ledger.csv tests/compat/allow.txt "$(date -u +%F)" 18 18 build/ci/tests/compat-fixtures/results.csv`, with `t="$TMPDIR/p15-base"` holding those `git show origin/main:` copies | 0 |
| nothing outside .planning changed on the branch but the ledger and the page | `git diff --quiet origin/main...HEAD -- . ':(exclude).planning' ':(exclude)docs/compatibility.md' ':(exclude)tests/compat/ledger.csv'` | 0 |

## Timeouts

Read at 2026-11-13T19:04Z, immediately before the edit, from the repository root with no change of directory. No workflow was dispatched. The saved reads live under `$TMPDIR` as `p15-durations-<file>.tsv`, and the edited comments cite those reads, so a run that finishes later changes nothing.

| check | command | exit status |
| ----- | ------- | ----------- |
| read, CI | `sh scripts/ci-durations.sh ci.yml 40 > "$TMPDIR/p15-durations-ci.yml.tsv"` | 0 |
| read, Nightly | `sh scripts/ci-durations.sh nightly.yml 40 > "$TMPDIR/p15-durations-nightly.yml.tsv"` | 0 |
| read, Digest | `sh scripts/ci-durations.sh digest.yml 40 > "$TMPDIR/p15-durations-digest.yml.tsv"` | 0 |
| hygiene arithmetic check | `cmake -DDIR="$PWD/.github/workflows" -DCHECK=timeouts -P tests/cmake/repository.cmake` | 0 |
| cross-check against the saved reads | the loop of the plan's verify command: for the seven edited lines, minutes (the script's minutes, at least the floor where the line has one), seconds and run ID equal the saved row | 0 |
| only timeout lines changed | `git diff -U0 origin/main -- .github/workflows` with every changed line that is not a `timeout-minutes:` line filtered out, the filter's output empty | 0 |

| workflow | job | before | after | slowest s | runs | run |
| -------- | --- | ------ | ----- | --------- | ---- | --- |
| CI | asan | 5 | 9 | 243 | 37 | https://github.com/szTheory/cogmaster/actions/runs/43462502049 |
| CI | build | 9 | 12 | 332 | 37 | https://github.com/szTheory/cogmaster/actions/runs/43460663238 |
| CI | be32 | 15 | 21 | 609 | 37 | https://github.com/szTheory/cogmaster/actions/runs/43458609482 |
| Nightly | bench | 10 | 13 | 368 | 40 | https://github.com/szTheory/cogmaster/actions/runs/42764095033 |
| Nightly | bench-wasm32 | 15 | 16 | 456 | 40 | https://github.com/szTheory/cogmaster/actions/runs/42768407261 |
| Nightly | fuzz | 5 | 5 | 144 | 40 | https://github.com/szTheory/cogmaster/actions/runs/43020467034 |
| Digest | digest | 5 | 5 | 55 | 1 | https://github.com/szTheory/cogmaster/actions/runs/42769805382 |

The fuzz and digest rows change their comment only: the 5-minute floor still sets the minutes (twice 144 s is 288 s, twice 55 s is 110 s). The fuzz comment named 90 s (26 runs) and now names 144 s of the 40-run window; the digest comment read `no runs yet` and now names the one dispatch run. No job's minutes fell; every edit loosens a limit. No outlier was trimmed and no ceiling was added.

build and be32 were corrected in the same edit, because both were below twice the fresh slowest (build: 9 minutes, 540 s, against 664 s; be32: 15 minutes, 900 s, against 1218 s). The cause is test growth, not the runner: the runner and library diagnose tests arrived after those limits were set.

Left alone, with run IDs that are not the fresh slowest but the rule holding against the fresh read: wasm32 (300 s against 282 s), retroarch (300 s against 124 s), perf (360 s against 212 s), hygiene (300 s against 276 s) and nofp (120 s against 106 s). The other CI and Nightly jobs also hold; the rehearsal is the narrowest margin (780 s against 770 s) and is untouched.

## Hand-off

The judged commit above becomes reachable on the remote once the branch is pushed at /gsd-ship; the verifier then runs `gh api repos/szTheory/cogmaster/commits/0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4` and records its status (the run's stamp must be a public commit). The CI run URL for the pushed head is recorded at ship. Plan 2 made the workflow commit after the ledger commit (afd413e); it is the newest commit on the branch that touches anything outside `.planning/`.

## Final gates

Run on the head that carries the timeouts commit, from the repository root; nothing is pushed.

| check | command | exit status |
| ----- | ------- | ----------- |
| the ci workflow (676 tests in its test step and 5 in its release test step passed) | `cmake --workflow --preset ci` | 0 |
| the hygiene workflow (84 tests passed) | `cmake --workflow --preset hygiene` | 0 |
| the timeouts commit is the newest commit touching anything outside `.planning/` | `w=$(git log -1 --format=%H origin/main..HEAD -- .github/workflows/ci.yml) && l=$(git log -1 --format=%H origin/main..HEAD -- . ':(exclude).planning') && test -n "$w" && test "$w" = "$l"` | 0 |
| exactly one commit touches the first workflow file, and it also touches the other two | `git log --format=%H origin/main..HEAD -- .github/workflows/ci.yml` counted (1), then `git show --name-only` of it | 0 |
| the branch's non-planning changes are the ledger, the page, the three workflows and the maintaining guide | `git diff --quiet origin/main...HEAD -- . ':(exclude).planning'` with the six paths excluded | 0 |
| each of those six paths changed | `git diff --quiet origin/main...HEAD -- <path>` is false for each | 0 |
| no test `TIMEOUT` line changed | `git diff origin/main...HEAD -- tests CMakeLists.txt` filtered for added or removed `TIMEOUT` lines, none found | 0 |
| nothing pushed | `git ls-remote --heads origin phase/15-re-run-merge-and-timeouts` printed nothing | 0 |

## Ship hand-off

The title and body below are written once, here, and are used unchanged by the ship step (every edit of a pull request's title or body reruns the matrix). The body becomes the squash message, so it carries counts only.

### Title

fix(compat): publish the post-fix library results and re-derive job timeouts

### Body

Publishes the compatibility results judged at the fix head and re-derives the CI job timeouts that had drifted from the twice-the-slowest rule.

- rows flipped (fail to pass): 285
- newly judged: 0 (no row moved from unknown or absent, and 0 rows were added)
- other transitions: none
- page totals: pass 1029 and fail 96 (the earlier page read pass 744 and fail 381); 1986 rows, 1125 evaluated
- judged on the page: 512 PAL rows and 53 FM rows
- merge: rows_added=0, ratchet=ok, page_bytes=13813
- no pass row lost (the transition count exited 0)
- judged-cart floors: met (554 cart passes against 20, 511 PAL rows judged against 1, 51 FM rows judged against 1)
- fail rows left: 50 with ntsc (48 with fm off, 2 with fm on) and 46 with pal; the earlier diagnose read 49 default-profile fails
- expected churn: 1116 re-judged rows carry a new evidence stamp and date, passes included
- Nightly changes: the fuzz job is no longer report-only, a superseded Nightly run on a pull request is cancelled, and the Nightly package installs retry

Job timeouts, each twice the slowest successful run of the last 40 runs, rounded up to whole minutes, with the 5-minute floor kept where one applied:

| workflow | job | before | after | slowest s | runs | run |
| -------- | --- | ------ | ----- | --------- | ---- | --- |
| CI | asan | 5 | 9 | 243 | 37 | https://github.com/szTheory/cogmaster/actions/runs/43462502049 |
| CI | build | 9 | 12 | 332 | 37 | https://github.com/szTheory/cogmaster/actions/runs/43460663238 |
| CI | be32 | 15 | 21 | 609 | 37 | https://github.com/szTheory/cogmaster/actions/runs/43458609482 |
| Nightly | bench | 10 | 13 | 368 | 40 | https://github.com/szTheory/cogmaster/actions/runs/42764095033 |
| Nightly | bench-wasm32 | 15 | 16 | 456 | 40 | https://github.com/szTheory/cogmaster/actions/runs/42768407261 |
| Nightly | fuzz | 5 | 5 | 144 | 40 | https://github.com/szTheory/cogmaster/actions/runs/43020467034 |
| Digest | digest | 5 | 5 | 55 | 1 | https://github.com/szTheory/cogmaster/actions/runs/42769805382 |

The fuzz and digest rows only re-cite a run in their comment. build and be32 were below the rule and are corrected in the same edit; the cause is test growth in the diagnose tests, not the runner. The jobs wasm32, retroarch, perf, hygiene and nofp keep their limits and their older run references, because the rule holds for them against the same read.

### Next steps

code review: pending (/gsd-code-review 15 runs next and records run or not applicable in its own review record, D-19)

These lines are not part of the squash text.

1. `/gsd-code-review 15`, first, because review runs before verification (D-19).
2. `/gsd-verify-work 15`.
3. `/gsd-ship 15`, which pushes once and opens the pull request non-draft with the title and body above.
4. As separate shell steps after `/gsd-ship` returns (rule 10): `gh pr merge --auto --squash`, then check that a CI run exists on the head commit with `gh run list --commit <sha>` (D-14).
5. Once the branch is pushed, the core-commit reachability check of D-06: `gh api repos/szTheory/cogmaster/commits/0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4` must answer 200; record the command and its status.
6. After the merge, confirm the release pull request is green and that no pull request labelled `autorelease: pending` is left: `gh pr list --label 'autorelease: pending'` prints nothing, as `docs/maintaining.md` describes (D-14 step 6, rule 10). Only then is the phase's release done.
