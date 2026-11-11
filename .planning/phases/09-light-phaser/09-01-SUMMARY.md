---
phase: 09-light-phaser
plan: "01"
subsystem: public-header
tags:
  - header
  - abi
  - api-version-13
status: complete

requires: []
provides:
  - COGMASTER_DEVICE_LIGHT_PHASER 4u in the public header, with versions API 13, behaviour 15 and state 15 and an unchanged layout
  - tests/api/v13.c registered as abi.api_v13 on the ci, wasm32 and nofp lanes
affects: [09-02, 09-03, 09-05, 09-08]

actuals:
  tokens: 1800
  tasks: 2
  commits: 2

plan_head_before: e0a754a55034b77aca305b5d18598d1cf924e91b
plan_head_after: 3935f966eeedc639bee8530659f038f468d36cda
commits: 2

tech-stack:
  added: []
  patterns:
    - "An older version pin keeps every constant, size and offset as == and loosens only the three version macros and v.api, v.behaviour, v.state to >="
    - "The device ceiling in src/internal.h stays where it is until the plan that teaches the validator, the fuzz model and the state checker about the new device"

key-files:
  created:
    - tests/api/v13.c
  modified:
    - include/cogmaster.h
    - docs/embedding.md
    - tests/api/v12.c
    - tests/abi/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/nofp.txt

key-decisions:
  - "COGMASTER__DEVICE_LAST stays COGMASTER_DEVICE_SPORTS_PAD, so device 4 is still ERR_ARGUMENT in set_input and in the state bound and no test or model written against the old ceiling moves"
  - "The one header comment line sits on the axis fields of cogmaster_input, which the file already comments, and says the pair is an absolute aim with 0 0 for none on a phaser port"
  - "v13.c holds no case about what a phaser selection answers, because it would turn red in the plan that raises the ceiling"

requirements-completed: []

coverage:
  - id: C1
    description: "The header names COGMASTER_DEVICE_LIGHT_PHASER as 4u after SPORTS_PAD and raises the versions to API 13, behaviour 15 and state 15 with ABI 1; the header is 9,774 bytes of its 24,576"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^abi\\.api_v13$' exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "No layout moved: cogmaster_input 48, cogmaster_run_result 24, cogmaster_info 76, cogmaster_cart_info 536 and the four axis offsets are pinned in v13.c, v12.c keeps every size and offset pin as ==, and the device ceiling and every src, fuzz, ports and core file are identical to origin/main"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "git diff --quiet origin/main -- src tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports tests/fuzz tests/core exit 0; ctest --test-dir build/ci -R '^(abi\\.|docs\\.api_names|ports\\.state\\.|fuzz\\.regress\\.pad)' exit 0 (30 of 30)"
      status: pass
    human_judgment: false
  - id: C3
    description: "abi.api_v13 is registered beside abi.api_v12 and listed in the ci, wasm32 and nofp inventories only, and docs.api_names finds the new name in embedding.md (one added line of 279 bytes)"
    requirement: GUN-01
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci exit 0 (545 of 545); cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "The version pin is live: raising COGMASTER_API_VERSION to 14 without a v14.c turns abi.api_version red, and restoring it turns it green"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "python edit of include/cogmaster.h to 14, rebuild exit 0, ctest -R '^abi\\.api_version$' exit 8; restored, rebuild exit 0, same ctest exit 0"
      status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-11
---

# Phase 9 Plan 1: The phaser constant and the version 13 pin Summary

**`COGMASTER_DEVICE_LIGHT_PHASER` (4u) joins the public header with API 13, behaviour 15 and state 15 and no layout change, pinned by `abi.api_v13` on three lanes, while the device ceiling stays at the Sports Pad so no behaviour, hash or model moves.**

## Performance

- **Duration:** about 15 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 8 (1 new)

## Accomplishments

- `include/cogmaster.h` carries the new constant, the three raised versions and one comment on the axis fields; `tests/api/v12.c` loosens only its version checks to `>=` and keeps every constant, size and offset as `==`.
- `tests/api/v13.c` pins the constant, the three versions, the four struct sizes and the four axis offsets, and says nothing about what a phaser selection answers.
- `docs/embedding.md` gains one 279-byte paragraph naming the device and its aim fields; `docs.api_names` stays green.
- `abi.api_v13` is registered beside `abi.api_v12` and listed in the ci, wasm32 and nofp inventories.

## Task Commits

1. **Task 1: Header delta, version loosening and the v13 pin** - `7851c60` (feat)
2. **Task 2: Register abi.api_v13** - `3935f96` (test)

**Plan metadata:** the commit that carries this file.

## Evidence

All from the repository root, no `cd`:

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(abi\.\|docs\.api_names)'` (task 1; 23 tests) | 0 |
| `git diff --quiet origin/main -- src tests/fuzz tests/ports tests/core` and the wider set with `tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake` | 0 |
| negative control: python edit raising `COGMASTER_API_VERSION` to 14, rebuild, `ctest -R '^abi\.api_version$'` | rebuild 0, ctest 8 (red) |
| same control reverted, rebuild, same ctest | 0, 0 |
| `ctest --test-dir build/ci -N -R '^abi\.api_v1[0-3]$'` | 0 (4 tests) |
| `cmake --workflow --preset ci` | 0 (545 of 545) |
| `cmake --workflow --preset hygiene` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 |
| `clang-format --dry-run -Werror include/cogmaster.h tests/api/v12.c tests/api/v13.c` | 0 |
| `ctest --test-dir build/ci -R '^(abi\.\|docs\.api_names\|ports\.state\.\|fuzz\.regress\.pad)'` (30 tests) | 0 |

Acceptance counts: `COGMASTER_DEVICE_LIGHT_PHASER 4u` once in the header and the name in `docs/embedding.md`; the `DEVICE_LAST` define in `src/internal.h` is the Sports Pad; the header is 9,774 bytes; `embedding.md` gains one line; `tests/api/v12.c` has 0 phaser references and 17 size and offset pins; no planning ID appears in an added line; `abi.api_v13` appears once in each of the three inventories and in none of hygiene, fuzz, library and vectors-full. No run URL: nothing was pushed or dispatched.

## Decisions Made

See key-decisions.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None.

## Threat Flags

None. No code reads the new constant yet.

## Self-Check: PASSED

- `tests/api/v13.c` exists; commits `7851c60` and `3935f96` are ancestors of HEAD; `git rev-list --count e0a754a..HEAD` was 2 before this file.
- Every acceptance criterion of both tasks was re-run and passes.
