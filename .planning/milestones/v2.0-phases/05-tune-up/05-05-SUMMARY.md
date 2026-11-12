---
phase: 05-tune-up
plan: "05"
subsystem: ci
tags: [quality, budgets, lessons, policy, tests]
status: complete

requires:
  - phase: 05-tune-up
    provides: the tune-up CI baseline (plans 05-01 and 05-03)
provides:
  - quality.json with a docs ceiling, two test-count floors and a reasons list
  - tests/cmake/quality.cmake, a string(JSON) checker with an extensible MEASURED argument
  - one docs cap, read by budgets.cmake from quality.json
  - policy.lessons_unique in the ci and hygiene lanes
affects: [05-06, 05-14]

actuals:
  tokens: 3657
  tasks: 2
  commits: 2

plan_head_before: b144e9fe65ef89eae7e3bca25fd523936ea4cc67
plan_head_after: bf31c18693cf6499b2cf4d52dcfab0dcfff60d3e
commits: 2

tech-stack:
  added: []
  patterns:
    - "Limits live in one flat quality.json; a measured key is named on the command line as an integer, lines:<path> or bytes:<path>"
    - "Negative-control regexes tolerate CMake's wrapping of FATAL_ERROR text with [ \\n]+ between words"

key-files:
  created:
    - quality.json
    - tests/cmake/quality.cmake
  modified:
    - tests/cmake/budgets.cmake
    - tests/cmake/repository.sh
    - tests/policy/budget_cases.cmake
    - tests/policy/tests.cmake
    - tests/policy/repository-checks.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt

key-decisions:
  - "The floors are 410 for tests.ci and 385 for tests.wasm32, the inventories' non-comment line counts after task 1's own additions; plan 05-14 sets the final values"
  - "A docs.budgets.no_quality control was added beside the planned controls, because budgets.cmake now has a failure path for a missing quality.json that nothing else exercised"

requirements-completed: []

coverage:
  - id: Q1
    description: "quality.json holds docs.max_bytes 38912, tests.ci and tests.wasm32 floors and a reasons list, and quality.cmake enforces the floors against the inventories' line counts"
    requirement: TUNE-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.quality$' exit 0; cmake -DQUALITY=quality.json with -DMEASURED=tests.ci=lines:tests/inventory/ci.txt -P tests/cmake/quality.cmake exit 0"
        status: pass
    human_judgment: false
  - id: Q2
    description: "Each quality negative control (ceiling one below, floor one above, missing key, malformed file) exits 1 naming the key or file"
    requirement: TUNE-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.quality\\.' exit 0, 4 of 4 tests; RED run before quality.cmake existed failed all 7 new quality and budget controls"
        status: pass
    human_judgment: false
  - id: Q3
    description: "budgets.cmake reads the docs cap from quality.json, so there is one cap, and fails naming the doc when over it and naming quality.json when it is missing"
    requirement: TUNE-04
    verification:
      - kind: unit
        ref: "ctest -R '^docs\\.budgets' exit 0 in build/ci (13 tests with the quality ones) and build/hygiene (8 tests); grep -q 40960 tests/cmake/budgets.cmake exit 1"
        status: pass
    human_judgment: false
  - id: Q4
    description: "policy.lessons_unique passes on the real ledger (79 IDs), fails naming a repeated ID, and fails on zero IDs, in both the ci and hygiene lanes"
    requirement: TUNE-04
    verification:
      - kind: unit
        ref: "ctest -R '^policy\\.lessons_unique' exit 0 with 3 tests in build/ci and in build/hygiene; cmake -DROOT=$PWD -DDOC=.planning/LESSONS.md -DCHECK=lessons_unique -P tests/cmake/repository.cmake exit 0; RED run failed 3 of 3 before the case existed"
        status: pass
    human_judgment: false
  - id: Q5
    description: "The check does not cite a lesson ID in its source, and the check-line ratio stays under one tenth of src"
    requirement: TUNE-04
    verification:
      - kind: other
        ref: "grep -q 'XFER-[0-9]' tests/cmake/repository.sh exit 1; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 printing 595 check lines / 7449 src lines"
        status: pass
    human_judgment: false
  - id: Q6
    description: "The repository gates stay green with the new tests registered in the exact inventories"
    requirement: TUNE-04
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (409 tests); cmake --workflow --preset hygiene exit 0 (43 tests)"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 05: quality.json, its checker and lessons_unique Summary

