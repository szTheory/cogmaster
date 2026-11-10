---
phase: 06-korean-cartridges-play
plan: "02"
subsystem: public-api
tags:
  - api-10
  - board-constants
  - cart-info
  - abi
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-01 generator slot table naming the Korean-board slots
provides:
  - COGMASTER_API_VERSION 10 with ABI 1, behaviour revision 12 and state format 12
  - appended board constants COGMASTER_BOARD_KOREAN 4 to COGMASTER_BOARD_4PAK 9 and board source constants 1 to 4
  - cogmaster_cart_info fields board_source, board_candidate, scan_sega, scan_candidate, scan_runner_up, unsupported_id and unsupported_reason[128] after title
  - tests/api/v10.c registered as abi.api_v10 in all three inventories
affects: [06-03, 06-06, 06-07, 06-08]

actuals:
  tokens: 1800
  tasks: 2
  commits: 2

plan_head_before: 9d64d4f0ceafd65129c925321acddf03420079ef
plan_head_after: 7ccc55a
commits: 2

tech-stack:
  added: []
  patterns:
    - "A new API version appends fields behind the old struct size and a version program proves an old-size caller leaves every appended byte untouched"

key-files:
  created:
    - tests/api/v10.c
  modified:
    - include/cogmaster.h
    - tests/abi/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/nofp.txt
    - tests/inventory/wasm32.txt
    - tests/cmake/repository.sh
    - docs/embedding.md

key-decisions:
  - "The appended cart_info block starts at offset 376 (the version 9 size on every target) and adds 152 bytes: six uint32 fields then unsupported_reason[128]"
  - "api_version compares the API number as a number: awk's sub() leaves a string, so k<=v was a string comparison and from API 10 on only v1.c was scanned"

requirements-completed: []

coverage:
  - id: C1
    description: "The header reports API 10, ABI 1, behaviour revision 12 and state format 12, and every name it adds appears in tests/api/v10.c"
    requirement: MAP-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^abi\\.(api_v10|api_version|api_version\\.self_test|api_v9|api_v3)$' exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "Board constants 4 to 9 and board source constants 1 to 4 have the stated values and existing values 1 to 3 do not move"
    requirement: MAP-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^abi\\.api_v10$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "A caller passing the version 9 struct size (376) keeps every appended byte untouched; a full-size caller sees board_source at most 4, zero scan counts and a zero-terminated unsupported_reason"
    requirement: MAP-05
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^abi\\.api_v10$' exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "The embedding guide names API version 10, the new constants and the new fields, and agrees with the header"
    requirement: MAP-06
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^docs\\.' exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "The whole workflow passes with the version bump and the new test"
    requirement: MAP-06
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 02: API 10 surface Summary

The public header is API 10 (ABI 1, behaviour revision 12, state format 12) with the six Korean-board constants, the four board-source constants and the seven appended `cogmaster_cart_info` fields, proven by a new `tests/api/v10.c` that checks values, layout and the old-size caller guarantee.

## What was built

- `include/cogmaster.h`: version numbers bumped once for the phase; `COGMASTER_BOARD_KOREAN` 4, `_KOREAN_NB` 5, `_HICOM` 6, `_ZEMINA` 7, `_NEMESIS` 8, `_4PAK` 9; `COGMASTER_BOARD_SOURCE_DB` 1, `_HEADER` 2, `_HEURISTIC` 3, `_DEFAULT` 4; `board_source`, `board_candidate`, `scan_sega`, `scan_candidate`, `scan_runner_up`, `unsupported_id` and `unsupported_reason[128]` after `title`. Header is 9,110 bytes (cap 24,576). Nothing fills the fields yet; a probe leaves them zero.
- `tests/api/v10.c`: values of every new constant, `offsetof` of each appended field from 376, `sizeof` equal to 376 + 152, a probe with the version 9 size and `0xA5` sentinels showing the appended bytes untouched, and a full-size probe showing `board_source` at most 4, zero scan counts and a terminated reason.
- `tests/abi/tests.cmake` and the three inventories: `abi.api_v10` registered and listed in byte order between `abi.api_v1` and `abi.api_v2`.
- `docs/embedding.md`: four lines for the version paragraph (constants, fields, revisions, link to the boards guide) and the per-version name list extended by two lines (6 added lines against `origin/main`).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] `abi.api_version` stopped scanning every version program at API 10**
- **Found during:** Task 1 verification
- **Issue:** `tests/cmake/repository.sh` took the API number from `sub()`, which leaves an awk string, so `for(k=1;k<=v;k++)` compared strings: `"2" <= "10"` is false and only `v1.c` was read, flagging every older name as untested.
- **Fix:** `v+=0` after the digit check on the existing line, so the version compares as a number. No counted line was added (budget stays 729 of 745).
- **Files modified:** tests/cmake/repository.sh
- **Commit:** d89c8be

**2. [Rule 3 - Blocking] `docs.api_names` fails until the guide names the new constants**
- **Found during:** Task 1 verification (full ctest)
- **Issue:** every non-wildcard header name must appear in the guide; this is Task 2's content, so Task 1's commit leaves `docs.api_names` red until Task 2's commit.
- **Fix:** Task 2 names every constant literally.
- **Commit:** 7ccc55a

**Total deviations:** 2 auto-fixed (1 bug, 1 ordering). **Impact:** none on scope.

## Verification

- `cmake --preset ci > /dev/null && cmake --build --preset ci && ctest --test-dir build/ci -R '^abi\.(api_v10|api_version|api_version\.self_test|api_v9|api_v3)$'`: exit 0
- Task 2 verify chain (greps, size under 36,864, `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake`): exit 0 (729 check lines of 745)
- `cmake --workflow --preset ci`: exit 0
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake`: exit 0; `git diff origin/main -- tests/built`: exit 0 with 0 removed lines (no shipped pin moved)
- Acceptance: the four version defines present (count 4), `abi.api_v10` once per inventory file, header 9,110 bytes, 6 lines added to `docs/embedding.md`

## Known Stubs

None. The new `cogmaster_cart_info` fields read zero until plans 06-03 and 06-08 fill them; that is the plan's stated scope, not a placeholder in rendered output.

## Threat Flags

None. The change appends fields to an output struct behind the caller's size tag and adds no input path.

## Self-Check: PASSED

- FOUND: include/cogmaster.h, tests/api/v10.c, tests/abi/tests.cmake, docs/embedding.md
- FOUND commits: d89c8be, 7ccc55a
