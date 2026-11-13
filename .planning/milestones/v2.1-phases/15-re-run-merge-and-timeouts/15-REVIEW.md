---
phase: 15-re-run-merge-and-timeouts
reviewed: 2026-11-13T00:00:00Z
depth: standard
files_reviewed: 6
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/digest.yml
  - .github/workflows/nightly.yml
  - docs/compatibility.md
  - docs/maintaining.md
  - tests/compat/ledger.csv
findings:
  critical: 0
  warning: 5
  info: 3
  total: 8
  necessity: 1
status: issues_found
---

# Phase 15: Code Review Report

**Reviewed:** 2026-11-13
**Depth:** standard
**Files Reviewed:** 6
**Status:** issues_found

## Summary

The phase changed seven `timeout-minutes` lines across three workflows, merged a library re-run into
`tests/compat/ledger.csv` and `docs/compatibility.md`, and added one paragraph to `docs/maintaining.md`. I checked the
changed content directly instead of trusting the run-evidence file.

What holds up:

- **Timeouts.** I re-ran `sh scripts/ci-durations.sh` read-only against the live API for `ci.yml`, `nightly.yml` and
  `digest.yml`. All seven cited seconds and run IDs match (be32 609 s, build 332 s, asan 243 s, bench 368 s,
  bench-wasm32 456 s, fuzz 144 s, digest 55 s). The minutes are `ceil(2 x S / 60)`, with the 5-minute floor where the
  comment names one. `cmake -DCHECK=timeouts -P tests/cmake/repository.cmake` exits 0.
- **Ledger.** Parsed as CSV, base and head both hold 1986 rows with no duplicate key and no added or dropped row. The
  only status change is fail to pass (285 rows); no base pass row changed status. 1116 rows carry the single stamp
  `core=0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4` and the date 2026-11-13. Row order and quoting are unchanged and
  there is no CRLF.
- **Page.** `sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md` exits 0. The page
  totals (1029 + 96 = 1125 evaluated) agree with the ledger.
- **Doc claims.** The new `docs/maintaining.md` paragraph matches `ci.yml` and `scripts/classify-changes.sh`. A hand
  dispatch takes its base from `github.sha`. The release-input paths are the ones listed. The rehearsal job takes 385
  to 387 s, so "about six minutes" is accurate. The `conformance.md#whole-list-ledger` anchor exists.

No Critical issue found. The Warnings are two stale CI controls that this phase's own data exposes (the fuzz
`continue-on-error` and the dead ledger-migration shims), two Nightly robustness gaps, and a traceability problem with
the new ledger stamp. Most were present before the phase and sit in files it touched.

Per project rule 7, findings about workflow and tooling code (WR-01 to WR-04, IN-01 to IN-03) are better recorded as
seeds than as requirements for this phase.

## Warnings

### WR-01: Fuzz job is still `continue-on-error` after its stated exit condition was met

