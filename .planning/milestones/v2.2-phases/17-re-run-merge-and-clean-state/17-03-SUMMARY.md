---
phase: 17-re-run-merge-and-clean-state
plan: 03
subsystem: compatibility-ledger
tags:
  - library-judge
  - ledger
  - worktree
  - stamp-ancestry
  - diagnose

requires:
  - phase: 17-re-run-merge-and-clean-state
    provides: plan 01 left the diagnose prune on the branch and plan 02 the milestone-close script, so the diagnose counts come from the final line shape and CI had a clean sample of both
provides:
  - tests/compat/ledger.csv and docs/compatibility.md carried from a judge run on the phase base, stamped with that base, an ancestor of origin/main
  - 17-RUN-EVIDENCE.md with the pushed head, the library run, the merge checks, the diagnose counts by cause and the local gates (counts, commands, exit statuses and run URLs only)
  - scripts/library.sh reads the main checkout's dotenv from a linked worktree, with a test case
affects:
  - 17-04
  - ship

plan_head_before: 1b8d4466ab72b4ca75e5c026270540a3f70165cc
plan_head_after: 6aa2ebdc6fa773511fc8efa73a853622698e800e
commits: 5

actuals:
  tokens: 92450
  tasks: 4
  commits: 5

tech-stack:
  added: []
  patterns:
    - a judge run on a commit already on main, in a detached worktree, so the ledger stamp is an ancestor of main by construction
    - machine-written files cross from the worktree to the branch only through git apply of a diff written to a file, then cmp

key-files:
  created:
    - .planning/phases/17-re-run-merge-and-clean-state/17-RUN-EVIDENCE.md
  modified:
    - tests/compat/ledger.csv
    - docs/compatibility.md
    - scripts/library.sh
    - tests/library/tests.cmake
    - .planning/STATE.md

key-decisions:
  - "The judge ran once at the phase base 0a97075 in a detached worktree, so the single core stamp on the carried rows is an ancestor of origin/main (exit 0) while the Phase 15 pre-squash stamp stays a non-ancestor (control, exit 1)"
  - "The library script of the branch head was invoked by absolute path from the worktree, so the dotenv fallback of the orchestrator's fix applied and no local settings file was copied into the worktree"
  - "No pull request, no push after the carry commit: plan 04 pushes the head before it measures, and the merge happens at ship"

requirements-completed:
  - RUN-03
  - RUN-04

