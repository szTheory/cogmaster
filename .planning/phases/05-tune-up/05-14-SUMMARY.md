---
phase: 05-tune-up
plan: "14"
subsystem: ci
tags:
  - perf
  - gate
  - ci
  - quality
status: complete

requires:
  - phase: 05-tune-up
    provides: two zero-spread control runs on main (05-01), the first hosted run of the phase (05-06), the digest and nightly paths (05-13)
provides:
  - The instructions-per-frame gate armed in ci.yml, in a commit of its own
  - tests/perf/allow.txt with comments only, and the phases 6 to 9 rule
  - Gate wording in docs/perf-fuzz.md, docs/maintaining.md and the follow-up todo
  - quality.json test floors equal to the final inventory counts
  - One proof run in which a real slower pull request failed perf and CI required, and nothing else
  - One green pull_request run on the final phase head, and the ship-time list
affects: [phase 6, phase 7, phase 8, phase 9, ship]

actuals:
  tokens: 850
  tasks: 3
  commits: 2

plan_head_before: 60e0c47b124f0d45cd2ba654daee4b4fde13d7d3
plan_head_after: 73547dfcb1d8199b56a34f6356f1301415530e6c
commits: 2

tech-stack:
  added: []
  patterns:
    - "A gate is proven once on a throwaway draft pull request stacked on the phase branch, because the arming commit is not on main yet"
    - "The final-head run is a pull_request run, the only event on which title, perf, rehearsal and cleanup all run"

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - tests/perf/allow.txt
    - docs/perf-fuzz.md
    - docs/maintaining.md
    - .planning/todos/pending/post-merge-ci-follow-ups.md
    - quality.json

key-decisions:
  - "The arming commit changes ci.yml alone and its subject is ci: arm the instructions-per-frame gate, so the final-run check can find it by subject"
  - "The test floors are set to the line counts of the inventories (422 and 397), not to the counts a macOS ctest run prints (418 locally), because the floors are checked against the inventories"
  - "The proof loop went into src/frame.c, where cogmaster_run_frame lives; the plan named src/instance.c"

requirements-completed:
  - TUNE-02

coverage:
  - id: E1
    description: "ci.yml runs the perf job with COGMASTER_PERF_MODE gate, armed after the two zero-spread control runs recorded in the first plan of the phase"
    requirement: TUNE-02
    verification:
      - kind: unit
        ref: "grep -q 'COGMASTER_PERF_MODE: gate' .github/workflows/ci.yml exit 0; ctest --test-dir build/ci -R '^(perf\\.gate\\.|policy\\.quality$)' 8 of 8 passed; arming commit 950ff2c; control runs https://github.com/szTheory/cogmaster/actions/runs/42752737395 and https://github.com/szTheory/cogmaster/actions/runs/42752770307"
        status: pass
    human_judgment: false
  - id: E2
    description: "tests/perf/allow.txt has no boot_to_play line, only comments, and the comment-only file parses in gate mode"
    requirement: TUNE-02
    verification:
      - kind: unit
        ref: "cmake -DALLOW=tests/perf/allow.txt -DROM=boot_to_play -DHEAD=1000000 -DBASE1=1000000 -DBASE2=1000000 -DMODE=gate -DTODAY=2026-11-10 -P tests/cmake/perf_compare.cmake exit 0 and printed verdict=PASS; grep for a boot_to_play line exit 1"
        status: pass
    human_judgment: false
  - id: E3
    description: "A throwaway draft pull request with a busy loop on the frame path failed perf with verdict FAIL and CI required, with every other job successful, then was closed unmerged with its branch deleted"
    requirement: TUNE-02
    verification:
      - kind: e2e
        ref: "Task 2 verify command exit 0 on run https://github.com/szTheory/cogmaster/actions/runs/42764095086 (perf boot_to_play head=12902843 base=12302839 delta_bp=487 verdict=FAIL); gh pr close 18 --delete-branch exit 0; gh api repos/szTheory/cogmaster/branches/perf-proof exit 1 (404)"
        status: pass
    human_judgment: false
  - id: E4
    description: "A hosted pull_request run on the final phase head concluded success with every job successful, title, perf under the armed gate, rehearsal and cleanup included"
    requirement: TUNE-02
    verification:
      - kind: e2e
        ref: "Task 3 verify command exit 0 on run https://github.com/szTheory/cogmaster/actions/runs/42764989102 at head 73547dfcb1d8199b56a34f6356f1301415530e6c (perf boot_to_play head=12302839 base=12302839 delta_bp=0 verdict=PASS)"
        status: pass
    human_judgment: false
  - id: E5
    description: "quality.json test floors equal the inventory line counts of the phase: 422 for tests.ci and 397 for tests.wasm32"
    requirement: TUNE-02
    verification:
      - kind: unit
        ref: "jq -e on quality.json floors against grep -vc '^#' of tests/inventory/ci.txt and wasm32.txt exit 0; policy.quality passed"
        status: pass
    human_judgment: false
  - id: E6
    description: "Docs and the follow-up todo say gate and boot_to_play; none says the gate is report-only or names a testcard control line"
    requirement: TUNE-02
    verification:
      - kind: other
        ref: "grep -rqiE 'report-only|report mode' docs/perf-fuzz.md docs/maintaining.md exit 1; grep -q 'perf control testcard' on the todo exit 1"
        status: pass
    human_judgment: false
  - id: E7
    description: "The ship-time list carries the rule 10 steps and the exact commands that put the two control run URLs into the phase pull request body"
    requirement: TUNE-02
    verification:
      - kind: other
        ref: "the Ship-time confirmations section of this file, items 0 and 1"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 14: Arm the perf gate Summary

