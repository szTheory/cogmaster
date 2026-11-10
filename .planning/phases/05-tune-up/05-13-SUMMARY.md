---
phase: 05-tune-up
plan: "13"
subsystem: ci
tags:
  - nightly
  - digest
  - policy
  - github-actions
status: complete

requires:
  - phase: 05-tune-up
    provides: ci-durations summarise (05-03), the floor-only timeout comment form (05-03), the policy and inventory conventions (05-05, 05-06), the Dependabot and release App conventions (05-11, 05-12)
provides:
  - Nightly forced failure and passing dispatch on main, with run URLs and the issue it opened and closed
  - scripts/digest.sh, the digest formatter and a timeouts mode that joins on the ci-durations display name
  - tests/hygiene/fixtures/digest.json with digest.format, digest.timeouts, policy.digest and policy.digest.self_test in the hygiene preset
  - .github/workflows/digest.yml, a Monday digest with a dry_run dispatch and the least privilege the decision allows
  - A Weekly digest section in docs/maintaining.md
affects: [05-14]

actuals:
  tokens: 21000
  tasks: 3
  commits: 3

plan_head_before: 94b6239bc64ba06fd93a7881e39fd245033f07c7
plan_head_after: 7880a4622cb16845c2163def31091abf601212d6
commits: 3

tech-stack:
  added: []
  patterns:
    - "A formatter reads one JSON document from stdin, writes it to a temp file before jq reads it, and reuses scripts/ci-durations.sh summarise for the slowest-job row"
    - "A timeouts mode keys each job by the display name ci-durations prints (job-level name without a trailing matrix suffix, else the YAML key), so a join between the two never misses a job whose key differs from its name"
    - "A permission check tracks block indentation, skips blank lines inside a block, and strips comments before matching"

key-files:
  created:
    - scripts/digest.sh
    - tests/hygiene/fixtures/digest.json
    - .github/workflows/digest.yml
  modified:
    - tests/hygiene/tests.cmake
    - tests/cmake/repository.sh
    - tests/inventory/hygiene.txt
    - tests/policy/repository-checks.txt
    - docs/maintaining.md

key-decisions:
  - "The digest job restates contents read beside its three added permissions, because a job's permissions replace the workflow's; policy.digest accepts contents read in the job block and nothing outside the decision's four"
  - "A red pull request is read from the newest CI run on its head commit through actions/runs?head_sha, never statusCheckRollup, which would need Checks read and Commit statuses read"
  - "The sound-code nightly run is the plain post-merge dispatch on main, not a scheduled night: main's head f69ab03 contains 8faf841"
  - "The digest check added 10 counted lines (719 to 729 of 745), leaving 16"

requirements-completed:
  - TUNE-03
  - TUNE-08

