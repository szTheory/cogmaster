---
phase: 07-saves-survive-a-restart
plan: "02"
subsystem: public-api
tags:
  - api
  - header
  - board-constant
  - versions
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-01 built the 93C46 device the new board constant names
provides:
  - COGMASTER_BOARD_SEGA_EEPROM 10u appended after the 4PAK board, with API version 11, behaviour revision 13 and state format 13
  - tests/api/v11.c, registered as abi.api_v11 on the ci, nofp and wasm32 inventories
  - docs/embedding.md text for API 11 and the new board
  - cogmaster-run --probe naming board 10 sega_eeprom
affects: [07-03, 07-04, 07-05]

actuals:
  tokens: 1650
  tasks: 2
  commits: 2

plan_head_before: f08d24e
plan_head_after: 4ee7a171de6788f9ef76c7182a7e6d5681fe2f06
commits: 2

tech-stack:
  added: []
  patterns:
    - "An older API program asserts its version with >= so a later version bump does not break it; only the newest program pins the exact numbers"

key-files:
  created:
    - tests/api/v11.c
  modified:
    - include/cogmaster.h
    - tests/api/v10.c
    - tests/abi/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/nofp.txt
    - tests/inventory/wasm32.txt
    - docs/embedding.md
    - runner/probe.c

key-decisions:
  - "The board constant is appended at 10 and no other define changed value; a saved state from format 12 is refused by the existing version check and no migration is written"
  - "tests/api/v10.c asserted API version equal to 10, which the bump would have broken; it now asserts at least 10 like v6 to v9 do"
  - "The probe word is sega_eeprom, the word plan 07-04 gives the catalog generator so the two stay equal"

requirements-completed: []

coverage:
  - id: C1
    description: "The public header appends COGMASTER_BOARD_SEGA_EEPROM 10u and reads API version 11, behaviour revision 13 and state format 13 with no earlier define changed"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^abi\\.api_v11$' exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "Every public name the header holds is named in some tests/api program up to v11, and abi.api_v11 runs on the ci, nofp and wasm32 inventories"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(abi\\.|policy\\.|core\\.state|header\\.)' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "cogmaster-run --probe names board 10 with the word sega_eeprom, and the embedding docs state the new numbers and board"
    requirement: SAVE-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^(runner\\.probe|docs\\.|policy\\.)' exit 0"
      status: pass
    human_judgment: false

duration: 12min
completed: 2026-11-10
---

# Phase 7 Plan 2: The API 11 surface Summary

**API 11 is public: the Sega EEPROM board constant is reserved at 10, behaviour revision and state format move to 13, and the v11 program, the docs and the probe name all carry it.**

## Performance

- **Duration:** about 12 min
- **Completed:** 2026-11-10
- **Tasks:** 2
- **Files modified:** 9 (one new)

## Accomplishments

- `include/cogmaster.h`: `COGMASTER_BOARD_SEGA_EEPROM 10u` after the 4PAK board with a one-line comment, and the three version numbers raised. Measured against `origin/main`, the only removed defines are the version numbers.
- `tests/api/v11.c` checks the three compile-time numbers, the same numbers through `cogmaster_get_versions`, the new constant at 10 and its difference from every earlier board constant. It is registered as `abi.api_v11` with the other API programs and listed on all three inventories.
- `docs/embedding.md` states API 11, behaviour 13 and state format 13 in the existing version text and links to `saves.md` for the chip behaviour. `runner/probe.c` names board 10 `sega_eeprom`.

## Task Commits

1. **Task 1: API 11 constants, version numbers and the v11 program** - `89bec28` (feat)
2. **Task 2: Embedding docs and the probe's board name** - `4ee7a17` (docs)

**Plan metadata:** the commit that carries this file.

## Evidence

All from the repository root, no `cd`:

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(abi\.\|policy\.\|core\.state\|header\.)'` | 0 (48 of 48) |
| `ctest --test-dir build/ci -R '^(runner\.probe\|docs\.\|policy\.)'` then `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` | 0 |
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` | 0 |

No run URL: nothing was pushed in this plan.

## Decisions Made

See key-decisions. No migration for state format 12: states are not a cross-version format and the existing version check refuses them.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] tests/api/v10.c pinned the API version to 10**
- **Found during:** Task 1
- **Issue:** `CHECK_EQ_U64(COGMASTER_API_VERSION, 10u)` in v10.c fails as soon as the header reads 11; the plan did not list v10.c.
- **Fix:** Changed it to `CHECK(COGMASTER_API_VERSION >= 10)`, the form v6 to v9 already use. v11.c pins the exact value.
- **Files modified:** tests/api/v10.c
- **Verification:** `abi.api_v10` and `abi.api_v11` pass.
- **Committed in:** 89bec28

---

**Total deviations:** 1 auto-fixed (1 blocking). **Impact on plan:** none; one assertion form changed in a file the bump touches anyway.

## Issues Encountered

None. `state advance-plan` was not used: STATE.md has no "Current Plan: N" line, so the Current Position text was edited by hand, as 07-01 found.

## Known Stubs

None.

## Next Phase Readiness

The board and state plans compile against settled numbers. Plan 07-04 must give `tools/cartdb.c` board_names the word `sega_eeprom` at index 10 so the catalog and the probe agree. `docs/saves.md` (plan 07-03) is linked from the embedding docs.

## Self-Check: PASSED

- `tests/api/v11.c` exists; commits `89bec28` and `4ee7a17` are ancestors of HEAD.

---
*Phase: 07-saves-survive-a-restart*
*Completed: 2026-11-10*
