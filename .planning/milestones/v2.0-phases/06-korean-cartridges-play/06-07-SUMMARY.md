---
phase: 06-korean-cartridges-play
plan: "07"
subsystem: mapper
tags:
  - mapper
  - mapper-ext
  - zemina
  - nemesis
  - 4pak
  - cartdb
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-06 shared 8 KiB read, modulo and range helpers and the exhaustive test rig; plan 06-03 slot rows and catalog
provides:
  - Zemina, Nemesis and 4 PAK arms in src/mapper_ext.c behind the existing default arms
  - mapper.ext.zemina, mapper.ext.nemesis and mapper.ext.hes_4pak, exhaustive register and read tests against an independent model
  - slot rows 12, 18 and 20 flipped to their boards, so 10 table rows (zemina 8, nemesis 1, 4pak 1) load from a hash hit
  - docs/boards.md describing every board and an Unsupported by design list with each reason
affects: [06-08, 06-09, 06-11]

actuals:
  tokens: 4150
  tasks: 2
  commits: 3

plan_head_before: fc15c3d3366975e9d7bb6fc596b0f838e9de343d
plan_head_after: 37448f621a280f66cc3bc05a0ab3b0715476fc52
commits: 3

tech-stack:
  added: []
  patterns:
    - "A 16 KiB-page board keeps its register values in raw and writes its slots as bank8 pairs, so the one shared 8 KiB read serves it"
    - "A fault expectation in a sweep follows the model: the first over-range write fixes the address the sticky fault record must name"
    - "The generator's start-up self-check refuses the temporary slot reason, so a wave-1 row cannot ship"

key-files:
  created: []
  modified:
    - src/mapper_ext.c
    - tests/mapper/test_mapper_ext.c
    - tests/mapper/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tools/cartdb.c
    - src/cartdb.inc
    - tests/compat/catalog.tsv
    - tests/cart/test_cartdb.c
    - tests/cart/generator.cmake
    - docs/boards.md

key-decisions:
  - "4 PAK writes to $FFFD-$FFFF set 16 KiB slots 0, 1 and 2 to the value modulo the page count and leave raw alone (MAME inherits the Sega write_mapper); the register-only reading is cited in code and test and not implemented"
  - "Zemina and Nemesis share one write arm; Nemesis differs only in powering up on the last 8 KiB bank at $0000"
  - "Over-range on the 4 PAK means a slot value at or past the page count on a non-power-of-two image, checked for every slot a write sets"

requirements-completed: [MAP-04, MAP-07]

coverage:
  - id: C1
    description: "A Zemina, Nemesis or 4 PAK table row loads on its board with no setting: slot rows 20, 18 and 12 name COGMASTER_BOARD_ZEMINA, NEMESIS and 4PAK with an empty reason, and the first row of each probes OK with board source DB"
    requirement: MAP-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(cart\\.|compat\\.|mapper\\.)' exit 0, 31 of 31 passed (cart.database probes the first row of each slot and requires every supported board to own a row)"
      status: pass
    human_judgment: false
  - id: C2
    description: "The Janggun row stays unsupported: it probes as the unsupported-board status with board 0, its slot id and a reason naming the unresolved reverse bit, and docs/boards.md lists it under Unsupported by design with every other unsupported slot"
    requirement: MAP-04
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.database$' exit 0 (the janggun row through fact_to_info); awk over tests/compat/catalog.tsv exit 0 for the janggun, zemina, nemesis and 4pak columns; grep -c 'Unsupported by design' docs/boards.md prints 1"
      status: pass
    human_judgment: false
  - id: C3
    description: "mapper.ext.zemina, mapper.ext.nemesis and mapper.ext.hes_4pak write every value 0 to 255 to every documented register through the bus on exact-size images of 32, 48, 128, 432, 800 KiB and 1 MiB, check bank8 below twice the page count and equal to an independent model after each write, and sweep the low 13 bits of every bank on 128 and 432 KiB"
    requirement: MAP-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.' exit 0, 18 of 18 passed; the same sources under cc -fsanitize=address,undefined exit 0 for all three cases; the asan job in CI runs the same executable; mutations of the 4 PAK mask, the Nemesis power-on bank and the Zemina window order each made their case exit 1"
      status: pass
    human_judgment: false
  - id: C4
    description: "Documented register writes record no fault in range or on a power-of-two image, a write to $1234 records rom_write, and an over-range write on a non-power-of-two image records mapper_range at the first over-range address"
    requirement: MAP-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.ext\\.' exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "4 PAK slot 2 is ((reg0 and $30) plus reg2) modulo the page count under each of the four $3FFE bit 5-4 settings, the $FFFD-$FFFF writes leave raw unchanged, a $FFFC write moves no bank, a later $3FFE write recomputes slot 2 from raw, and a state round trip restores raw so slot 2 recomputes from the saved values"
    requirement: MAP-07
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.ext\\.hes_4pak$' exit 0"
      status: pass
    human_judgment: false
  - id: C6
    description: "No table slot carries the temporary reason, the generator refuses it at start-up, only the board of slots 12, 18 and 20 changed in the regenerated table, and the pin line is unchanged"
    requirement: MAP-03
    verification:
      kind: other
      ref: "grep -q 'mapper arm not present' src/cartdb.inc exit 1; the regeneration command of the 06-03 summary exit 0; git diff -U0 src/cartdb.inc shows only the three slot rows and the board of 10 fact rows changed; head -1 src/cartdb.inc piped through grep for the pin prints 1"
      status: pass
    human_judgment: false
  - id: C7
    description: "The whole plan passes the full local workflow and hygiene, stays inside the check-line budget, and leaves src/mapper.c, tests/audio, tests/testcard_pins.cmake and tests/built untouched"
    requirement: MAP-07
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0 (426 tests passed); cmake --workflow --preset hygiene exit 0 (48 of 48); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines of 7746 src lines); git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0; grep -c '^-[^-]' over the tests/built diff prints 0; git log -1 --format=%s -- src/mapper.c names the 06-04 commit; wc -c docs/boards.md prints 7341"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 07: Zemina, Nemesis and 4 PAK boards Summary

