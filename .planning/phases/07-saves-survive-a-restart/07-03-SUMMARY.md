---
phase: 07-saves-survive-a-restart
plan: "03"
subsystem: cartridge-eeprom
tags:
  - eeprom
  - 93c46
  - mapper
  - state
  - docs
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: the 93C46 device (plan 01) and the board constant with API 11 (plan 02)
provides:
  - the Sega EEPROM board arm in src/mapper_ext.c (read, write, reset) behind default: in mapper.c, which is unchanged
  - 128 bytes of erased cartridge RAM for board 10, so the battery span, generation, runner and libretro need no special case
  - a bounded state visit of the chip, a control byte limited to bits 3 and 7, and a bank8 and raw visit that no longer reaches board 10
  - eight eeprom.* cases (five board, three state) on the ci and wasm32 inventories
  - a 93C46 EEPROM section in docs/saves.md
affects: [07-04, 07-05, 07-06]

actuals:
  tokens: 9100
  tasks: 3
  commits: 3

plan_head_before: 8b66deba95cf445eee3c9938741e7006883b8a7b
plan_head_after: 5a2b8252d69595cc2761caaddc90af584ee1513e
commits: 3

tech-stack:
  added: []
  patterns:
    - "A board that needs paging registers of its own keeps them in the ext arm and reuses the existing bank array, with a small static helper and no copy of mapper.c statics"
    - "A mid-command round trip is proven by saving at every clock count, loading into a second instance and comparing the finished state bytes, the Data Out stream and the image with an uninterrupted reference run"

key-files:
  created: []
  modified:
    - src/internal.h
    - src/mapper_ext.c
    - src/state.c
    - src/cart.c
    - tests/mapper/test_eeprom.c
    - tests/mapper/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/saves.md

key-decisions:
  - "The generation moves for a changed byte only while a battery span exists (the same guard the battery write uses), so a state with no supported persistence still loads with generation zero"
  - "A bit 7 write to the control register resets the chip, which also clears the byte the window shows; the pinned MAME board keeps that byte, the difference shows only when a game reads the window after a reset without writing it first"
  - "State bounds are tighter than the plan's listed widths: a command counts at most 7 bits after the start bit (the 8th completes it and the count restarts), data in and a read count at most 15, idle and complete phases count 0; the all-bit-counts round trip proves every one of these states is reachable"
  - "sizeof(struct cogmaster__eeprom) is 8 bytes (7 bytes of fields plus one of padding)"

requirements-completed: []

coverage:
  - id: D1
    description: "Board 10 opens a window at exactly $8000 with $FFFC bit 3 and returns the last written bits with the low two cleared, bit 1 set and Data Out; every other address stays ROM and the first 1 KiB stays page 0"
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.board\\.window$' exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "A window write drives data in, clock and chip select from bits 0, 1 and 2; a WRITE after EWEN stores the word little-endian in cart.ram and moves the generation once, and a same-data WRITE does not move it"
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.board\\.pins$' exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "A $FFFC write with bit 7 drops chip select, clears the shift state and locks writes again, and the image survives"
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.board\\.reset_bit$' exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "$FFFD-$FFFF page 16 KiB banks modulo the page count with mapper_range only past a non-power-of-two image; documented writes record no fault and a write to ROM or to a closed window does"
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.board\\.banks$' exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The 128-byte image starts all FF in cart.ram, is exported by cogmaster_get_battery with length 128 and KNOWN knowledge, and an imported 34 12 image reads back as 0x1234 from word 0 through the window"
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.board\\.span$' exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "A state saved at every clock count of a WRITE and of a READ resumes exactly: finished state bytes, Data Out stream, image and generation equal those of an uninterrupted run"
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.state\\.mid_command$' exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "A state saved with the window open and banks set loads into a fresh instance that reads the window and the banks identically"
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.state\\.window_open$' exit 0"
        status: pass
    human_judgment: false
  - id: D8
    description: "A state with a count past its phase's width, chip select low with a non-idle phase or a count, reserved flag bits, or a control byte outside bits 3 and 7 is refused with the instance unchanged"
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^eeprom\\.state\\.bounds$' exit 0"
        status: pass
    human_judgment: false
  - id: D9
    description: "docs/saves.md states the board's wiring, byte order, immediate ready, once-per-command generation rule and that wiring and chip model are not verified on real hardware"
    requirement: SAVE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-11
