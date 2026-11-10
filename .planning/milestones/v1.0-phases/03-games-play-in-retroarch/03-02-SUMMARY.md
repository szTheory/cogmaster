---
phase: 03-games-play-in-retroarch
plan: "02"
subsystem: repository-policy
tags:
  - cmake
  - posix
  - budgets
  - negative-controls
requires:
  - phase: 03-01
    provides: Production play tracer and its expanded core source baseline
provides:
  - Shared repository policy scanner with all existing gate names retained
  - Complete responsibility classification and strict physical-line accounting
  - Recurring equality, omission, helper and unterminated-line boundary controls
affects:
  - 03-03
  - 03-15
  - 03-16
  - 03-17
tech-stack:
  added: []
  patterns:
    - CMake dispatch to shared POSIX source scans and set comparisons
    - Count complete shared assertion helpers by responsibility
    - Exact inventories and nonzero exit plus tolerant diagnostic negative controls
key-files:
  created:
    - tests/cmake/repository.cmake
    - tests/cmake/repository.sh
    - tests/policy/repository-checks.txt
    - tests/policy/budget_cases.cmake
    - tests/hygiene/fixtures/needs.txt
  modified:
    - tests/cmake/budgets.cmake
    - tests/cmake/ci_required.cmake
    - tests/cmake/expect_fail.cmake
    - cmake/CogmasterTests.cmake
    - scripts/hygiene.sh
    - tests/hygiene/tests.cmake
    - docs/testing.md
    - docs/maintaining.md
key-decisions:
  - Count source-policy, core ABI, JUnit, inventory, assertion and hook helpers in full.
  - Classify packaged-output integration, bootstrap, administration, publication and CI routing individually by actual responsibility.
  - Keep synthetic boundary registration shared between CI and hygiene while retaining exact inventories.
  - Leave GAME-01 and GAME-07 pending until their complete phase-wide behavior is demonstrated.
requirements-completed: []
coverage:
  - id: repository-check-consolidation
    description: Existing source policy, core ABI and metadata gates retain their named fault detection after consolidation
    requirement: GAME-01
    verification:
      - kind: integration
        ref: cmake --workflow --preset ci
        status: pass
      - kind: integration
        ref: cmake --workflow --preset hygiene
        status: pass
    human_judgment: false
  - id: strict-repository-budget
    description: Physical check lines stay strictly below one tenth of core lines and unknown implementations reject
    requirement: GAME-01
    verification:
      - kind: integration
        ref: docs.budgets
        status: pass
      - kind: integration
        ref: docs.budgets.ratio_below
        status: pass
      - kind: integration
        ref: docs.budgets.ratio_equal
        status: pass
      - kind: integration
        ref: docs.budgets.unclassified
        status: pass
      - kind: integration
        ref: docs.budgets.helper
        status: pass
      - kind: integration
        ref: docs.budgets.helper_unclassified
        status: pass
    human_judgment: false
  - id: hosted-helper-accounting
    description: Complete RetroArch helper tree remains within 300 lines and rejects a combined 301-line fixture
    requirement: GAME-07
    verification:
      - kind: integration
        ref: policy.e2e_budget
        status: pass
      - kind: integration
        ref: policy.e2e_budget.self_test
        status: pass
    human_judgment: false
actuals:
  tokens: 27596
  tasks: 2
  commits: 3
plan_head_before: 5cb64a5f8993f30522018f37c9ef3df8f1298e82
plan_head_after: 920f4573c710ef6f5d09ed7dc53037ed787bcd1c
duration: 18min
completed: 2026-10-20
status: complete
---

# Phase 03 Plan 02: Proportionate repository checks Summary

**Shared CMake/POSIX checks preserve the existing negative controls while enforcing 448 counted physical lines against 4,516 core lines and the complete 293-line RetroArch helper budget.**

## Performance

- Started: 2026-10-20T18:24:40Z
- Completed: 2026-10-20T18:42:10Z
- Runtime: Codex
- Model: GPT-6
- Tasks: 2
- Files changed: 37
- Actual tokens: ceiling of realized diff characters divided by four, measured before summary metadata.

## Accomplishments

- Consolidated repeated workflow/source scans, API set comparisons, core ABI parsing, provenance, metadata and JUnit checks into one CMake dispatcher and POSIX scanner. Existing gate and negative-control names remain registered. CI and nightly call the shared JUnit route. Git listing, scan failure and shell exit statuses remain explicit.
- Added an individual responsibility classification of checker, build, operation and behavior files. The numerator includes complete common dispatcher/scanner, CI predicate, ABI, hygiene, exact-inventory, assertion and hook helpers. Every physical line counts, including blanks, comments and an unterminated last line. Unknown applicable implementations reject before the ratio is evaluated; equality rejects without rounding.
- Shared recurring synthetic cases between CI and hygiene prove one line against eleven passes, one against ten fails, an omitted checker fails, a counted compatibility helper increases the numerator, and an omitted parser helper fails. The hosted negative control now totals two files, including a nested helper with no final newline, so the 300-line limit covers the whole tree.
- Documented each counted responsibility, individual exclusion reasons and the faults prevented. No core source was padded, dependency added or protection removed.

## Task Commits

