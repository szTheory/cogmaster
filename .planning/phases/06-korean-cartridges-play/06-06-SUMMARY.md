---
phase: 06-korean-cartridges-play
plan: "06"
subsystem: mapper
tags:
  - mapper
  - mapper-ext
  - korean
  - hicom
  - cartdb
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-03 compact table, slot rows and catalog; plan 06-04 mapper_ext.c default arms and the 8 KiB bank state
provides:
  - Korean $A000, Korean no-bank and Hi-Com arms in src/mapper_ext.c with one shared 8 KiB read
  - tests/mapper/test_mapper_ext.c, an exhaustive register and read test against an independent model on six exact image sizes
  - slot rows 13, 15 and 17 flipped to their boards, so 20 table rows (hicom 9, korean 6, korean_nb 5) load from a hash hit
affects: [06-07, 06-09, 06-11]

actuals:
  tokens: 6700
  tasks: 2
  commits: 2

plan_head_before: e1bbd98b20567c5d29e2152175ac1a571752d3bc
plan_head_after: 93d33dbd808b85a33a9fde4e4c256a560514fbcc
commits: 2

tech-stack:
  added: []
  patterns:
    - "One shared read for every 8 KiB-bank board: rom[(bank8[addr >> 13] << 13) | (addr & 0x1FFF)]"
    - "A bank number runs past a non-power-of-two image: mapper_range fault; past a power-of-two image it wraps silently"
    - "A test keeps its own bank model from the writes it issues and never reads library state to decide"

key-files:
  created:
    - tests/mapper/test_mapper_ext.c
  modified:
    - src/mapper_ext.c
    - tests/mapper/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tools/cartdb.c
    - src/cartdb.inc
    - tests/compat/catalog.tsv
    - tests/cart/test_cartdb.c
    - docs/boards.md

key-decisions:
  - "The over-range rule for every ext board is: the highest 8 KiB bank a register write selects is at or past twice the page count and the page count is not a power of two (Korean 2v+1, Hi-Com 4v+3)"
  - "Hi-Com power-on is banks 0 to 3 in the 32 KiB window and 0, 1 again at $8000, all reduced modulo twice the page count"
  - "The Korean no-bank board needs no write arm: the default arm already records rom_write below $C000"

requirements-completed: []

coverage:
  - id: C1
    description: "A Korean, Korean no-bank or Hi-Com table row loads on its board with no setting: slot rows 13, 15 and 17 name COGMASTER_BOARD_HICOM, KOREAN and KOREAN_NB with an empty reason, and the first row of each probes OK with board source DB"
    requirement: MAP-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(cart\\.|compat\\.|mapper\\.)' exit 0, 28 of 28 passed (cart.database probes the first row of each slot)"
      status: pass
    human_judgment: false
  - id: C2
    description: "mapper.ext.korean, mapper.ext.korean_nb and mapper.ext.hicom write every value 0 to 255 to every documented register on exact-size images of 32, 48, 128, 432, 800 KiB and 1 MiB, check bank8 below twice the page count and equal to an independent model after each write, and sweep the low 13 bits of every bank on 128 and 432 KiB"
    requirement: MAP-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.' exit 0, 15 of 15 passed; the same sources under cc -fsanitize=address,undefined exit 0 for all four cases; the asan job in CI runs the same executable"
      status: pass
    human_judgment: false
  - id: C3
    description: "A documented register write in range or on a power-of-two image records no fault, a write to $1234 records rom_write on the cartridge, and an over-range write on a non-power-of-two image records mapper_range on the mapper"
    requirement: MAP-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.ext\\.' exit 0; a mutation that selected the wrong Korean bank made mapper.ext.korean and mapper.ext.state fail"
      status: pass
    human_judgment: false
  - id: C4
    description: "mapper.ext.state round-trips the Korean bank state on a 432 KiB image and refuses a state whose mapper_bank8 is 255, leaving the loaded banks unchanged"
    requirement: MAP-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.ext\\.state$' exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "Only the board of slots 13, 15 and 17 changed in the regenerated table, the pin line is unchanged, and slot seen and emitted counts are unchanged"
    requirement: MAP-03
    verification:
      kind: other
      ref: "diff of the sorted uniq -c board and slot listings before and after exit 1 with only slot 13, 15 and 17 lines differing; awk over tests/compat/catalog.tsv exit 0; head -1 src/cartdb.inc piped through grep for the pin exit 0"
      status: pass
    human_judgment: false
  - id: C6
    description: "The whole plan passes the full local workflow, hygiene, the check-line budget, and leaves src/mapper.c, tests/audio, tests/testcard_pins.cmake and tests/built untouched"
    requirement: MAP-03
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines); git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0; grep -c '^-[^-]' over the tests/built diff prints 0; git log -1 --format=%s -- src/mapper.c names the 06-04 commit"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 06: Korean, Korean no-bank and Hi-Com boards Summary

The three 8 KiB-bank boards now sit behind the `default:` arms of `src/mapper_ext.c` with one shared read, an exhaustive model-checked test, and flipped slot rows, so 20 Korean-family table rows (hicom 9, korean 6, korean_nb 5) load on their board from a hash hit.

