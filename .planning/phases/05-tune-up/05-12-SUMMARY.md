---
phase: 05-tune-up
plan: "12"
subsystem: ci
tags:
  - dependabot
  - auto-merge
  - policy
  - github-actions
status: complete

requires:
  - phase: 05-tune-up
    provides: the floor-only timeout comment form (05-03), the policy and inventory conventions (05-05, 05-06), the Dependabot author clause in CI required (05-11)
provides:
  - .github/dependabot.yml with weekly github-actions updates and the minor-and-patch group
  - .github/workflows/dependabot-automerge.yml, a pull_request-only workflow that enables squash auto-merge for the group or a same-major update
  - policy.dependabot and policy.dependabot.self_test in the ci preset
affects: [05-14]

actuals:
  tokens: 6500
  tasks: 1
  commits: 1

plan_head_before: 286f82b83bf1beb0ba089f2bdcc645ed5a48b65f
plan_head_after: 16fdbc7273bc78470fd07493b2f20aa1efd94ddd
commits: 1

tech-stack:
  added: []
  patterns:
    - "One awk case scans two files and prints dependabot: file: what for each fault; END-block faults print after per-line faults, so a self test regex follows that order"
    - "A scan strips comments before matching, so a workflow comment may name a forbidden trigger without tripping the check"

key-files:
  created:
    - .github/dependabot.yml
    - .github/workflows/dependabot-automerge.yml
  modified:
    - tests/cmake/repository.sh
    - tests/policy/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/maintaining.md

key-decisions:
  - "The auto-merge title is read with gh into a file under RUNNER_TEMP and parsed by sed; it is never expanded into the script text"
  - "A single update merges only when both versions have at least one dot and share a major, so a SHA-only title, even an all-digit one, never merges"
  - "repository.cmake is untouched: the forwarded DIR key already carries what the check needs"
  - "The check added 16 counted lines (703 to 719 of 745), leaving 26 for the digest check in plan 05-13"

requirements-completed:
  - TUNE-09

coverage:
  - id: C1
    description: "Dependabot updates GitHub Actions weekly with minor and patch in one group named minor-and-patch, ci prefix, 7-day cooldown, and no major in the group"
    requirement: TUNE-09
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.dependabot$' exit 0 over the real .github files"
        status: pass
    human_judgment: false
  - id: C2
    description: "The auto-merge workflow runs on pull_request only, never checks out code, acts only for dependabot[bot], and its title match contains the exact text the minor-and-patch group"
    requirement: TUNE-09
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.dependabot$' exit 0; grep -Eq 'pull_request_target|workflow_run|actions/checkout' on the workflow exits 1"
        status: pass
    human_judgment: false
  - id: C3
    description: "A configuration whose group is named all-actions, lists major and runs daily, and a workflow using pull_request_target and a checkout with a mismatched group name, each fail with named messages"
    requirement: TUNE-09
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.dependabot\\.self_test$' exit 0; RED run of both tests exited non-zero with unknown repository check before the case existed"
        status: pass
    human_judgment: false
  - id: C4
    description: "The new workflow lints, its floor-only timeout comment is accepted, the budgets hold, and both preset workflows pass"
    requirement: TUNE-09
    verification:
      - kind: other
        ref: "actionlint -ignore 'label \"ubuntu-26.04' .github/workflows/dependabot-automerge.yml exit 0; cmake -DDIR=.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 at 719 check lines of 7455 src lines; cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 12: Dependabot for Actions and its guarded auto-merge Summary

Weekly Dependabot updates for pinned Actions with a `minor-and-patch` group, an auto-merge workflow that cannot run untrusted code with the App token, and a ci-preset policy test that fails a renamed group, a group holding majors, or a privileged trigger or checkout.

## Accomplishments

- `.github/dependabot.yml`: one `github-actions` entry, `weekly`, prefix `ci`, `cooldown.default-days: 7`, one group `minor-and-patch` with `minor` and `patch` only, so majors stay separate pull requests.
- `.github/workflows/dependabot-automerge.yml`: `on: pull_request`, `contents: read`, no checkout, one job guarded by `dependabot[bot]`, `timeout-minutes: 5` with the floor-only comment `5-minute floor; no runs yet`. It fails with a named message if `COGMASTER_APP_CLIENT_ID` is empty, mints a token with the pinned `create-github-app-token` (`permission-pull-requests: write`, `permission-contents: write`) and runs `gh pr merge --auto --squash` when the title contains `the minor-and-patch group` or reads `from X to Y` with equal majors. Anything else logs why and exits 0 without merging.
- `tests/cmake/repository.sh`: a `dependabot)` case (16 lines) over both files. It prints `dependabot: <file>: <what>` for a non-Actions ecosystem, a non-weekly interval, a non-`ci` prefix, a cooldown other than 7, a group not named `minor-and-patch` or lacking minor and patch, `major` in the group, `pull_request_target` or `workflow_run`, `actions/checkout`, a missing `the minor-and-patch group` title match and a guard that does not name `dependabot[bot]`. Comments are stripped before matching.
- `policy.dependabot` (real files, DIR is `.github`) and `policy.dependabot.self_test` (a `file(WRITE)` fixture tree holding both bad files) in `tests/policy/tests.cmake`, listed with label `policy` in `ci.txt` and `wasm32.txt` in sorted position.
- `docs/maintaining.md`: a short Dependabot section.

## Evidence

- RED: after the tests and inventory rows and before the `dependabot)` case and the two files, `ctest --test-dir build/ci -R '^policy\.dependabot'` failed 2 of 2.
- GREEN: the same command exits 0 with 2 of 2 passing.
- `cmake -DDIR=.github/workflows -DCHECK=timeouts -P tests/cmake/repository.cmake` exits 0 (floor-only form accepted).
- `actionlint -ignore 'label "ubuntu-26.04' .github/workflows/dependabot-automerge.yml` exits 0.
- `grep -Eq 'pull_request_target|workflow_run|actions/checkout' .github/workflows/dependabot-automerge.yml` exits 1.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exits 0: 719 check lines / 7455 src lines (was 703; the ceiling is 745). 26 counted lines remain for plan 05-13.
- `cmake --workflow --preset ci` exits 0 and ran both new tests; `cmake --workflow --preset hygiene` exits 0.
- The title parse was run on sample titles: `from 7.0.1 to 7.0.2` gives majors 7 and 7, `from v7.0.1 to v8.0.0` gives 7 and 8, a SHA-only title, the group title and an all-digit SHA pair give no match.
- No hosted run yet: the first Dependabot pull request, its auto-merge and the first-run timeout measurement are ship-time confirmations in plan 05-14.

## Deviations from Plan

None - plan executed exactly as written. `tests/cmake/repository.cmake` needed no new key (DIR is already forwarded) and is untouched. The self test regex lists the messages in the order the check prints them (per-line faults of both files before the end-of-input faults), which is a property of the check rather than a change to the plan.

## Known Stubs

None.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: privileged-token-on-pr | .github/workflows/dependabot-automerge.yml | A new pull_request workflow mints an App token with pull-requests and contents write. It is limited to the Dependabot author, never checks out code, reads the title as data, and policy.dependabot fails any later pull_request_target, workflow_run or checkout. |

## Self-Check: PASSED

- FOUND: .github/dependabot.yml, .github/workflows/dependabot-automerge.yml, tests/cmake/repository.sh, tests/policy/tests.cmake, tests/inventory/ci.txt, tests/inventory/wasm32.txt, docs/maintaining.md
- FOUND: commit 16fdbc7