---

# Phase 7 Plan 3: The EEPROM board Summary

**The Sega EEPROM board (board 10) plays and saves through the existing battery API: a `$8000` window wired to the 93C46, Sega-style paging in its own ext arm, 128 bytes of erased cart RAM, a bounded per-bit state visit and a docs section, with `mapper.c` byte-identical to main.**

## Performance

- **Duration:** about 5 min by the wall clock of this run
- **Started:** 2026-11-11T01:09:17Z
- **Completed:** 2026-11-11T01:14:30Z
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- `src/mapper_ext.c`: one arm per switch for `COGMASTER__BOARD_SEGA_EEPROM`, citing MAME `sega8_eeprom_device` at pin `1d7a454ed79e`. Read: the window at exactly `$8000` while `$FFFC` bit 3 is set, the fixed first 1 KiB, else the 16 KiB bank of the slot. Write: `$FFFC` keeps bits 3 and 7 and bit 7 resets the chip; `$FFFD`-`$FFFF` set the three banks modulo the page count with the mapper_range rule through a small static helper; a window write calls `cogmaster__eeprom_write` and moves the generation when it returns 1; any other write below `$C000` records rom_write. Reset: banks 0, 1, 2 and a reset chip. `src/mapper.c` and `src/battery.c` are unchanged against `origin/main`.
- `src/cart.c`: board 10 always gets 128 bytes of cart RAM filled with FF; the fact-to-info comment no longer says EEPROM has no persistence.
- `src/internal.h`: the internal board enum entry and `struct cogmaster__eeprom eeprom` in the instance.
- `src/state.c`: `ctrl_max` 0x88 for board 10 plus a cross-check that no other bit is set; the bank8 and raw visit is limited to the Korean family through the 4 PAK; the chip is visited as `eeprom_last`, `eeprom_phase`, `eeprom_count`, `eeprom_cmd`, `eeprom_data` and `eeprom_flags` with per-phase count bounds and the rule that chip select low means idle with count 0.
- Eight cases in `tests/mapper/test_eeprom.c`, registered as `eeprom.board.*` and `eeprom.state.*` with the ci and wasm32 inventories in step.
- `docs/saves.md`: a 93C46 EEPROM section.

## Task Commits

1. **Task 1: The EEPROM board arm, its cart RAM and register-level tests** - `4dcf223` (feat)
2. **Task 2: State bounds for board 10 and mid-command round trips** - `e8a3d78` (feat)
3. **Task 3: The EEPROM board in docs/saves.md** - `5a2b825` (docs)

The device was proven in plan 01 and the board cases passed on the first run, so no separate failing-test commit was made; the negative control below shows the state case detects a missing check.

## Sizeof tripwire

`_Static_assert(sizeof(struct cogmaster__eeprom) == 8u, ...)` sits beside the mapper tripwire in `src/state.c` (7 bytes of fields plus one byte of padding).

## Negative control (rule 7)

A temporary edit to `src/state.c` replaced the chip-select-low cross-check with `(void)flags;`, rebuilt with `cmake --build --preset ci`, then reverted.

| Fault planted | Command | Exit |
| --- | --- | --- |
| the CS-low implies idle and count 0 check removed | `ctest --test-dir build/ci -R '^eeprom\.state\.bounds$'` | 8 (fails) |
| check restored, rebuilt | `ctest --test-dir build/ci -R '^eeprom\.'` | 0 |