coverage:
  - id: D1
    description: "the code of plans 01 and 02 was pushed once and a dispatched ci run on that code is green, so the diagnose counts come from the final line shape and CI has a clean sample"
    requirement: RUN-03
    verification:
      - kind: integration
        ref: "gh workflow run ci.yml --ref phase/17-re-run-merge-and-clean-state; first run on e896d84 failed on the two Windows legs (https://github.com/szTheory/cogmaster/actions/runs/43622165959), green on 1b8d446 (https://github.com/szTheory/cogmaster/actions/runs/43657530824) and on d2b8605 (https://github.com/szTheory/cogmaster/actions/runs/43660745598)"
        status: pass
    human_judgment: false
  - id: D2
    description: "the library judge ran once, with the no-argument form, in a detached worktree of the phase base and with the output directory variable unset; the owner authorised the agent to run it on the owner's machine under rule 6 and judged nothing"
    requirement: RUN-03
    verification:
      - kind: other
        ref: "sh scripts/library.sh with no argument in the worktree at 0a97075f34e48e2b1d0f72aa231006cc9366a398, exit 0, about 11 minutes"
        status: pass
    human_judgment: false
  - id: D3
    description: "the run is accepted only on the ratchet-ok merge line and a worktree diff naming exactly the ledger and the page, then carried by git apply and compared byte for byte"
    requirement: RUN-03
    verification:
      - kind: other
        ref: "grep -Eq of the merge line in the worktree summary.txt exit 0; git -C worktree diff --quiet checks exit 0; diff to a scratch file, test -s, git apply exit 0; cmp of both files against the worktree exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "the added ledger lines carry one distinct 40-hex core value equal to the phase base, it is an ancestor of origin/main and reachable, and the Phase 15 pre-squash stamp is not an ancestor"
    requirement: RUN-03
    verification:
      - kind: other
        ref: "git merge-base --is-ancestor of the base against origin/main exit 0; the same command on 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4 exit 1 (control); gh api repos/szTheory/cogmaster/commits/<base> --jq .sha exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "no pass row of the origin/main ledger became another status or disappeared: 87 rows moved from fail to pass and the pass-lost count is 0"
    requirement: RUN-03
    verification:
      - kind: other
        ref: "the Phase 15 awk transition count over the saved origin/main ledger and tests/compat/ledger.csv, no pipe after it, exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "the base branch's own ratchet and comparison accept the new ledger"
    requirement: RUN-03
    verification:
      - kind: integration
        ref: "sh <origin/main copy>/scripts/compat-ratchet.sh <origin/main ledger> tests/compat/ledger.csv tests/compat/allow.txt <date> 19 19 build/ci/tests/compat-fixtures/results.csv exit 0, printed the protected-comparison pass line"
        status: pass
    human_judgment: false
  - id: D7
    description: "the compatibility page equals its regeneration from the ledger and the three compat tests pass"
    requirement: RUN-04
    verification:
      - kind: integration
        ref: "sh scripts/compat-common.sh docs tests/compat/ledger.csv piped to cmp against docs/compatibility.md exit 0; ctest --test-dir build/ci -R '^compat\\.(exit|whole_list\\.fresh|page_bound)$' --no-tests=error exit 0 (3 of 3)"
        status: pass
    human_judgment: false
  - id: D8
    description: "the rows that still fail are counted by cause from the final line shape on the ledger head: 5 failing rows in the first NTSC profile, in three leaves, groups total equal to the fail total"
    requirement: RUN-04
    verification:
      - kind: other
        ref: "sh scripts/library.sh diagnose from the main checkout exit 0; header core 7da9ccc equal to the head, no dirty mark; leaves 2, 2, 1 and 0 rows; groups_total 5, fail_total 5"
        status: pass
    human_judgment: false
  - id: D9
    description: "the linked-worktree case of the library wrapper: a worktree with no dotenv of its own reads the main checkout's, and the case fails without the script change"
    requirement: RUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^library\\.synthetic$' (the linked-worktree case, failing without the scripts/library.sh change, passing with it)"
        status: pass
    human_judgment: false
  - id: D10
    description: "the ci workflow passes on the ledger head, the fence holds, the worktree is removed and no pull request is open"
    requirement: RUN-04
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (738 of 738, then 5 of 5); git diff --quiet origin/main...HEAD -- src include tests/compat/allow.txt quality.json exit 0; git worktree list no longer names the worktree; gh pr list --head phase/17-re-run-merge-and-clean-state --state open returns 0"
        status: pass
    human_judgment: false

duration: 70min
completed: 2026-11-15
status: complete
---

# Phase 17 Plan 03: Re-run on main, carry and diagnose Summary

**The library judge ran once in a detached worktree of the phase base, its ratchet-ok result was carried by `git apply` into one commit, and the ledger and page now stamp `0a97075`, an ancestor of `origin/main`: 87 rows moved from fail to pass (fail 96 to 9, pass 1029 to 1116), no pass row was lost, and the diagnose on the ledger head counts the 5 failing NTSC rows by cause.**

## Performance

