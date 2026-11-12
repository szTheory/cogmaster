---
phase: 07-saves-survive-a-restart
plan: "04"
subsystem: cartridge-database
tags:
  - cartdb
  - eeprom
  - generator
  - perf
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-03 Sega EEPROM board (board 10, 128-byte cart RAM) and plan 07-02 API 11 constant
provides:
  - the eeprom slot maps to board 10 in the generator; the seven Game Gear 93C46 rows emit persistence 2, geometry 1 and 128 bytes
  - src/cartdb.inc and tests/compat/catalog.tsv regenerated from the pinned lists, 7 rows changed and nothing else
  - a probe of an eeprom-slot row reports board 10, BATTERY, 128 bytes, SUPPORTED, so the core exports a 128-byte span with no setting
  - the boot_to_play Ir/frame delta against main from two workflow_dispatch runs
affects: [07-05, 07-06]

actuals:
  tokens: 9000
  tasks: 2
  commits: 2

plan_head_before: 03d075f9b082cad6547564b8d13bb35c3a58e9a5
plan_head_after: a06bf24efe3035ca24c3143985834d7af958d097
commits: 2

tech-stack:
  added: []
  patterns:
    - "An equivalence capture of the old table before regeneration, compared by comm afterwards"

key-files:
  created: []
  modified:
    - tools/cartdb.c
    - src/cartdb.inc
    - tests/compat/catalog.tsv
    - tests/cart/test_cartdb.c
    - tests/cart/generator.cmake
    - tests/api/v11.c
    - docs/boards.md

key-decisions:
  - "The eeprom slot sets persistence and the declared size by board, not by slot id, so the rule follows the board constant"
  - "Persistence value 3 is no longer emitted; the table has 70 rows of persistence 2 and none of 3"

requirements-completed: []

coverage:
  - id: D1
    description: "The generator maps the eeprom slot to board 10 with an empty reason and emits persistence 2, geometry 1 and 128 bytes for its 7 rows; no row carries persistence 3"
    requirement: SAVE-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^cart\\.(generator|database)$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "The regenerated table equals the old one outside the 7 eeprom rows, shown by comm of both fact lists"
    requirement: SAVE-01
    verification:
      - kind: other
        ref: "comm -23 and comm -13 over build/cartdb-input/old.facts and new.facts, 7 lines each, same SHA-1 set"
        status: pass
    human_judgment: false
  - id: D3
    description: "A database hit on an eeprom-slot row probes as board 10, BATTERY, persistent_size 128, geometry SUPPORTED"
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cart\\.database$'"
        status: pass
    human_judgment: false
  - id: D4
    description: "boot_to_play Ir/frame on the phase branch is within +1.00 percent of main, from two dispatch runs; allow.txt is untouched"
    requirement: SAVE-01
    verification:
      - kind: other
        ref: "gh workflow run ci.yml on the phase branch and on main; delta_bp=0; git diff --quiet origin/main -- tests/perf/allow.txt exit 0"
        status: pass
    human_judgment: false
---

# Phase 7 Plan 04: Turn the Game Gear EEPROM rows on Summary

The seven Game Gear 93C46 cartridges in the pinned CC0 lists now carry board 10, persistence 2, geometry 1 and 128 bytes in the regenerated table, no other row moved, and the boot_to_play Ir/frame delta against main is 0 basis points.

## What was built

- `tools/cartdb.c`: the `eeprom` slot row is `COGMASTER_BOARD_SEGA_EEPROM` with an empty reason; for a row on that board the record loop sets persistence 2 and declared 128 (instead of persistence 3); the geometry rule gives 1 also for board 10 with 128 bytes; `board_names[10]` is `sega_eeprom`, the word the runner probe uses.
- `src/cartdb.inc` and `tests/compat/catalog.tsv`: regenerated, never hand-edited. `src/cartdb.inc` changes the slot row and the 7 rows; the catalog changes 7 lines (board name `unsupported` to `sega_eeprom`).
- `tests/cart/test_cartdb.c`: board 10 joins the list of boards that must own a row; persistence counts are 70 of value 2 and 0 of value 3; the first row of the eeprom slot probes as board 10, BATTERY, 128, SUPPORTED.
- `tests/cart/generator.cmake`: the synthetic eeprom records are counted as board 10, 128-byte, geometry 1 rows instead of persistence 3.
- `docs/boards.md`: the "not emulated" bullet is gone; board 10 is in the field table and one sentence names the EEPROM board and points at `docs/saves.md`.

## Table equivalence