Zemina, Nemesis and 4 PAK page through the shared 8 KiB read of `src/mapper_ext.c` behind the `default:` arms, with three exhaustive model-checked tests and flipped slot rows, so 10 more table rows load on their board from a hash hit; Janggun stays an unsupported row and `docs/boards.md` now lists every unsupported slot with its reason.

## What was built

- `src/mapper_ext.c`: Zemina and Nemesis share one write arm (registers at exactly `$0000`-`$0003` set banks 4, 5, 2 and 3 to the value modulo twice the page count; any other write below `$C000` is `rom_write`); Nemesis differs only in `cogmaster__mapext_reset`, where bank 0 powers up as the last 8 KiB bank. The 4 PAK keeps the `$3FFE` and `$BFFF` values in `raw[0]` and `raw[1]`, pages 16 KiB slots as `bank8` pairs through one helper, powers up all zero, and treats `$FFFD`-`$FFFF` as MAME does. The comment and the test both name the two readings of those addresses (MAME's inherited Sega `write_mapper`, and the register-only reading) and the one implemented. Every arm cites `rom.cpp` and pin `1d7a454ed79e`; no MAME text was copied. `src/mapper.c` is untouched.
- `tests/mapper/test_mapper_ext.c`: the model grew a raw pair and one `model_write(addr, v)` that returns whether the write selects past the image; the Korean and Hi-Com cases use it unchanged in behaviour. `run_regs` sweeps every value 0 to 255 to each register of Zemina (4 addresses), Nemesis and 4 PAK (`$3FFE`, `$7FFF`, `$BFFF`, `$FFFC`-`$FFFF`) on six exact image sizes, with `$BFFF` repeated under the four `$3FFE` settings, power-resetting between registers so a sticky fault from one never hides another. After every write it compares all six banks, the 4 PAK raw pair, the first and last byte of each window, and on 128 and 432 KiB the low 13 bits of every bank, and it checks the fault record against the first over-range address the model saw. `hes_4pak` also checks explicit values (slot 2 from raw, `$FFFF` leaving raw alone, a later `$3FFE` recomputing) and a state round trip that restores raw.
- `tests/mapper/tests.cmake`, `tests/inventory/{ci,wasm32}.txt`: the three cases registered beside the others (`# Timeout:` comment at the 10 s floor; slowest local run 0.27 s), inventories in sorted place.
- `tools/cartdb.c`, `src/cartdb.inc`, `tests/compat/catalog.tsv`: slot rows `zemina`, `nemesis` and `4pak` name their boards with empty reasons; catalog board names `zemina`, `nemesis` and `hes_4pak`; the generator's table self-check now fails on the temporary reason, so no wave-1 row can ship. Regenerated with the 06-03 command, never hand-edited.
- `tests/cart/test_cartdb.c`: the first row of every Korean-family slot (six now) probes OK on its board with board source DB, and every one of the nine supported boards must own a row. The Janggun row's unsupported probe was already asserted by the named-row loop and still passes.
- `docs/boards.md`: the three boards, the `board` values 7 to 9, and an Unsupported by design list (Janggun, Seo Jin, 188-in-1, the SG-1000 boards, Master Gear, X-Terminator, the Game Gear EEPROM boards) with each reason, the probe fields and the exit 77.

## Slot counts after regeneration

```
slot 4pak seen=1 emitted=1 -> hes_4pak
slot nemesis seen=1 emitted=1 -> nemesis
slot zemina seen=8 emitted=8 -> zemina
slot janggun seen=1 emitted=1 -> unsupported (reverse bit 6 vs 7 unresolved (MAME and SMS Power disagree))
```

The seen and emitted counts equal the 06-03 summary (zemina 8, nemesis 1, 4pak 1); every other slot line is unchanged and the first line of `src/cartdb.inc` still names the pin.

## Verification

- `cmake --preset ci` and `cmake --build --preset ci` exit 0 with no warnings.
- `ctest --test-dir build/ci -R '^mapper\.'` exit 0 (18 of 18); `ctest --test-dir build/ci -R '^(cart\.|compat\.|mapper\.)'` exit 0 (31 of 31); `ctest --test-dir build/ci -N -R '^mapper\.ext\.' | grep -cE 'mapper\.ext\.(zemina|nemesis|hes_4pak)$'` prints 3; `grep -c 'rom.cpp' src/mapper_ext.c` prints 8.
- The plan's task 2 verify command (build, ctest, the grep for the temporary reason, the awk over the catalog) exits 0.
- `cc -std=c17 -O1 -fsanitize=address,undefined` over `src/*.c` and the test, then each of the three cases, exit 0 (0.6 s, 0.6 s, 1.4 s). The `asan` preset needs `clang-21`, which this machine lacks; CI's asan job runs the registered tests.
- Three deliberate mutations (4 PAK mask `$30` to `$20`, Nemesis power-on bank off by one, Zemina window order swapped) each made the matching case exit 1; the source was restored from a copy.
- `cmake --workflow --preset ci` exit 0 (426 tests passed); `cmake --workflow --preset hygiene` exit 0 (48 of 48); `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines of 7746 src lines).
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0; `git diff origin/main -- tests/built` exit 0 with 0 removed lines; `git log -1 --format=%s -- src/mapper.c` names the 06-04 commit.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] cart.generator expected the 4pak slot line to say unsupported**
- **Found during:** Task 2
- **Issue:** `tests/cart/generator.cmake` matched `cartdb: slot 4pak ... unsupported` on the synthetic-input run; flipping the row made `cart.generator` fail.
- **Fix:** the pattern now expects the board name `hes_4pak` on that line, the same fault coverage (the per-slot line reflects the table).
- **Files modified:** tests/cart/generator.cmake
- **Commit:** 37448f6

**2. [Rule 3 - Blocking] clang-format on the new test**
- **Found during:** Task 2 hygiene run
- **Issue:** `hygiene.format` rejected the new test source and the extended supported-board list.
- **Fix:** ran clang-format 21 over the four files; the formatting of `tests/mapper/test_mapper_ext.c` landed in its own `style` commit because task 1 was already committed.
- **Files modified:** tests/mapper/test_mapper_ext.c, tests/cart/test_cartdb.c
- **Commit:** 56cc250, 37448f6

The perf gate was not re-run: the arms are inside `default:` and `src/mapper.c` is untouched, so the Sega, Codemasters and ROM-only paths run the same instructions as at plan 06-04 (delta_bp 0).

## Issues Encountered

None.

## Known Stubs

None. Janggun, Seo Jin, 188-in-1, the SG-1000 boards, Master Gear, X-Terminator and the Game Gear EEPROM boards are unsupported by design and documented as such; no row carries a temporary reason.

## Threat Flags

None. The change adds pure reads of an in-image array indexed by a bank state that the state loader bounds, and no network, auth or trust-boundary surface.

## Self-Check: PASSED

FOUND: src/mapper_ext.c, tests/mapper/test_mapper_ext.c, src/cartdb.inc, tests/compat/catalog.tsv, docs/boards.md. FOUND commits 2025775, 56cc250 and 37448f6 as ancestors of HEAD.