## What was built

- `src/mapper_ext.c`: `ext_bank` reduces a bank number modulo twice the page count (0 for no image); `ext_check` records `mapper_range` when the highest selected bank is at or past twice the page count and the page count is not a power of two. Korean takes one register at exactly `$A000` (banks 4 and 5 become `2(v mod p)` and `+1`); every other write below `$C000` is `rom_write`. Hi-Com takes `$FFFF` only (banks 0 to 3 become `4v + i`, banks 4 and 5 stay 0 and 1). Korean no-bank has no register and uses the default arm. Each arm cites the MAME `rom.cpp` device by name and pin; no MAME text was copied. The three arms stay behind `default:` in the two `mapper.c` switches, which this plan does not touch.
- `tests/mapper/test_mapper_ext.c`: for six exact image sizes it replaces the cartridge of a loaded instance with a hash-patterned image of exactly that size (so ASan sees an overread), writes every value 0 to 255 through the bus, and compares every `bank8` entry, the first and last byte of each 8 KiB window, and (on 128 and 432 KiB) the low 13 bits of every bank with a model kept from the writes alone. It also checks the fault record after each write, the power reset, the `$FFFC-$FFFF` and `$1234` controls, and the state round trip with the refused `mapper_bank8` of 255.
- `tools/cartdb.c`, `src/cartdb.inc`, `tests/compat/catalog.tsv`: slot rows 13, 15, 17 name their board; catalog board names for 4 to 6 are `korean`, `korean_nb`, `hicom`. Regenerated with the command recorded in the 06-03 summary (output redirected for `src/cartdb.inc`, `--catalog` writing the catalog), never hand-edited.
- `tests/cart/test_cartdb.c`: the supported-board list covers all six boards and the first row of each Korean-family slot probes `COGMASTER_STATUS_OK` on its board with board source DB.
- `docs/boards.md`: the three boards, the `board` field values 4 to 6, 8 KiB wrap, and that Korean no-bank and Hi-Com are found by hash only.

## Slot counts after regeneration

```
slot hicom seen=9 emitted=9 -> hicom
slot korean seen=6 emitted=6 -> korean
slot korean_nb seen=5 emitted=5 -> korean_nb
```

The seen and emitted counts equal those in the 06-03 summary (9, 6, 5); the other slot lines are unchanged and the first line of `src/cartdb.inc` still names the pin.

## Board and slot listings before and after

`grep -oE '\.board = [0-9]+u, \.slot = [0-9]+u' src/cartdb.inc | sort | uniq -c`:

Before (lines of the three slots only; every other line is identical in both listings):

```
   9 .board = 0u, .slot = 13u
   6 .board = 0u, .slot = 15u
   5 .board = 0u, .slot = 17u
```

After:

```
   6 .board = 4u, .slot = 15u
   5 .board = 5u, .slot = 17u
   9 .board = 6u, .slot = 13u
```

A diff of the two full listings shows exactly these three lines removed and these three added.

## Verification

- `cmake --preset ci` and `cmake --build --preset ci` exit 0.
- `ctest --test-dir build/ci -R '^mapper\.'` exit 0 (15 of 15); `ctest --test-dir build/ci -R '^(cart\.|compat\.|mapper\.)'` exit 0 (28 of 28).
- `ctest --test-dir build/ci -N -R '^mapper\.ext\.' | grep -cE 'mapper\.ext\.(korean|korean_nb|hicom|state)$'` prints 4; `grep -c 'rom.cpp' src/mapper_ext.c` prints 4.
- `cc -std=c17 -O1 -fsanitize=address,undefined` over `src/*.c` and the test, then the four cases, exit 0 each. The `asan` preset needs `clang-21`, which this machine does not have; CI's asan job runs the registered tests.
- A deliberate mutation of the Korean bank arithmetic made `mapper.ext.korean` and `mapper.ext.state` fail; the source was restored from a copy.
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0 (48 of 48); `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines of 7676 src lines).
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0; `git diff origin/main -- tests/built` exit 0 with 0 removed lines.
- Timing: each `mapper.ext.*` test runs in 0.04 s at most (slowest of 3 local runs), recorded in the `# Timeout:` comment with a 10 s floor.

## Deviations from Plan

None - plan executed exactly as written. The perf gate was not re-run: the three arms are inside `default:` and `src/mapper.c` is untouched, so the Sega, Codemasters and ROM-only paths run the same instructions as at plan 06-04 (delta_bp 0).

## Issues Encountered

None.

## Known Stubs

None. Zemina, Nemesis and 4-Pak remain named-unsupported rows until plan 06-07.

## Threat Flags

None. The change adds a pure read of an in-image array indexed by a bank state that the state loader bounds, and no network, auth or trust-boundary surface.

## Self-Check: PASSED

FOUND: src/mapper_ext.c, tests/mapper/test_mapper_ext.c, src/cartdb.inc, tests/compat/catalog.tsv, docs/boards.md. FOUND commits 5187df6 and 93d33db as ancestors of HEAD.