**One flat `quality.json` now drives the docs cap and the ci and wasm32 test-count floors through a `string(JSON)` checker with five negative controls, and `policy.lessons_unique` fails a reused or missing lesson ID in both the ci and hygiene lanes.**

## Performance

- **Tasks:** 2 of 2, 2 commits
- **Files:** 2 created, 8 modified

## Accomplishments

- Task 1, commit `b0e00b8`. `quality.json` holds `ceilings` `docs.max_bytes` 38912, `floors` `tests.ci` 410 and `tests.wasm32` 385, and an empty `reasons` list. `tests/cmake/quality.cmake` (49 lines) takes `-DQUALITY=<file>` and `-DMEASURED=<key>=<value>;...`, where a value is an integer, `lines:<path>` or `bytes:<path>`. It prints `quality: <key> measured=<n> ceiling=<c>` or `floor=<f>`, and fails with `quality: <key>: ...` for an exceeded ceiling, an unmet floor, a key with neither, a non-object or malformed file, or a non-integer value. `budgets.cmake` reads the docs cap with `string(JSON ... ERROR_VARIABLE)` and the literal 40960 is gone. Every synthetic budget tree writes its own `quality.json`.
- Task 2, commit `bf31c18`. A `lessons_unique` case in `repository.sh` collects IDs from table rows and from `Lesson CM-` and `Lesson XFER-` lines, names each ID seen twice, fails on zero, and prints the count (79 on the real ledger). It is registered in `budget_cases.cmake`, the file shared with the hygiene lane, so a planning-only pull request runs it.

## Evidence

All commands run from the repository root. No log is committed; scratch output went to the session scratchpad. No hosted run exists for this plan yet: the commits are local on `phase/05-tune-up` and the wasm32 inventory, which this plan edits identically to ci, is checked by the hosted wasm32 job (no wasm compiler is installed locally, so `cmake --preset wasm32` cannot configure here).

| Command | Exit |
|---------|------|
| `cmake --preset ci` with the new tests registered and no `quality.cmake` | 0 (inventory matches) |
| `ctest --test-dir build/ci -R '^(policy\.quality\|docs\.budgets)'` before implementing (RED) | 8, 7 of 13 failed |
| `cmake --preset ci` then the same ctest after implementing (GREEN) | 0 and 0, 13 of 13 passed |
| `cmake --preset hygiene` then `ctest --test-dir build/hygiene -R '^docs\.budgets'` | 0 and 0, 8 of 8 |
| `ctest --test-dir build/ci -R '^policy\.lessons_unique'` before the case existed (RED) | 8, 3 of 3 failed |
| the same ctest in `build/ci` and `build/hygiene` after (GREEN) | 0 and 0, 3 of 3 each |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0, `repository: 595 check lines / 7449 src lines` (N=595, M=7449; was 532 before) |
| `cmake --workflow --preset ci` | 0, 409 tests passed |
| `cmake --workflow --preset hygiene` | 0, 43 tests passed |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Negative-control regex broken by CMake's message wrapping**
- **Found during:** Task 1 GREEN run
- **Issue:** `policy.quality.malformed` expected `not valid JSON` but CMake wrapped the long path in the FATAL_ERROR text across lines
- **Fix:** the four quality regexes now allow `[ \n]+` between words
- **Files modified:** tests/policy/tests.cmake
- **Commit:** b0e00b8

**2. [Rule 1 - Bug] Lessons controls hung for the full timeout**
- **Found during:** Task 2 GREEN run
- **Issue:** `-DDOC=` was placed after `-P`, so CMake treated it as a script argument, `$DOC` was empty and awk read standard input
- **Fix:** `-DDOC=` precedes `-P`; the check now fails fast with `DOC must name a file` when it is empty or not a file
- **Files modified:** tests/policy/budget_cases.cmake, tests/cmake/repository.sh
- **Commit:** bf31c18

**3. [Rule 2 - Missing critical] No control for the new missing-quality.json failure**
- **Found during:** Task 1
- **Issue:** `budgets.cmake` gained a failure path (no quality.json or no key) that no listed control exercised
- **Fix:** added `docs.budgets.no_quality`, a tree without quality.json that must fail naming it; registered in ci.txt, wasm32.txt and hygiene.txt
- **Files modified:** tests/policy/budget_cases.cmake, the three inventories
- **Commit:** b0e00b8

**Total deviations:** 3 auto-fixed. **Impact:** one extra test per lane; no scope change.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- FOUND: quality.json
- FOUND: tests/cmake/quality.cmake
- FOUND commit b0e00b8
- FOUND commit bf31c18
