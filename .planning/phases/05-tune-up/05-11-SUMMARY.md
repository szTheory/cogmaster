---
phase: 05-tune-up
plan: "11"
subsystem: ci
tags: [ci-required, rehearsal, dependabot, negative-control]
status: complete

requires:
  - phase: 05-tune-up
    provides: the CI required roll-up and its fixture rows (earlier phases), quality.json and the docs cap (plans 05-05, 05-06), the README check (plan 05-10)
provides:
  - a rehearsal if that skips a pull request whose author is dependabot[bot]
  - a CI required jq clause that accepts a skipped rehearsal and cleanup for that author only, passed as --arg actor
  - needs.txt rows with an actor column, a Dependabot skip row and a Dependabot failure row
  - policy.ci_required.actor_self_test, a control whose widened author clause must fail the person row
affects: [05-12, 05-14]

actuals:
  tokens: 4200
  tasks: 1
  commits: 1

plan_head_before: c0a1fccd68cd41481e4a8c319330cc1efe57a66a
plan_head_after: 388e7ce87ba2836f7b1be0833a4f8171724dba4f
commits: 1

tech-stack:
  added: []
  patterns:
    - "The rehearsal guard and the roll-up clause test the same author, and ci_required.cmake requires the roll-up to take it as --arg actor"
    - "A self test widens one clause with string(REPLACE) and a configure-time FATAL_ERROR when the replacement finds nothing"

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - tests/cmake/ci_required.cmake
    - tests/hygiene/fixtures/needs.txt
    - tests/hygiene/tests.cmake
    - tests/inventory/hygiene.txt
    - docs/testing.md

key-decisions:
  - "The actor control matches the diagnostic up to 'want fail' because CMake wraps the full message across two lines"
  - "ci_required.cmake gained no counted lines: the actor changes extend existing lines, so the check budget stays at 703 of 745"

requirements-completed: []

coverage:
  - id: C1
    description: "A pull request by dependabot[bot] that changes a release input skips the rehearsal, and CI required accepts the skipped rehearsal and cleanup for that author only"
    requirement: TUNE-09
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^policy\\.ci_required' exit 0 with three tests passing; rows: rehearsal skipped for the Dependabot author (pass) and rehearsal skipped when a release input changed with a person author (fail)"
        status: pass
    human_judgment: false
  - id: C2
    description: "A rehearsal that runs for the Dependabot author and fails still fails CI required"
    requirement: TUNE-09
    verification:
      - kind: unit
        ref: "tests/hygiene/fixtures/needs.txt row rehearsal failed for the Dependabot author (fail), run by policy.ci_required exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "A ci.yml copy whose author clause accepts any author makes the person row pass, so the control fails naming that row"
    requirement: TUNE-09
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R '^policy\\.ci_required\\.actor_self_test' exit 0; RED run of policy.ci_required and .self_test exited non-zero before ci.yml gained --arg actor"
        status: pass
    human_judgment: false
  - id: C4
    description: "The workflow still lints, the commit carries the skip, the clause and the control together, and the budgets hold"
    requirement: TUNE-09
    verification:
      - kind: other
        ref: "actionlint -ignore 'label \"ubuntu-26.04' .github/workflows/ci.yml exit 0; git show --stat HEAD exit 0 listing ci.yml, ci_required.cmake, needs.txt and tests/hygiene/tests.cmake; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 at 703 check lines of 7455 src lines; cmake --workflow --preset hygiene exit 0; cmake --workflow --preset ci exit 0"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 11: Rehearsal skip for the Dependabot author Summary

A Dependabot release-input pull request now skips the rehearsal its read-only token cannot run, and `CI required` accepts that skip for `dependabot[bot]` alone, with a control that fails if the clause ever widens to any author.

## Accomplishments

- `ci.yml`: the rehearsal `if:` gains `github.event.pull_request.user.login != 'dependabot[bot]'` with a comment on the read-only token; `CI required` gains `ACTOR`, `--arg actor "$ACTOR"` and `or $actor == "dependabot[bot]"` in its rehearsal and cleanup skip clause, and its comment block names the exception.
- `needs.txt`: every row carries an actor column (`octocat` for the existing 19), plus a Dependabot skip row (pass) and a Dependabot failure row (fail).
- `ci_required.cmake`: pops `actor`, passes `--arg actor`, and requires the argument list to end `--arg release_inputs;--arg actor`.
- `policy.ci_required.actor_self_test`: widens the author clause to `true` on a ci.yml copy and expects exit 1 naming `rehearsal skipped when a release input changed`. Registered in `tests/inventory/hygiene.txt` in sorted position.
- `docs/testing.md`: the roll-up paragraph names the exception in one clause, and the self-test table lists the new control.

## Evidence

- RED: after the fixture and `ci_required.cmake` edits and before the `ci.yml` edit, `ctest --test-dir build/hygiene -R '^policy\.ci_required'` failed 2 of 2 with `ci_required: missing needs or jq -e program and its expected arguments`.
- GREEN: the same command exits 0 with 3 of 3 passing.
- `actionlint -ignore 'label "ubuntu-26.04' .github/workflows/ci.yml` exits 0.
- `cmake --workflow --preset hygiene` exits 0 (44 hygiene tests); `cmake --workflow --preset ci` exits 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exits 0: 703 check lines / 7455 src lines. This plan added no counted lines, so 42 remain for plan 05-12.
- `git show --stat HEAD` exits 0 and lists the six files of commit 388e7ce.
- No hosted run yet: the first Dependabot pull request and its auto-merge are ship-time confirmations in plan 05-14.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Control regex matched a wrapped message**
- **Found during:** Task 1, first GREEN run
- **Issue:** The plan's diagnostic `rehearsal skipped when a release input changed: want fail, got exit 0` is wrapped by CMake across two lines in the error output, so `expect_fail.cmake`'s regex could not match it. The control itself behaved correctly (exit 1, person row named).
- **Fix:** The regex stops at `rehearsal skipped when a release input changed: want fail`, which fits on one line.
- **Files modified:** tests/hygiene/tests.cmake
- **Commit:** 388e7ce

**Total deviations:** 1 auto-fixed (1 test-regex bug). **Impact:** none on behaviour or scope.

## Known Stubs

None.

## Threat Flags

None. The change narrows what a Dependabot pull request runs and widens no token permission; the exception is keyed on the pull request author field, which the control pins to one login.

## Self-Check: PASSED

- FOUND: .github/workflows/ci.yml, tests/cmake/ci_required.cmake, tests/hygiene/fixtures/needs.txt, tests/hygiene/tests.cmake, tests/inventory/hygiene.txt, docs/testing.md
- FOUND: commit 388e7ce