**File:** `.github/workflows/nightly.yml:124-125` (also `docs/perf-fuzz.md:72`)
**Issue:** The comment says "Report-only until ten green nightly runs; then this line goes." The live API shows 24
scheduled nightly runs in a row with a `success` fuzz job, and this phase edited the same job's timeout comment, citing
40 runs. While `continue-on-error: true` stays, a libFuzzer or sanitizer finding leaves the job green.
`report` decides from `needs.*.result` (`jq 'all(.[]; .result == "success")'`, line 179), and a job-level
`continue-on-error` job reports `success`. So no `Nightly failed` issue is ever opened for a fuzz finding. The only trace
is a red step inside a green run, plus the uploaded artifact. This is the "load-bearing `continue-on-error`" pattern.
**Fix:** Delete lines 124-125 and update the sentence in `docs/perf-fuzz.md:72` ("The job is `continue-on-error` until
ten green nightly runs") to say a finding fails the night and opens the nightly issue. If the owner wants to keep it
report-only, rewrite the comment to say why, so it is not a stale promise.

### WR-02: Ledger-format migration and first-introduction branches in the compat step can no longer run

**File:** `.github/workflows/ci.yml:198-216` (guards at 147-151)
**Category:** necessity
**Issue:** Phantom requirement (a migration shim for data the repo proves was migrated). The `if` branch fires only when
the base ledger header is the old nine-column header `v1` and the head header is `v2`. The comment on lines 198-200
says "Dead once the base header is the new one." The base is `github.event.pull_request.base.sha`. For `push`,
`workflow_dispatch` and `workflow_call` the base is the commit itself, so head equals base. On the current `main`
(621dd02) the ledger header is already `v2`, because the `tv` column landed in 1449a6d (#31). So the condition cannot be
true for any pull request into `main`. The `else` branch ("first introduction") needs a base without
`scripts/compat-ratchet.sh`. `git cat-file -e 621dd02:scripts/compat-ratchet.sh`, `:quality.json` and
`:tests/compat/ledger.csv` all succeed, so that branch and the `if [ -f quality.json ]` and
`if [ -f scripts/compat-ratchet.sh ]` guards at lines 147-148 cannot fail either. The dead path is three executable
branches in the step that protects the compatibility ledger. A reader has to prove each one unreachable before
trusting the step, and the `v1` and `v2` literals must be kept in sync with any future header change.
**Fix:** Keep one path. Remove the `v1`/`v2` variables, the migration branch and the first-introduction branch, and
call `sh "$trusted/scripts/compat-ratchet.sh" ...` unconditionally. Drop the two `-f` guards in the preservation step
and let `cp` fail loudly if the base is missing a file. Removal is a scope decision for the owner, which is why this is
not Critical.

### WR-03: Nightly runs on pull requests with no concurrency group, and overlaps CI's macOS jobs

**File:** `.github/workflows/nightly.yml:19-32` (no `concurrency:` block anywhere in the file); `docs/maintaining.md:164-166`
**Issue:** Any pull request that touches `nightly.yml` (this phase's does), the fuzz, z80 or vector tests, or the bench
sources runs the whole Nightly workflow. That includes `bench (macos-15)`, the wasm32 bench (about 6 minutes) and fuzz.
The live API shows 19 of the 52 most recent Nightly runs were `pull_request` events. Unlike `ci.yml`, the file has no
`concurrency` group, so each push to such a pull request starts another full run and never cancels the superseded one.
It also runs a macOS runner beside CI's macOS builds, which contradicts `docs/maintaining.md:164`: "Never run two
macOS-using workflows at once (CI and Nightly)". The paragraph this phase added says every push "reruns the matrix" and
warns about the rehearsal, but does not mention that an edit of `nightly.yml` reruns Nightly as well.
**Fix:**
```yaml
concurrency:
  group: nightly-${{ github.event.pull_request.number || github.run_id }}
  cancel-in-progress: ${{ github.event_name == 'pull_request' }}
```
Add a sentence to the new `docs/maintaining.md` paragraph: "A change to `nightly.yml`, the fuzz, z80 or vector tests or
the bench sources also runs Nightly, macOS bench included."

### WR-04: Nightly `apt-get` calls have no retries; every `ci.yml` call has them

**File:** `.github/workflows/nightly.yml:91` and `:132`
**Issue:** All four `apt-get` sites in `ci.yml` (lines 277, 317, 362, 589) use `-o Acquire::Retries=3`. The two in
`nightly.yml` use plain `sudo apt-get update && sudo apt-get install -y ...`. `docs/maintaining.md:170` documents
"Temporary failure resolving" runner faults at apt steps. A transient fault in `bench-wasm32` fails the job and the
`report` job opens or comments on a `Nightly failed` issue, which is noise a single retry would absorb. The `fuzz`
job's comment names "(apt)" as the reason for its 5-minute floor, and it hides the same fault behind WR-01.
**Fix:** Use `sudo apt-get -o Acquire::Retries=3 update && sudo apt-get -o Acquire::Retries=3 install -y ...` at both
sites, as in `ci.yml`.

### WR-05: The new ledger stamp names an unpushed, docs-only commit that a squash merge will leave outside `main`

**File:** `tests/compat/ledger.csv` (1116 evidence cells, `core=0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4`); `docs/compatibility.md`
**Issue:** Commit 0b4ae67 is `docs(15): create phase plan`. `git branch -r --contains` finds it on no remote branch, and
`git ls-remote --heads origin 'phase/15*'` prints nothing. The previous stamp (8fc3905) was an ancestor of `origin/main`,
so `git checkout <stamp>` reproduced the judged core from a clone of `main`. This one is not an ancestor of `main` and,
under rule 10's squash merge, never will be. A fresh clone cannot `git cat-file -e` it without fetching
`refs/pull/<n>/head`. Rule 12 says a pin must answer 200 at `gh api repos/<o>/<r>/commits/<sha>` and never point at an
unpushed branch. That check is deferred to ship, and the stamp fails it until the push. After the merge, the ledger's
only record of which core was judged is a commit outside the default branch's history. The tree is equal to the merged
tree outside `.planning/`, but nothing in the repo says so.
**Fix:** Run the post-push `gh api` check at ship as planned. Then add one sentence to
`docs/conformance.md#whole-list-ledger` saying a `core=` stamp from a phase branch resolves through
`git fetch origin pull/<n>/head`, and that its tree matches the merge commit outside `.planning/`. For later re-runs,
judge a commit already on `main`, as the previous milestone did.

## Info

### IN-01: Comments on the jobs left alone cite measurements the live data no longer supports

**File:** `.github/workflows/ci.yml:335, 354, 390, 577`
**Issue:** `docs/testing.md` says a timeout comment states S and N from "the last 40 successful runs". The live read
shows the cited numbers have drifted for jobs this phase did not touch.

| Job | Comment says | Live read (40-run window) |
|---|---|---|
| `wasm32` | 2 x 67 s | 141 s |
| `retroarch` | 2 x 36 s, "slowest of 1 run" | 62 s over 37 runs |
| `hash-equality` | 2 x 9 s | 18 s |
| `perf` | 2 x 151 s | 106 s |

`wasm32` has the thinnest margin: its 300 s floor is 18 s above twice the real slowest run (282 s). `hygiene` is close
too, at 300 s against 276 s. `hygiene.timeout_comments` only checks that the timeout is at least twice the cited S, so
it cannot catch this. Phase 15 left these alone on purpose because the minutes still satisfy the rule. The result is
comments that no longer say what was measured.
**Fix:** Run `sh scripts/ci-durations.sh ci.yml 40` and refresh the S, N and run ID on the comment lines above in the
same edit that next touches a timeout. Consider a periodic check that flags a comment whose cited S is under half the
live slowest.

### IN-02: `gh pr list` in the digest silently stops at 30 pull requests

**File:** `.github/workflows/digest.yml:74`
**Issue:** `gh pr list --state open --json ...` returns at most 30 results by default. With more than 30 open pull
requests, the red and seven-day-idle checks silently skip the rest. The digest is the project's only alarm for a
red or idle pull request.
**Fix:** `gh pr list --state open --limit 200 --json number,title,updatedAt,headRefOid`.

### IN-03: The generated page cuts an HTML entity in half in two rows

**File:** `docs/compatibility.md:160-161` (generator `scripts/compat-common.sh:20`)
**Issue:** `x=substr(md(r[k,2]),1,64)` truncates after escaping. The Dinobasher rows end in `prototype&# |`, a broken
`&#41;` that renders as literal `&#`. This is not new: the same two rows were cut the same way in the base page. The
same fault can cut `&amp;`, `&#40;` and the others in any long title.
**Fix:** Truncate the raw title, then escape: `x=md(substr(r[k,2],1,N))`. Pick N so the page stays under its size cap,
then regenerate the page and `tests/compat/ledger.csv`-derived outputs together.

---

_Reviewed: 2026-11-13_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