The instructions-per-frame gate is armed in one commit, a real slower pull request failed it once with every other job green, and the final phase head passed a hosted pull_request run with the gate on.

## Commits

| Task | Commit | Subject |
|---|---|---|
| 1 | 950ff2c | ci: arm the instructions-per-frame gate (ci.yml alone; comment cites control runs 42752737395 and 42752770307) |
| 1 | 73547df | ci(05-14): delete the stale allow line, fix gate wording, tighten floors |
| 2, 3 | none on the phase branch | the throwaway commit lived on `perf-proof` only and never reached the phase branch |

## Evidence

Baselines before editing: `grep -c 'COGMASTER_PERF_MODE: report' .github/workflows/ci.yml` printed 1 and `grep -c '^boot_to_play ' tests/perf/allow.txt` printed 1.

| Command | Exit status |
|---|---|
| Task 1 verify (greps, `cmake --preset ci`, `ctest -R '^(perf\.gate\.|policy\.quality$)'`) | 0 (8 of 8) |
| `git log -1 --format=%s -- .github/workflows/ci.yml` | `ci: arm the instructions-per-frame gate` |
| `cmake -DALLOW=tests/perf/allow.txt -DROM=boot_to_play ... -DMODE=gate ... -P tests/cmake/perf_compare.cmake` | 0 |
| `jq -e` floors against inventory counts | 0 |
| `git push origin phase/05-tune-up` | 0 |
| `gh pr create --draft --base main --head perf-proof` | 0 (pull request 18) |
| Task 2 verify (FAIL line, only `perf` and `CI required` not successful, title, rehearsal and cleanup present) | 0 |
| `git push --force origin phase/05-tune-up:perf-proof` | 0 |
| Task 3 verify (event, conclusion, jobs, arming commit ancestor, no non-planning diff, pull request closed unmerged) | 0 |
| `gh pr close 18 --delete-branch` | 0 |
| `gh api repos/szTheory/cogmaster/branches/perf-proof` | 1 (404) |
| `git ls-remote --heads origin` filtered for perf-proof (count) | 0 |
| rehearsal prereleases on the repository (`gh api repos/szTheory/cogmaster/releases`, tag prefix rehearsal-) | 0 listed |
| `cmake --workflow --preset ci` | 0 (418 tests passed on macOS) |
| `cmake --workflow --preset hygiene` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 |
| every `docs/*.md` under 38,912 bytes | 0 |

## Proof run

The proof pull request is number 18, stacked on the phase head with a volatile counter run 100,000 times in `cogmaster_run_frame`. Run https://github.com/szTheory/cogmaster/actions/runs/42764095086 at head `86d139a2b66180721cebfe5fe9f2236c6a85a388`:

```text
perf boot_to_play head=12902843 base=12302839 delta_bp=487 verdict=FAIL
```

Exactly `perf` and `CI required` were not successful. title, hygiene, changes, asan, nofp (two lanes), be32, build (six legs), wasm32, hash-equality, retroarch, rehearsal and cleanup all concluded success. The loop never reached the phase branch (the branch was deleted with the pull request).

## Final phase head run

- Run: https://github.com/szTheory/cogmaster/actions/runs/42764989102
- Event pull_request, conclusion success, head SHA `73547dfcb1d8199b56a34f6356f1301415530e6c`, built by pushing the phase head to `perf-proof` and force-updating the draft pull request 18, which was then closed unmerged.
- Every job concluded success: changes, hygiene, title, build (six legs), asan, nofp (two lanes), be32, wasm32, hash-equality, retroarch, perf, rehearsal, cleanup and CI required.
- The perf line, armed gate on a clean head: `perf boot_to_play head=12302839 base=12302839 delta_bp=0 verdict=PASS`.
- The arming commit 950ff2c is an ancestor of that head, and `git diff --quiet 73547df HEAD -- . ':(exclude).planning'` exits 0, so only `.planning` files differ from the run's head.
- Branch `perf-proof` answers 404 and no rehearsal prerelease remains.