- **Duration:** about 70 min (first task commit 2026-11-14T19:29 -05:00, last 20:36 -05:00), of which the judge took about 11 min and the diagnose between 20 and 25 min
- **Tasks:** 4 (Task 1 tracer, Task 2 the owner session, Tasks 3 and 4 auto)
- **Commits:** 5 measured from the plan base (`1b8d446`) to `6aa2ebd`
- **Files modified:** 6 (the ledger, the page, `scripts/library.sh`, `tests/library/tests.cmake`, the evidence note, STATE.md). The ledger and page are machine-written and account for about 89k of the 92k tokens in `actuals.tokens`; the files written by hand come to about 3.5k tokens.

## Accomplishments

- **Push and a clean CI sample.** The code of plans 01 and 02 was pushed once and `ci` dispatched. The first run failed on both Windows legs in plan 02's script (see Deviations), the fix `1b8d446` turned it green, and the run on `d2b8605` is green too.
- **Judge, accepted by counts.** `merge: rows_added=0 ratchet=ok page_bytes=10261`, a two-file worktree diff, and a clean `exit:` verdict (`rows=1986 pass=1116 fail=9 unknown=832 unsupported=5 skipped=24 infrastructure_error=0`, judged-cart floors met, experimental flag absent). The library status counts that go to standard output only are recorded as not reported.
- **Carry.** The diff went through a scratch file, `test -s`, `git apply`, `cmp` against the worktree, one commit (`7da9ccc`, `fix(compat): publish the library results judged on main`). 1116 ledger lines changed: 1107 pass and 9 fail rows restamped with the judged commit; 87 of them are fail-to-pass flips (42 PAL, 43 NTSC without FM, 2 NTSC with FM).
- **Stamp.** One distinct 40-hex value on the added lines, equal to the phase base `0a97075f34e48e2b1d0f72aa231006cc9366a398`; `is-ancestor` against `origin/main` exits 0, the Phase 15 pre-squash stamp exits 1 as the control, and `gh api` answers for the base.
- **Ratchet and page.** The transition count exits 0 with 0 pass rows lost, the `origin/main` copies of the ratchet and comparison scripts accept the new ledger (revisions 19 and 19), the page equals its regeneration byte for byte, and `compat.exit`, `compat.whole_list.fresh` and `compat.page_bound` pass.
- **Diagnose.** Core `7da9ccc` equals the head, behaviour 19, exit 0. Leaves: `dark_no_fault.never_draws` 2, `static_no_fault.frozen` 2, `fault_alive.unsupported_video_mode.vdp` 1, `input_mismatch` 0. Totals: judged 573, run_fail 5, ledger_fail 5, matched 5, ledger_fail_not_in_run 0, ledger_fail_now_pass 0, run_fail_not_in_ledger 0, groups_total 5, fail_total 5. The 4 other fail rows of the ledger are PAL rows, which diagnose does not run.
- **Clean up.** The worktree is removed (`git worktree remove --force`, then `prune`), the scratch outputs deleted, `cmake --workflow --preset ci` passes 738 of 738 (then 5 of 5), the fence on `src`, `include`, `allow.txt` and `quality.json` holds, and no pull request is open.

## Task Commits

1. **Task 1 (tracer): push, green dispatched run, worktree** - `2d75ac8` (docs), with the fix `1b8d446` (fix, attributed to plan 02) and the state note `ade9957` (docs)
2. **Task 2 (owner session): the owner authorised the agent to run the library commands** - no commit of its own; the orchestrator's fix `d2b8605` (fix) came out of it
3. **Task 3: judge, carry, checks** - `7da9ccc` (fix)
4. **Task 4: diagnose counts, local gates, Runtime Log row** - `6aa2ebd` (docs)

**Plan metadata:** the commit that carries this file (docs: complete plan).

`plan_head_before` is `1b8d446`, the commit before this plan's first task commit; the ledger file for the measurement did not exist because the first task ran in an earlier session, so it was written afterwards from that parent.

## Commands and exit statuses

The full command and exit-status tables are in `17-RUN-EVIDENCE.md` (Pushed head, Merge checks, Diagnose counts, Local gates). All of them ran from the repository root except the judge, whose one child shell changes directory (the stated working-directory exception, because `scripts/library.sh` takes its root from the working directory).

