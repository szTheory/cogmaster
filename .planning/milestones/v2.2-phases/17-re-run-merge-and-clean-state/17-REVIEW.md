---
phase: 17-re-run-merge-and-clean-state
reviewed: 2026-11-15T02:21:35Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - .github/workflows/ci.yml
  - docs/conformance.md
  - docs/maintaining.md
  - docs/runner.md
  - runner/diag.c
  - runner/diag_class.h
  - scripts/library.sh
  - scripts/milestone-close.sh
  - tests/hygiene/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/wasm32.txt
  - tests/library/diagnose.cmake
  - tests/library/tests.cmake
  - tests/policy/repository-checks.txt
  - tests/policy/tooling_cases.cmake
  - tests/runner/diagnose.cmake
  - tests/runner/test_runner.c
findings:
  critical: 0
  warning: 5
  info: 4
  total: 9
  necessity: 0
status: issues_found
---

# Phase 17: Code Review Report

**Reviewed:** 2026-11-15T02:21:35Z
**Depth:** standard
**Files Reviewed:** 17
**Status:** issues_found

## Summary

Scope was the 17 listed files, as changed since `0a97075f34e48e2b1d0f72aa231006cc9366a398`. `tests/compat/ledger.csv` and `docs/compatibility.md` were left out as instructed. No file under the owner's library directory or `.env.local` was read, printed or named.

No Critical issues. The shape-2 prune is consistent end to end. The format string and its 21 arguments in `runner/diag.c` agree. The awk field numbers (`NF == 30`, `$9 == "2"`, `$29`, `$30`) and the sub-row labels (`pcslot where pc faddr vdpmode size`) agree. The sort keys only depend on relative order, so removing `rom_write` and `ctrl` from the closed lists does not reorder anything. A grep for the pruned words (`wclass`, `pclass`, `diagnose 1`, `dev=ctrl`, `where=port`) finds no stale reference outside the public-header name tables, which keep `rom_write` and `ctrl` on purpose.

Checks run read-only, with scratch output under the scratchpad:
- The `policy.milestone_close` driver and fake `gh` were extracted verbatim, and the extraction was shown byte-identical to what `file(CONFIGURE ... @ONLY)` writes. All 11 cases pass under macOS `sh` and under `dash`.
- The jq filters were run on sample JSON and behave as intended: an empty `check_runs` gives `null`, which maps to unknown; the compare filter counts only `feat`, `fix` and `perf`, including `!` and `(scope)` forms, and not `fixup!`.
- Mutations were applied to a committed copy of the script. Removing the draft check, or making `count` treat unknown as ok, is caught. Three other mutations are not (see IN-01).
- Run against the old `library.sh`, the new linked-worktree fixture fails in the way the fix prevents, so that test has teeth.
- The `taxonomy` and `leak_title` diagnose cases pass or fail as registered, `docs.budgets` passes, and `hygiene.sh --tree` exits 0.