- `git show HEAD:src/cartdb.inc > build/cartdb-input/old.inc`: exit 0 (taken before the regeneration, on the committed table). An awk script in `$TMPDIR` (not committed) wrote `build/cartdb-input/old.facts` with 1657 rows (sha1, bytes, board, machine, region, video, persistence, ram bytes, geometry, public flag; sorted).
- `scripts/fetch-pin.sh mame-sms`, `mame-gamegear` and `mame-sg1000` into `build/cartdb-input/lists`: exit 0 each (SHA-256 verified by the script).
- `build/ci/cogmaster-cartdb --pin 1d7a454ed79e97d35d2a9f127ac029ead94711d6 --catalog tests/compat/catalog.tsv <three lists> --fixture tests/cart/fixture_facts.txt`: exit 0. Stderr: `slot eeprom seen=7 emitted=7 -> sega_eeprom`; 1670 candidates, 21 excluded. The stdout went to `build/cartdb-input/new.inc` and was copied to `src/cartdb.inc`.
- The same awk over the new `src/cartdb.inc` wrote `new.facts`, 1657 rows (unchanged count).
- `comm -23 old.facts new.facts`: exit 0, 7 lines; `comm -13 old.facts new.facts`: exit 0, 7 lines. The two lists name the same 7 SHA-1s (`cmp` exit 0). The 7 rows differ only in board (0 to 10), persistence (3 to 2), ram bytes (0 to 128) and geometry (0 to 1). 7 changed rows and 0 other differences.
- `git diff --numstat origin/main -- tests/compat/catalog.tsv`: exit 0, 7 added and 7 removed. `head -1 src/cartdb.inc | grep -c 1d7a454ed79e` prints 1. `grep -c '93C46 EEPROM save chip not emulated' tools/cartdb.c` prints 0.
- Per-persistence counts from the new table: 1587 of 1, 70 of 2, 0 of 3.

## Perf delta

Both runs are workflow_dispatch runs of `ci.yml`; the control line comes from each run's perf job.

- Branch run: https://github.com/szTheory/cogmaster/actions/runs/42938338186 (conclusion success), head `a06bf24efe3035ca24c3143985834d7af958d097`.
  `perf control boot_to_play runs=12302855,12302855,12302855 spread=0`
- Main run: https://github.com/szTheory/cogmaster/actions/runs/42937863505 (conclusion success), head `1e2dfbf3d090e0072252992126563fcf50daed09` (origin/main at the time of the check; plan 07-04's reuse of this value is valid while origin/main has not moved).
  `perf control boot_to_play runs=12302847,12302847,12302847 spread=0`
- H=12302855, B=12302847, `delta_bp=0` (shell integer arithmetic; the exact ratio is 0.65 hundredths of a percent). `git diff --quiet origin/main -- tests/perf/allow.txt` exits 0, so no allow line was added.
- The first branch run, https://github.com/szTheory/cogmaster/actions/runs/42937859365 at head `453d6bcb73888c2392d368a8d56ec47d7f479cbe`, reported the same control value (12302855) but failed on `abi.api_v11` (see Deviations); it was re-run after the fix and is the green run above.
- `git diff --quiet origin/main -- src/mapper.c tests/audio tests/testcard_pins.cmake tests/built`: exit 0.

## Task Commits

1. **Task 1: Flip the eeprom slot, regenerate, prove table equivalence** - `453d6bc` (feat)
2. **Task 2: Measure boot_to_play Ir/frame** - no source change; evidence in this file (allow.txt untouched)
3. **Fix found by Task 2's branch run** - `a06bf24` (fix)

**Plan metadata:** the docs commit that holds this file.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The API 11 test read an uninitialised struct**
- **Found during:** Task 2 (the branch dispatch run failed on `abi.api_v11` on every platform; it passed locally by chance of stack contents)
- **Issue:** `tests/api/v11.c` (plan 07-02) passed `cogmaster_versions v;` with no `size` set, so `cogmaster_get_versions` returned an argument error on the runners.
- **Fix:** zero the struct and set `size`, as `tests/api/v1.c` does.
- **Files modified:** `tests/api/v11.c`
- **Verification:** `ctest -R '^abi\.'` exit 0 locally; branch run 42938338186 green overall.
- **Commit:** a06bf24

**2. [Rule 1 - Bug] The generator check counted persistence 3 rows**
- **Found during:** Task 1 (`cart.generator` failed after the regeneration)
- **Issue:** `tests/cart/generator.cmake` counted the synthetic eeprom records as `.persistence = 3u`.
- **Fix:** it counts board 10, 128-byte, persistence 2, geometry 1 rows.
- **Files modified:** `tests/cart/generator.cmake` (not listed in the plan's files)
- **Commit:** 453d6bc

**Total deviations:** 2 auto-fixed (both Rule 1). **Impact:** none on scope; both were tests that encoded the old behaviour or a plan 07-02 slip.

## Verification

- `cmake --preset ci && cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^(cart\.|compat\.|eeprom\.|battery\.|runner\.probe|docs\.)'` exit 0 (49 tests)
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: 723 check lines of 8268 src lines
- `clang-format` 21 reports nothing on the changed C files

## Known Stubs

None.

## Threat Flags

None. The change moves seven rows of a read-only table and adds no network, auth or trust-boundary surface.

## Self-Check: PASSED

- FOUND: tools/cartdb.c, src/cartdb.inc, tests/compat/catalog.tsv, tests/cart/test_cartdb.c, tests/cart/generator.cmake, tests/api/v11.c, docs/boards.md
- FOUND commits: 453d6bc, a06bf24
