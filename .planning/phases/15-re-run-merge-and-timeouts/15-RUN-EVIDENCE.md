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

## Hand-off

The judged commit above becomes reachable on the remote once the branch is pushed at /gsd-ship; the verifier then runs `gh api repos/szTheory/cogmaster/commits/0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4` and records its status (the run's stamp must be a public commit). The CI run URL for the pushed head is recorded at ship. Plan 2 makes the workflow commit after the ledger commit.