1. Task 1 RED: `937ddf2` — exact-equality regression fails because the original checker returns zero.
2. Task 1 GREEN: `8f3c2b3` — shared checks, classified accounting and omission control.
3. Task 2: `920f457` — recurring boundaries on both lanes, complete hosted helper control and maintenance documentation.

The three commits are measured from the persisted plan ledger. Summary and state metadata commits are separate.

## Verification Evidence

All commands ran from the repository root. Run URLs are not applicable to these local executions; no logs are committed.

| Command | Exit | Result | Run URL |
|---|---|---|---|
| `cmake --workflow --preset ci` | 0 | 197 tests and four archive checks pass; exact inventory enforced | Not applicable, local |
| `cmake --workflow --preset hygiene` | 0 | 29 checks pass, including all six recurring budget cases | Not applicable, local |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | 0 | 448 counted lines / 4,516 core lines, strictly below one tenth | Not applicable, local |
| `cmake -DCHECK=e2e_budget -DDIR=tests/retroarch -P tests/cmake/repository.cmake` | 0 | Complete hosted tree is 293 lines, at most 300 | Not applicable, local |
| `cmake -DCHECK=check_junit -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/repository.cmake` | 0 | Each selected CI case executed once without failure or skip | Not applicable, local |
| `cmake -DCHECK=check_junit -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/repository.cmake` | 0 | Each hygiene case executed once without failure or skip | Not applicable, local |
| `git diff --check` | 0 | No whitespace errors | Not applicable, local |

The task 1 CI command also passed again after its commit as the tracer feedback gate, before task 2 expansion. No required verification was skipped. Optional unavailable cross toolchains were not invoked; their configured hosted lanes remain responsible for platform execution.

## TDD Gate Compliance

Task 1 committed RED before implementation. The actual CTest JUnit report was submitted to `node` invoking `check tdd-red-evidence`; the verdict was `RED_EVIDENCE_OK`. The report and JSON record remain ignored build artifacts.

- RED command: `ctest --test-dir build/ci -R '^docs.budgets.ratio_equal$' --output-on-failure --output-junit tdd-ratio.xml`, exit 8. The target executed the planned `expect_fail` assertion and found that one counted line against ten core lines incorrectly returned zero. This was the intended missing behavior, without a discovery, syntax or fixture failure.
- GREEN: equality and the omitted-checker control reject with the intended exit and diagnostic; all original positive/negative checks pass. Task 2 adds the just-below, counted helper and omitted helper cases to both exact inventories.
- Refactor: source scan sharing and repeated predicate reduction are the implementation; subsequent layout and error propagation cleanup ships with task 2. No separate behavior-free commit was needed. Global TDD mode remains disabled.

## Deviations from Plan

### Auto-fixed Issues

1. **[Rule 3 - Blocking] Expanded consolidation to the actual shared helpers.** The initial conservative classification totaled 1,467 physical lines against a strict maximum of 451. Consolidating only the originally listed small scanners could not satisfy the plan. The implementation also consolidates core ABI, JUnit and CI predicate checks, exact inventory registration, negative-control handling and hygiene scanning. Workflow and runner registrations now invoke the shared dispatcher. Files: `cmake/CogmasterTests.cmake`, `scripts/hygiene.sh`, `tests/cmake/ci_required.cmake`, `tests/cmake/expect_fail.cmake`, shared checker files, runner registrations and CI/nightly workflows. Commits: `8f3c2b3`, `920f457`.
2. **[Rule 2 - Correctness] Preserved fail-closed scan behavior and final-line accounting.** Captured Git enumeration and hosted scan statuses, preserved the negative control's sanitizer override, and retained single-character inventory rows in exact comparison. The synthetic cases explicitly cover unterminated final lines and complete helper totals. Commit: `920f457`.

Fifteen old check implementation files were intentionally removed after their routes moved to the shared scanner: ABI, action pins, API names/version, JUnit, e2e budget, exit table, info claims, no-float, no-skip, notices, pins, single version, timeout comments and vendored identity. Their registered checks remain present. `needs.txt` is the unchanged synthetic input/expected-result table for CI roll-up testing; it contains no checker implementation.

## Accounting Scope

The catalog distinguishes source policy from operations and behavior by actual responsibility. Core ABI, provenance, documentation, workflow policy, JUnit completeness, exact inventories and every shared assertion helper count in full. Bootstrap/build tools, remote administration, publication and the CI router have individual operational reasons. Archive-content and loader-floor scripts exercise packaged deliverable behavior and remain integration tests with their original controls; their code was neither moved nor weakened. Synthetic fixture registration is test construction, with an explicit classification for the new shared boundary fixture file. New scripts and compatibility helpers must be classified; an unknown helper fails automatically.

## Requirement Scope

This plan supplies the declared implementation guardrails for GAME-01 and GAME-07. Rendering expansion and real-host content behavior remain later plans' scope, so neither whole-phase requirement is marked complete. There are no goal-blocking stubs or deferred checks introduced by this plan.

## Self-Check: PASSED

All five created artifacts exist. All three task commits are ancestors of HEAD. The persisted ledger measures three commits between the recorded before/after revisions. CI, hygiene, explicit inventory completeness and both live budgets passed after the final implementation change.