| Command | Exit |
|---|---|
| `sh scripts/library.sh` with no argument in the worktree at the base, `COGMASTER_LIBRARY_OUT` unset | 0 |
| merge-line grep and two-file diff check on the worktree | 0, 0 |
| carry (`git diff` to a file, `test -s`, `git apply`) and the two `cmp` checks | 0, 0 |
| one stamp equal to the base; `is-ancestor` on the base; `is-ancestor` on the Phase 15 stamp; `gh api` on the base | 0, 0, 1 (control), 0 |
| transition count awk; page `cmp`; compat ctest (3 of 3) | 0, 0, 0 |
| old base judge (`compat-ratchet.sh` from `origin/main`) | 0 |
| fence on `allow.txt`, `quality.json`, `src` and `include` against `origin/main` | 0 |
| Task 3 automated verify on the ledger commit | 0 |
| `sh scripts/library.sh diagnose` from the main checkout, `COGMASTER_LIBRARY_OUT` unset | 0 |
| `git worktree remove --force`, then `git worktree prune` | 0, 0 |
| `cmake --workflow --preset ci` | 0 |
| fence `origin/main...HEAD` on `src include tests/compat/allow.txt quality.json` | 0 |
| `gh pr list --head phase/17-re-run-merge-and-clean-state --state open` count | 0 |

## Decisions Made