## Final measures

- `policy.e2e_budget` prints 273 lines under `tests/retroarch`, at most 300 (target 274; the same figure as plan 05-02).
- Repository check lines: 729 of 7,455 `src/` lines, strict one tenth (limit 745).
- quality.json floors: `tests.ci` 422 (was 410), `tests.wasm32` 397 (was 385). A local macOS `ctest` of the ci preset runs 418 tests because some inventory entries do not run on macOS; the floors follow the inventories, which CI checks against its junit files.
- No absolute `ir_per_frame` ceiling was added (deferred by the plan).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The plan names src/instance.c for the frame path**
- **Found during:** Task 2
- **Issue:** `cogmaster_run_frame` is defined in `src/frame.c`; `src/instance.c` does not contain it.
- **Fix:** The throwaway busy loop went into `src/frame.c`, after the input latch and before the bus run. It was clang-format-21 clean and passed hygiene on the pull request.
- **Files modified:** `src/frame.c` on the throwaway branch only
- **Commit:** 86d139a (never on the phase branch)

**Total deviations:** 1 auto-fixed (a wrong file name in the plan). **Impact:** none.

## Known Stubs

None.

## Threat Flags

None. The only trust-boundary change is the gate itself, which now fails a pull request, never a release; the throwaway pull request ran the existing rehearsal, which published and deleted its own prerelease.

## Ship-time confirmations

`/gsd-ship` (or the session after the merge) runs these from the repository root and records each command, its exit
status and the run URL. Items 0 and 1 run when the phase pull request exists, before the merge; the rest need the
phase on main.

0. **Perf control runs in the phase pull request body** (the perf-gate decision: record the control URLs and lines
   "in the PR body and SUMMARY"). After `/gsd-ship` opens the pull request, run:
   `gh pr view phase/05-tune-up --json body --jq .body > "$TMPDIR/pr-body.md"`, then
   `awk '/^## Perf control runs/ {f=1} f && /^## / && !/^## Perf control runs/ {f=0} f' .planning/phases/05-tune-up/05-01-SUMMARY.md >> "$TMPDIR/pr-body.md"`,
   then `gh pr edit phase/05-tune-up --body-file "$TMPDIR/pr-body.md"`. Confirm with
   `gh pr view phase/05-tune-up --json body --jq .body > "$TMPDIR/pr-body-check.md"` and
   `awk '/perf control boot_to_play runs=[0-9]+,[0-9]+,[0-9]+ spread=0/ {c++} END {exit !(c >= 2)}' "$TMPDIR/pr-body-check.md"`
   (exit 0). The pull request body must also keep the attribution lines the ship step adds.
1. **AGENTS rule 10: title, auto-merge, CI on the head.** Give the pull request a Conventional Commit title:
   `gh pr view phase/05-tune-up --json title --jq .title > "$TMPDIR/pr-title.txt"` and
   `grep -Eq '^(feat|fix|docs|ci|test|refactor|perf|build|chore)(\([a-z0-9-]+\))?!?: ' "$TMPDIR/pr-title.txt"`
   (exit 0); if it exits 1, set one with `gh pr edit phase/05-tune-up --title "ci: tune up timeouts, gates, docs and
   review debt"` and re-check. Then, in a shell and never inside GSD command text, run
   `gh pr merge phase/05-tune-up --auto --squash` (exit 0). Confirm a CI run exists on the head commit:
   `gh pr view phase/05-tune-up --json headRefOid --jq .headRefOid > "$TMPDIR/head.txt"`, then
   `gh run list --workflow ci.yml --commit "$(cat "$TMPDIR/head.txt")" --json databaseId,url > "$TMPDIR/head-runs.json"`
   and `jq -e 'length > 0' "$TMPDIR/head-runs.json"` (exit 0). Record the run URL.
2. **Release pull request green after the merge (AGENTS rule 10; every phase ends in a release, never held).** After
   the phase merges, `gh pr list --label 'autorelease: pending' --json number,url,statusCheckRollup > "$TMPDIR/release-pr.json"`;
   confirm one release pull request is listed and no check in its rollup is red or pending
   (`jq -e 'length == 1 and (.[0].statusCheckRollup | all((.conclusion // .state) | IN("SUCCESS", "SKIPPED", "NEUTRAL")))' "$TMPDIR/release-pr.json"`,
   exit 0), then let it merge. Afterwards `gh pr list --label 'autorelease: pending' --json number --jq length`
   prints 0, and `gh run list --workflow release.yml --branch main --limit 1 --json conclusion,url` shows success.