Fault prevented: a state that loads a chip with chip select low but mid-command, which would resume as a command no clock stream could have started.

## Evidence

All commands from the repository root; no run URL (a local run, nothing pushed by this plan).

- `cmake --preset ci` then `cmake --build --preset ci`: exit 0.
- `ctest --test-dir build/ci -R '^(eeprom\.|mapper\.|battery\.|fixtures\.|abi\.(globals|undefined|symbols)$|libretro\.)'`: exit 0 (105 of 105 passed after Task 1).
- `ctest --test-dir build/ci -R '^(eeprom\.|mapper\.|battery\.|core\.state|fuzz\.regress\.)'`: exit 0 (45 of 45 passed after Task 2).
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake`: exit 0, 723 check lines of 8268 src lines (no check code added).
- `ctest --test-dir build/ci -R '^(docs\.|policy\.)'`: exit 0 (35 of 35).
- `cmake --workflow --preset ci`: exit 0. `cmake --workflow --preset hygiene`: exit 0.
- `git diff --quiet origin/main -- src/mapper.c src/battery.c tests/audio tests/testcard_pins.cmake tests/built`: exit 0.
- Task 1 and Task 2 acceptance greps all held: three uses of the board constant, `sega8_eeprom` and the pin in `src/mapper_ext.c`, one `sizeof(struct cogmaster__eeprom)` and `0x88u` in `src/state.c`, eight inventory lines per lane, and the three docs phrases in `docs/saves.md`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] The generation moves only while a battery span exists**
- **Found during:** Task 1
- **Issue:** the state visit bounds the generation at zero when no supported persistence exists, so an EEPROM board built with a non-battery persistence value would write a state it could not load.
- **Fix:** the window write increments the generation only when `cogmaster__battery_length` is nonzero, the same guard `cogmaster__battery_write` uses.
- **Files modified:** `src/mapper_ext.c`
- **Committed in:** `4dcf223`

**2. [Rule 1 - Bug] Tighter chip count bounds than the plan listed**
- **Found during:** Task 2
- **Issue:** the plan's widths (command 8, data 16, read 17) are not the values the device holds: a command's count restarts at 8 and the read count never passes 15 (plan 01).
- **Fix:** the visitor accepts command 0 to 7, data in and read 0 to 15 and every other phase 0, which still accepts every state the all-counts round trip reaches.
- **Files modified:** `src/state.c`
- **Committed in:** `e8a3d78`

---

**Total deviations:** 2 auto-fixed (1 missing critical, 1 bug). **Impact:** none on scope; both keep the saved state exactly loadable.

## Issues Encountered

- The first negative control attempt deleted the check line, which left an unused variable and a failed build, so the old binary passed; the control was redone by replacing the check with `(void)flags;` and failed as expected.
- A `-Wsign-conversion` error in the new test's bit shift was fixed by casting before the shift.
- The local `asan` preset was not run (clang-21 may be absent); CI runs it. The new code has no heap use beyond the test's malloc and bounded loops.

## Known Stubs

None.

## Threat Flags

None. The board adds no endpoint, file access or trust boundary; the one new input, a loaded state, is bounded field by field.

## Next Phase Readiness

Plan 07-04 can flip the database rows and the generator to board 10 with persistence 2, geometry 1 and 128 bytes of RAM, and add the fixture facts row; the arm, the RAM rule and the state visit are in place. The runner, libretro and import rules need no change for the span itself.

## Self-Check: PASSED

- The three task commits `4dcf223`, `e8a3d78` and `5a2b825` are ancestors of HEAD.
- `src/mapper_ext.c`, `src/state.c`, `src/cart.c`, `src/internal.h`, `tests/mapper/test_eeprom.c`, `tests/mapper/tests.cmake`, both inventories and `docs/saves.md` exist and carry the planned changes.

---
*Phase: 07-saves-survive-a-restart*
*Completed: 2026-11-11*