- Followed the plan's shape. Nothing was added for stamp ancestry: no test, gate, script or workflow line; the recorded `is-ancestor` commands and their control are the check, and the diff from the phase base to the ledger commit shows no change under `tests/compat` other than the ledger, to the two compat scripts or under `.github`.
- The three scratch files of the transition count and the old base judge went to the session scratch directory instead of `${TMPDIR:-/tmp}`; both are outside the tree and equivalent.
- The `library:` standard-output counts of the judge are `not reported`; nothing else in the plan depends on them.
- Did not push after the carry commit. Task 4 says plan 04 pushes this head before measuring, and the merge belongs to ship.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The library script found no library settings in a linked worktree (orchestrator, owner-authorised)**
- **Found during:** Task 2 and Task 3, the judge run
- **Issue:** the judge runs in a detached worktree of the phase base, which has no local settings file of its own, and `scripts/library.sh` reads the library setting only from the root of the tree it runs in, so the wrapper found no library there. The plan said an export from the owner's terminal would carry it; the owner instead authorised the agent to run the library commands, and the session had no such export.
- **Fix:** commit `d2b8605` `fix(library): read the main checkout's dotenv from a linked worktree`: when the working tree has no dotenv, the script falls back to the parent of the common git directory, and `tests/library/tests.cmake` gained a linked-worktree case. The case was shown failing with the script change removed and passing with it; `sh scripts/hygiene.sh --tree` exits 0; the dispatched `ci` run on that head is green (https://github.com/szTheory/cogmaster/actions/runs/43660745598). The judge then ran the branch's script by absolute path from the worktree, so the fallback applied without copying any settings file into the worktree. The diagnose half runs from the main checkout, where the script already found the value.
- **Files modified:** `scripts/library.sh`, `tests/library/tests.cmake`
- **Commit:** `d2b8605`

**2. [Rule 1 - Bug] Plan 02's script failed on both Windows legs (found by the first dispatched run)**
- **Found during:** Task 1, the first dispatched `ci` run on `e896d84` (https://github.com/szTheory/cogmaster/actions/runs/43622165959)
- **Issue:** `scripts/milestone-close.sh` read the release manifest with `git show origin/main:<path>`, and the Windows shell rewrote the revision path, so the cases `baseline`, `release_tag` and `pending` of `policy.milestone_close` answered `unknown` (exit 2). The fault was in plan 02's script, not in the diagnose prune.
- **Fix:** commit `1b8d446` `fix(17-02): read the release manifest without a Windows shell rewriting the revision path` passes `MSYS_NO_PATHCONV=1` and `MSYS2_ARG_CONV_EXCL='*'` to that one call; `ctest --preset ci -R '^policy\.milestone_close$'` exits 0 locally and the dispatched run on `1b8d446` is green (https://github.com/szTheory/cogmaster/actions/runs/43657530824).
- **Files modified:** `scripts/milestone-close.sh`
- **Commit:** `1b8d446`

### Variances

**3. [Variance] The owner session was an authorisation, not an export or a fallback run**
- Task 2 offered an export or the owner's own terminal. The owner authorised the agent to run both commands; the agent never read, listed, named or printed the library directory or the local settings file, a hook enforced that, and from the judge it opened only `summary.txt` and from diagnose only the header, the leaf group lines and the totals line.

**4. [Variance] The plan's commit ledger did not exist at SUMMARY time**
- Task 1 ran in an earlier session without writing the per-plan ledger, so `plan_head_before` was set to the parent of this plan's first commit afterwards (see Task Commits).

**5. [Variance] `rows_added` is 0 although the ledger changed**
- The merge adds no new rows; every change is a re-stamp of a judged row or a status move. The acceptance pattern allows any integer, and the line was accepted as written.

---

**Total deviations:** 2 auto-fixed (one blocking, one bug), 3 variances noted
**Impact on plan:** none on the result; the dotenv fix is a small permanent improvement for any worktree run.

## Issues Encountered

- The first dispatched `ci` run failed on the Windows legs (Deviation 2); one fix and a re-dispatch turned it green.
- The diagnose took longer than the Phase 14 figure the plan quoted (20 to 25 minutes against 13 to 17), which stays inside the bounded-session limit. It is a data point for the timeouts of plan 04 only if a timeout is derived from diagnose, which none is.

## Known Stubs

None. The ledger and page are machine-written data, not placeholders.

## Threat Flags

None. This plan adds no network endpoint, auth path or file access beyond the existing library wrapper. T-17-06: the agent opened only `summary.txt` and the diagnose counts lines, the evidence note passes the plan's leak grep, and the worktree whose results held the owner's path is removed. T-17-07: the ledger and page were written only by the script after the ratchet, carried by `git apply` and compared with `cmp`; the allow list, `quality.json`, `src` and `include` are unchanged against `origin/main`. T-17-08: one stamp equal to the base, an ancestor of `origin/main`, the control stamp is not, and the base is reachable by `gh api`.

## For the owner

Nothing is needed from you for this plan. The owner session is finished. The ledger and page are committed on `phase/17-re-run-merge-and-clean-state` and not yet pushed past `d2b8605`; plan 04 pushes the head, re-derives the timeouts and hands the branch to `/gsd-ship`, which opens the pull request and merges the stamped ledger through the unchanged ratchet.

## Next Phase Readiness

- Plan 04 can start: the branch head is `6aa2ebd` plus this plan's metadata commit, the tree outside `.planning/` is clean, no pull request is open, and `17-RUN-EVIDENCE.md` is ready for its `## Timeouts` and `## Ship hand-off` sections.
- RUN-03 and RUN-04 hold on this plan's criteria; the pull-request merge itself happens at ship.

## Self-Check: PASSED

- Files found: `tests/compat/ledger.csv`, `docs/compatibility.md`, `scripts/library.sh`, `tests/library/tests.cmake`, `.planning/phases/17-re-run-merge-and-clean-state/17-RUN-EVIDENCE.md`.
- Commits found as ancestors of HEAD: `2d75ac8`, `ade9957`, `d2b8605`, `7da9ccc`, `6aa2ebd`; also `1b8d446`, the fix to plan 02's script.

---
*Phase: 17-re-run-merge-and-clean-state*
*Completed: 2026-11-15*