3. **Weekly digest dry run.** `gh workflow run digest.yml --ref main -f dry_run=true`. Then
   `gh run list --workflow digest.yml --limit 1 --json databaseId,url` and `gh run watch <id> --exit-status`
   (exit 0). The job summary holds the digest. Record that run ID beside digest.yml's floor-only timeout comment in
   the next tune-up, or in a `ci:` follow-up if the first run shows the floor is wrong.
4. **Dependabot first pull request.** When `gh pr list --author app/dependabot --json number,title` lists one, check
   `gh run list --workflow dependabot-automerge.yml --limit 1 --json conclusion,url` (success) and
   `gh pr view <n> --json autoMergeRequest` (not null for the `minor-and-patch` group or a non-major update, null
   for a major). If the merge fails for lack of workflow-file permission, add `permission-workflows: write` to the
   token step in a `ci:` pull request. If the App client ID variable is not visible to Dependabot-triggered runs,
   the named failure says so; then store the client ID as a Dependabot secret, which is an owner action under rule
   6, and say why.
5. **First auto-merged release-input bump.** A Dependabot pull request that bumps an action pin in ci.yml or
   release.yml changes a release input, skips its rehearsal (plan 05-11) and can auto-merge, so the release path
   first meets the new action on main. After the first such merge, run `gh run list --workflow release.yml
   --branch main --limit 1 --json conclusion,url` and confirm the release pull request it feeds is green
   (`gh pr list --label 'autorelease: pending' --json number,statusCheckRollup`). If the release path fails, revert
   the bump in a `ci:` pull request and record the run URL.
6. **The wasm32 lane required on main.** Run `gh run list --workflow ci.yml --branch main --event push --limit 1
   --json databaseId,url` for the merge commit and confirm `hash-equality` and `wasm32` succeeded
   (`gh run view <id> --json jobs`).
7. **Armed gate on a later pull request.** If Tasks 2 and 3 could not run before the merge, open the same throwaway
   draft pull request against main, confirm `verdict=FAIL` with every other job green, and close it with
   `--delete-branch`. Tasks 2 and 3 did run before the merge (runs 42764095086 and 42764989102), so this item needs no action.
8. **First runs of the new workflows' timeouts.** After the first `digest.yml` and `dependabot-automerge.yml` runs,
   `sh scripts/ci-durations.sh digest.yml` and `sh scripts/ci-durations.sh dependabot-automerge.yml` report their
   durations. The floor-only comments stay until the next tune-up measures 40 runs, or all of them if fewer.
9. **Nightly after the merge.** Confirm the first scheduled nightly run after the merge succeeded
   (`gh run list --workflow nightly.yml --event schedule --limit 1 --json conclusion,headSha,url`) and that its head
   contains the merge commit (`git merge-base --is-ancestor <merge sha> <headSha>` exits 0).

## Phase notes

- Third-party SN76489 and YM2413 sound ROM conformance (the PROJECT.md debt item "deferred with no lane") is not addressed by this phase and stays deferred, recorded in SEED-007 by plan 05-09.
- P1 WR-02 and P1 IN-04 are tooling findings deferred into SEED-007 under AGENTS rule 7, not fixed (plan 05-09).
- P4 WR-02 is recorded as fixed per 04-REVIEW-DISPOSITION.md and 04-REVIEW-FIX.md, a planner decision (plan 05-09).
- The be32 C++ trim was settled by static evidence (the two greps in plan 05-03's SUMMARY showing every configure enables C++), replacing the configure trial the CONTEXT speed decision described; the trim was not taken.
- The README check runs against the built `boot_to_play.sms` fixture instead of a manifest fixture (plan 05-10).
- The measured `policy.e2e_budget` count after plan 05-02 is 273 lines, copied from its SUMMARY and re-measured here, against the 274 target.
- The sound-code nightly evidence is the plain post-merge dispatch on main with its ancestry test against 8faf841 (plan 05-13), not a scheduled run.

## Self-Check: PASSED

- `.github/workflows/ci.yml`, `tests/perf/allow.txt`, `docs/perf-fuzz.md`, `docs/maintaining.md`, `.planning/todos/pending/post-merge-ci-follow-ups.md` and `quality.json` exist; commits 950ff2c and 73547df are ancestors of HEAD; run URLs above answered by `gh run view` (exit 0).