Necessity (#4100) was assessed, using `necessity-review.md`. No finding qualifies. The kept `rom_write` and `ctrl` name tables and the `unknown` default in `cogmaster__diag_where` serve the public header and a forward-compatibility rule that `docs/runner.md` states.

The weaknesses are in the new tooling. A re-derived timeout was not re-derived, one `gh` answer can read green when it is incomplete, the dotenv lookup depends on a git flag older versions lack, a new fixture omits the signing override that every sibling sets, and the new script's classification no longer matches the size it was classified at.

## Warnings

### WR-01: `perf` timeout comment was re-derived but the value was not

**File:** `.github/workflows/ci.yml:577`
**Issue:** The comment now reads `2 x 106 s, slowest of 21 runs`, but the value is still `6`, which is what the old `2 x 151 s` measurement gave (302 s rounds up to 6 minutes). `docs/testing.md:331-333` defines a timeout as twice the slowest run, rounded up to minutes: 212 s is 4 minutes. A 5-minute floor applies only to network-bound and roll-up jobs. Whichever reading holds, 6 is neither. `hygiene.timeout_comments` checks only that the timeout is at least twice S, so a value that stayed high passes, and the phase that exists to re-derive timeouts leaves one loose. Every other `timeout-minutes` line in the diff matches its comment: 12, 9, 2, 21, 14 and the 5-minute floors.
**Fix:**
```yaml
    timeout-minutes: 4  # 2 x 106 s, slowest of 21 runs, run 43334476928
```
If `perf` counts as network-bound, use `5  # 5-minute floor; 2 x 106 s, slowest of 21 runs, run 43334476928`.

### WR-02: `release-pr` row can read green from an incomplete search

**File:** `scripts/milestone-close.sh:76-77`
**Issue:** The header promises that a failed `gh` call becomes an unknown row and "cannot be a green one". The search endpoint breaks that promise. When a search times out, GitHub answers HTTP 200 with `"incomplete_results": true` and a `total_count` that may be 0. `--jq .total_count` then yields `0`, and `count` prints `release-pr ok none pending`. This is the one row that catches a merged release pull request whose tag step failed. The search index is also eventually consistent, but the `incomplete_results` flag is the part the script can act on.
**Fix:** Fold the flag into the filter so that `count`'s existing non-numeric branch reports unknown:
```sh
p=$(gh api "search/issues?q=repo:$repo+is:pr+is:merged+label:%22autorelease:%20pending%22" \
  --jq 'if .incomplete_results then "incomplete" else .total_count end') || p=unknown
```
The fake `gh` answers already-filtered text, so add a case that answers `incomplete` for `search/issues*` and expects `release-pr unknown` and exit 2.

### WR-03: The dotenv fallback aborts the script on git older than 2.31

**File:** `scripts/library.sh:8`
**Issue:** `git rev-parse --path-format=absolute --git-common-dir` needs git 2.31 or newer. Older git echoes the unknown flag and prints a relative `.git`. I confirmed that git echoes an unknown flag, and that `dirname "$(printf '%s\n%s\n' --path-format=absolute .git)"` exits 1 with "illegal option". Because `envfile=$(dirname ...)/.env.local` is a plain assignment, `set -eu` ends the script with no message. The line runs on every call that has no `$root/.env.local`, and before the `COGMASTER_ROM_DIR` test, so it also breaks a run where the variable is already exported. This is the first use of `--path-format` in the repository, and no minimum git is documented. The `2>/dev/null` hides only git's stderr, not `dirname`'s.
**Fix:** Ask git for the main working tree, which works from git 2.7 and also holds for a separate git directory, and do it only when needed:
```sh
envfile=$root/.env.local
if [ -z "${COGMASTER_ROM_DIR-}" ] && [ ! -f "$envfile" ]; then
  main_root=$(git worktree list --porcelain | sed -n '1s/^worktree //p')
  [ -z "$main_root" ] || envfile=$main_root/.env.local
fi
```
The first `worktree` entry is always the main one.

### WR-04: The new worktree fixture commit does not disable signing

**File:** `tests/library/tests.cmake:296-297`
**Issue:** `git -c user.name=test -c user.email= commit -q -m fixture` has no `-c commit.gpgsign=false`. Every other fixture commit in the tree sets it: `tests/policy/tooling_cases.cmake:141` and `:239`, and `tests/policy/firmware_window_cases.cmake:37`. A contributor whose global git config signs commits gets a failed or hanging `library.synthetic`, and `COMMAND_ERROR_IS_FATAL ANY` then fails the whole test with an unrelated message. The empty `user.email=` is accepted by git 2.41 (tested), but it is the only fixture that depends on that.
**Fix:**
```cmake
  execute_process(COMMAND git -c user.name=test -c user.email=fixture@users.noreply.github.com
    -c commit.gpgsign=false commit -q -m fixture
    WORKING_DIRECTORY "${dotenv}" COMMAND_ERROR_IS_FATAL ANY)
```

### WR-05: The `operation` classification now rests on a size the script no longer has

**File:** `tests/policy/repository-checks.txt:19`
**Issue:** The reason recorded for classifying the script as `operation` was that the budget "carries no risk either way": about 45 lines would make 932 counted check lines against a limit of 973 (`17-CONTEXT.md`, D-14). The script shipped at 103 lines. As a `check` it would bring the numerator to 990, and `990 * 10` exceeds the 9738 `src` lines, so `docs.budgets` would fail. I measured 887 counted lines with `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`. The only thing keeping the budget green is the classification. The script's own text says it asserts a repository fact and exits 1 on it (a clean copy of main), while `docs/maintaining.md` and `docs/testing.md` reserve `operation` for "build, runtime or release operations". `ci-durations.sh`, the sibling used as precedent, is described as one that "never fails on repository content". This is a judgment call for the owner, not a proven defect. The claim that the budget is not at risk is now false.
**Fix:** Either keep `operation` and replace the justification with the real one (a read of remote state that is not run in CI), or trim the script to what the budget allows. Dropping the repeated per-row reason strings and folding the two helpers would cut it substantially. Do not leave the stale "no risk" premise as the record.

## Info

### IN-01: Three off-paths of the milestone-close check have no control

**File:** `tests/policy/tooling_cases.cmake:169-226`
**Issue:** The fake `gh` always answers `success` for `commits/v*`, and the throwaway repository is always on `main` at its own remote head. I committed a mutated copy of the script for each of these and ran the driver:
- Removing the `CI required` red-on-tag branch (`milestone-close.sh:68-69`) leaves all 11 cases green.
- Removing the `branch != main` branch (`:79-80`) leaves all 11 cases green.
- Removing the `head != main` branch (`:85-86`) leaves all 11 cases green.

Removing the draft check, or making `count` treat unknown as ok, is caught (`draft` and `offline` fail). Under the project's rule that review findings about test code become seeds, this is a seed, not a requirement.
**Fix:** Add `red_tag_ci` (the fake answers `failure` for `commits/v*`), `detached` (`git checkout -q --detach` before the run) and `ahead` (an extra local commit) as `check` lines with the expected `release` or `tree` row. Note that `check()` runs `git checkout -q -- .`, so a changed script or extra commit has to be set up inside the case, not before the driver starts.

### IN-02: Windows argument conversion is guarded for one call only

**File:** `scripts/milestone-close.sh:55,76`
**Issue:** The comment at `:54` records that a Windows shell rewrote `origin/main:path` into a `;`-separated list, and `git show` gets `MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'`. The search argument at `:76` has the same shape (a slash, then `repo:o/r+is:pr...`) and is passed to a native `gh.exe` with no guard. I could not run it on a Windows host, so this is unconfirmed. The test cannot show it either, because the fake `gh` is a shell script and receives its arguments unchanged. If it does mis-convert, the row would read unknown, never green.
**Fix:** Export the exclusion once near the top (`MSYS2_ARG_CONV_EXCL='*'; export MSYS2_ARG_CONV_EXCL`) and drop the per-call prefix, or confirm the search row once on a Windows host.

### IN-03: The dotenv lookup was fixed in one of three readers

**File:** `scripts/library.sh:7-8` (also `scripts/hygiene.sh:63` and `scripts/firmware-window.sh:10`, which are outside the listed files)
**Issue:** `library.sh` now finds the main checkout's `.env.local` from a linked worktree. `hygiene.sh` `fw_scan` and `firmware-window.sh` still read `./.env.local` only. In a linked worktree the firmware-window scan (the privacy gate for the owner's firmware dump) treats `COGMASTER_BIOS_DIR` as unset and returns 0 without comparing anything. `docs/conformance.md` now tells the owner to work in a detached worktree, which makes that path more likely. This is old behaviour, but the new worktree workflow widens its reach.
**Fix:** Move the lookup into one small helper that all three scripts source or call (for example the `git worktree list --porcelain` form in WR-03), or note in `docs/conformance.md` that hygiene's firmware scan is a no-op in a linked worktree.

### IN-04: `policy.milestone_close` timeout rests on a "few processes" estimate

**File:** `tests/policy/tooling_cases.cmake:167,248-250`
**Issue:** The comment says "eleven cases of a few git and sh processes each". Each case runs about nine `gh` shim invocations, five or more git calls, and several `sed`, `cut` and `grep` calls, roughly 30 process spawns, so about 330 in all. It takes 1.4 s locally, so a 40-times slower Windows shell would sit at the 60 s limit. The comment admits there is no hosted run, which is the same as the neighbouring tests, but those spawn far fewer processes.
**Fix:** Record the first hosted Windows timing and re-derive the limit from it, or start with 120 s. Merging `tracked` and `untracked` into one case would also cut the spawn count.

---

_Reviewed: 2026-11-15T02:21:35Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