coverage:
  - id: C1
    description: "A forced nightly failure on main opened one issue labelled nightly and the next passing run closed it, each recorded as a run URL"
    requirement: TUNE-08
    verification:
      - kind: e2e
        ref: "gh workflow run nightly.yml --ref main -f force_failure=true, run 42762171858 concluded failure, issue 17 opened; gh workflow run nightly.yml --ref main, run 42762969470 exit 0, issue 17 CLOSED with a last comment naming that run; the plan's Task 1 verify command exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "The nightly lanes ran on the sound code: the plain post-merge dispatch on main has a head that contains the sound merge"
    requirement: TUNE-08
    verification:
      - kind: e2e
        ref: "git merge-base --is-ancestor 8faf841 f69ab03335f84fb51c900e93c9343e209df6751e exit 0 for main's head and for run 42762969470's headSha; event workflow_dispatch, headBranch main"
        status: pass
    human_judgment: false
  - id: C3
    description: "The digest formatter prints the slowest job, a flaky line, open nightly issues and attention items from a fixture, joining a job's timeout margin on the display name"
    requirement: TUNE-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^digest\\.format$' exit 0; the RED run of all three digest tests exited non-zero before scripts/digest.sh existed"
        status: pass
    human_judgment: false
  - id: C4
    description: "The timeouts mode maps CI required, build and a key-only job to their timeouts and has no ci-required key, on a fixture and on the real ci.yml"
    requirement: TUNE-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^digest\\.timeouts$' exit 0; sh scripts/digest.sh timeouts .github/workflows/ci.yml piped to jq -e 'has(\"CI required\") and has(\"build\") and (has(\"ci-required\") | not)' exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The digest workflow's permissions, Monday cron and dry_run input are pinned, and a fixture that widens them fails naming each fault"
    requirement: TUNE-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^policy\\.digest(\\.self_test)?$' exit 0 over the real digest.yml and the widened fixture"
        status: pass
    human_judgment: false
  - id: C6
    description: "digest.yml lints, uses a pinned checkout and the floor-only timeout comment, names statusCheckRollup only in a comment, and the budgets and both preset workflows hold"
    requirement: TUNE-03
    verification:
      - kind: other
        ref: "actionlint -ignore 'label \"ubuntu-26.04' .github/workflows/digest.yml exit 0; ctest -R '^(policy\\.(digest|action_pins)|hygiene\\.timeout_comments)' exit 0; ctest --test-dir build/ci -R '^docs\\.budgets$' exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 at 729 check lines of 7455 src lines; cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 13: Nightly issue path and the weekly digest Summary

The nightly issue path is proven on main with run URLs (forced failure opened issue 17, a passing dispatch closed it, on a head that contains the sound merge), and the weekly digest exists as a tested formatter, a policy-pinned workflow and a maintainer doc section.

## Performance

- **Duration:** about 17 min of work plus run waits
- **Completed:** 2026-11-10
- **Tasks:** 3 of 3
- **Files:** 3 created, 5 modified

## Task 1: nightly evidence (no commit)

All commands run from the repository root.

| Step | Command | Exit status |
|------|---------|-------------|
| Precondition | `git show origin/main:.github/workflows/nightly.yml \| grep -q force_failure` | 0 |
| Main head | `gh api repos/szTheory/cogmaster/commits/main --jq .sha` gave `f69ab03335f84fb51c900e93c9343e209df6751e` | 0 |
| Ancestry before dispatch | `git merge-base --is-ancestor 8faf841 f69ab03335f84fb51c900e93c9343e209df6751e` | 0 |
| Label | `gh label list --search nightly` listed `nightly` | 0 |
| Forced failure | `gh workflow run nightly.yml --ref main -f force_failure=true` | 0 |
| Wait | `gh run watch 42762171858`; the run concluded `failure` (bench-wasm32 failed after its upload, report succeeded) | 0 |
| Issue open | `gh issue list --label nightly --state open --json number,url` gave issue 17 | 0 |
| Plain dispatch | `gh workflow run nightly.yml --ref main` | 0 |
| Wait | `gh run watch 42762969470 --exit-status` | 0 |
| Issue closed | `gh issue view 17 --json state,comments`: state CLOSED, last comment "Passed (workflow_dispatch) at f69ab03...: .../runs/42762969470" | 0 |
| Sound code | `gh run view 42762969470 --json headSha,event,headBranch` gave `f69ab03...`, `workflow_dispatch`, `main`; `git merge-base --is-ancestor 8faf841 f69ab03...` | 0 |
| Task verify | the plan's automated command with `NIGHTLY_ISSUE=17 PASS_RUN=42762969470 FAIL_RUN=42762171858` | 0 |

- Forced run (conclusion failure): https://github.com/szTheory/cogmaster/actions/runs/42762171858
- Passing run, the sound-code run (exit 0): https://github.com/szTheory/cogmaster/actions/runs/42762969470
- Issue: https://github.com/szTheory/cogmaster/issues/17, opened by the forced run, closed by the passing run
- No scheduled run with head dc3bf1e is cited. A scheduled night at head f69ab03 (run 42755378821) had run before the dispatches and was not used.

## Task 2: digest formatter (RED then GREEN)

- RED: `288f6bd` registered `digest.timeouts`, `digest.format` and `policy.digest.self_test` with the fixture; `ctest -R '^(digest\.(format|timeouts)|policy\.digest)'` failed all three (3 tests failed out of 3).
- GREEN: `0c6b4e5` added `scripts/digest.sh` (classified `operation`) and the `digest)` case in `tests/cmake/repository.sh`; the same ctest run exited 0, and so did the real `ci.yml` timeouts check from the plan.
- The fixture has `build` as its slowest job (200 s, run 2), one flaky run (run 3, attempt 2, `build (linux-x64)` failed attempt 1), one open nightly issue, one red and one idle pull request, and `CI required` at 150 s against a 4-minute timeout. `build` at 200 s against 7 minutes is inside the margin, and `digest.format` asserts it is not flagged.

## Task 3: workflow and docs

`7880a46` added `.github/workflows/digest.yml`, registered `policy.digest` on it and added the Weekly digest section (docs/maintaining.md is 17,709 bytes of 38,912). `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` both exit 0.

The digest's first dry run is a ship-time confirmation, not a pending task: GitHub dispatches only a workflow on the default branch, so `gh workflow run digest.yml -f dry_run=true` runs after the merge. Plan 05-14 lists it.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The digest check flagged a blank line inside a permissions block**
- **Found during:** Task 3, the first run of `policy.digest` on the real digest.yml
- **Issue:** a blank line after the workflow-level `permissions:` entries was read as a permission that is not `contents: read`
- **Fix:** both permission branches skip blank lines
- **Files modified:** tests/cmake/repository.sh
- **Commit:** 7880a46

**2. [Rule 2 - Missing critical functionality] The job restates `contents: read`**
- **Found during:** Task 3
- **Issue:** a job's `permissions` replace the workflow's, so a job holding only the three added permissions would lose the contents read its checkout needs on a private fork
- **Fix:** digest.yml's job lists `contents: read` beside the three; `policy.digest` still requires exactly the three and rejects anything else, and accepts `contents: read` in the job block
- **Files modified:** .github/workflows/digest.yml, tests/cmake/repository.sh
- **Commit:** 7880a46

**3. [Rule 3 - Blocking] The fixture slowest-job regex followed the formatter's wording**
- **Found during:** Task 2, GREEN
- **Issue:** the RED test expected `build .*200 s`, and the formatter prints `build, 200 s, run 2`
- **Fix:** the test regex names the formatter's line
- **Files modified:** tests/hygiene/tests.cmake
- **Commit:** 0c6b4e5

**Total deviations:** 3 auto-fixed (1 bug, 1 missing critical, 1 blocking). **Impact:** none on scope.

## Known Stubs

None.

## Threat Flags

None. digest.yml adds no endpoint or token: it reads the repository's own runs, jobs, issues and pull requests, and writes one issue and its comment with `issues: write`.

## Self-Check: PASSED

- FOUND: scripts/digest.sh, tests/hygiene/fixtures/digest.json, .github/workflows/digest.yml
- FOUND commits 288f6bd, 0c6b4e5, 7880a46 (`git merge-base --is-ancestor` exit 0 for each)
- Task 1, Task 2 and Task 3 verify commands and acceptance checks exit 0; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0
