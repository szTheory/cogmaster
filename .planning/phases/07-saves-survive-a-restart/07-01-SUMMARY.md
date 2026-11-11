---
phase: 07-saves-survive-a-restart
plan: "01"
subsystem: cartridge-eeprom
tags:
  - eeprom
  - 93c46
  - state-machine
  - differential-test
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: the board enum, the 128-byte-capable cartridge RAM and battery span the device image will live in
provides:
  - src/eeprom.c, the 93C46 serial EEPROM as a pure state machine with reset, one window write, one window read and one helper pair that owns the little-endian word order
  - struct cogmaster__eeprom in src/internal.h with every field bounded, and the three cogmaster__eeprom_ declarations
  - tests/mapper/test_eeprom.c with the cases commands, lock, read, changes and differential, registered as eeprom.* on the ci and wasm32 lanes
affects: [07-02, 07-03]

actuals:
  tokens: 6700
  tasks: 2
  commits: 2

plan_head_before: efd1cbb4a698b3a60950ec948a39f549bc7458b7
plan_head_after: 82c9233428812bdb04067f7517a3fd2a0e51f7ac
commits: 2

tech-stack:
  added: []
  patterns:
    - "A cartridge device is built as a pure module over one window byte and an image, proven with no board attached, before the board only wires pins"
    - "A datasheet-written command-level model is compared with the bit-level device on seeded chip-select-framed random frames after every frame"

key-files:
  created:
    - src/eeprom.c
    - tests/mapper/test_eeprom.c
  modified:
    - src/internal.h
    - CMakeLists.txt
    - tests/mapper/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "EWEN and EWDS take effect on the clock that completes them; WRITE, ERASE, WRAL and ERAL run when chip select falls, which is where the data sheet's self-timed programming cycle starts, so a change is reported exactly once at that fall"
  - "Ready is immediate (option A): Data Out is high except while a READ shifts data, and the dummy bit shows low the moment the 8th command bit is clocked"
  - "A write-type command given while writes are disabled still takes its clocks, so framing stays right, but is never marked pending and changes nothing"
  - "READ keeps its position in cmd, so the sequential read wraps from word 63 to word 0 without a separate address register, and its count never passes 15"
  - "No MAME text was copied; the cartridge wiring is for the board plan, so no PROVENANCE.md line"

requirements-completed: []

coverage:
  - id: C1
    description: "The device powers up write-disabled, refuses WRITE, ERASE, WRAL and ERAL until EWEN and again after EWDS, and READ ignores the write-enable latch"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^eeprom\\.commands$' exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "READ shifts a dummy 0 then D15 to D0 and continues past word 63 to word 0; WRITE and ERASE touch one word; WRAL overwrites every word with the clocked data and not the data ANDed with the old word; ERAL sets every byte to FF"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^eeprom\\.(commands|read)$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "A change is reported exactly once per completed write-type command that altered a byte, at chip select falling, and never per clock edge or for a same-data write"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^eeprom\\.changes$' exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "Chip select low abandons a command, clocks before the start bit and after a complete command are ignored, the write-enable latch survives chip select low, and the window byte is the last write masked with FC, or 02, or Data Out"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^eeprom\\.(lock|read)$' exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "A data-sheet model applied a command at a time agrees with the device on image, change reports and READ bit stream over 2000 seeded chip-select-framed frames including abandoned commands and extra clocks"
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^eeprom\\.differential$' exit 0"
      status: pass
    human_judgment: false

duration: 45min
completed: 2026-11-10
---

# Phase 7 Plan 1: The 93C46 device Summary

**A pure 93C46 serial EEPROM state machine (64 little-endian words in a 128-byte image, immediate ready, one change report per completed write-type command) proven with closed-form command cases and a seeded data-sheet differential model, no board attached.**

## Performance

- **Duration:** about 45 min of work in this run (a prior run died on a rate limit before committing)
- **Completed:** 2026-11-10
- **Tasks:** 2
- **Files modified:** 7 (two new)

## Accomplishments

- `src/eeprom.c`: `cogmaster__eeprom_reset`, `cogmaster__eeprom_write` and `cogmaster__eeprom_read`. A window write takes DI from b0, CLK from b1 and CS from b2, applies DI, then CS, then CLK. Each write is O(1) apart from the bounded 64-word WRAL and ERAL. No globals, no allocation, no function-pointer table.
- `struct cogmaster__eeprom` is 8 bytes (`sizeof` measured with `cc`; 7 bytes of fields plus one byte of padding). Bounds: command at most 8 clocks after the start bit, WRITE and WRAL data at most 16, READ at most 15 in the count (the dummy bit shows when the command completes, so a read never counts more than the 17 bits it shows). Chip select low means idle with count 0.
- Five test cases built into one executable, `cogmaster-test-eeprom`, registered as `eeprom.commands`, `eeprom.lock`, `eeprom.read`, `eeprom.changes` and `eeprom.differential` (labels mapper and portable, on the ci and wasm32 inventories).
- Data sheet cited: Microchip 93AA46/93LC46/93C46 1K Microwire serial EEPROM data sheet, x16 organisation: instruction set table, READ, EWEN and EWDS, WRITE, ERASE, WRAL, ERAL, the Data Out pin and the programming cycle. No MAME text was copied (MAME is only cited for the cartridge wiring in plan 07-03), so no PROVENANCE.md line was added.

## Task Commits

1. **Task 1: The 93C46 device and its closed-form command cases** - `bfaaffd` (feat)
2. **Task 2: Change counting, the differential model and test registration** - `82c9233` (test)

The RED step was folded into the task commits: the plan keeps the build and the configure-time inventory green at every commit, so the test and the device landed together, and Task 2's negative controls show the cases fail when the device is wrong.

## Retry handling

A previous run died before committing anything. The uncommitted edits it left were reviewed against the plan and kept: the `struct cogmaster__eeprom` block and its three declarations in `src/internal.h` (fields, bounds and comment matched the plan and were used as they stand) and the `src/eeprom.c` line in `CMakeLists.txt`. The file `src/eeprom.c` and the test did not exist, so they were written here. The only other leftover, a `.planning/STATE.md` stopped-at line reading "context exhaustion", is overwritten by this plan's state update. Nothing was dropped.

## Negative controls (rule 7)

Both run from the repository root against a temporary edit of `src/eeprom.c`, rebuilt with `cmake --build --preset ci`, and reverted afterwards.

| Fault planted | Command | Exit |
| --- | --- | --- |
| a change reported on every data clock | `ctest --test-dir build/ci -R '^eeprom\.changes$'` | 8 (fails) |
| WRAL writes the data ANDed with the old word | `ctest --test-dir build/ci -R '^eeprom\.differential$'` | 8 (fails; `eeprom.commands` fails too) |
| both reverted | `ctest --test-dir build/ci -R '^eeprom\.'` | 0 |

## Evidence

All commands from the repository root; no run URL (a local run, nothing pushed by this plan).

- `cmake --preset ci` then `cmake --build --preset ci`: exit 0.
- `ctest --test-dir build/ci -R '^(eeprom\.|mapper\.|battery\.)'`: exit 0, 29 of 29 passed.
- `ctest --test-dir build/ci -R '^(abi\.(globals|undefined|symbols)|mapper\.|battery\.)'`: exit 0.
- `cmake --workflow --preset ci`: exit 0.
- `cmake --workflow --preset hygiene`: exit 0.
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake tests/built`: exit 0 (no pin moved).
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, 723 check lines of 8171 src lines, strict one tenth (no check code added).

## Deviations from Plan

None - plan executed exactly as written. The Task 2 timeout comment records "slowest of 1 run" because the cases take under 0.01 s and only one run was timed.

## Issues Encountered

- A first build failed on `-Wsign-conversion` in two shift expressions and one test expression; fixed by casting before the shift. No behaviour change.
- The local `asan` preset was not run (clang-21 may be absent); CI runs it. The new executable has no heap use and bounded loops.

## Known Stubs

None.

## Threat Flags

None. The module takes only a byte and a 128-byte image; no endpoint, file or trust boundary is added.

## Next Phase Readiness

Plan 07-03 can wire the device to the cartridge window: the three functions, the struct and the 128-byte little-endian image contract are fixed. The board arm still needs the `COGMASTER_BOARD_SEGA_EEPROM` constant, an `eeprom` member in the mapper state, the state-format fields and the `ctrl_max` change described in the phase context.

## Self-Check: PASSED

- `src/eeprom.c` and `tests/mapper/test_eeprom.c` exist; commits `bfaaffd` and `82c9233` are ancestors of HEAD.
- All acceptance greps held: `cogmaster-test-eeprom` appears 5 times in tests.cmake, `NAME eeprom\.` is a loop registration and not a literal, `cogmaster__eeprom_` appears 3 times in internal.h, no planning IDs in the new files, the data sheet is cited, `Fault prevented: a 93C46` appears once, and both inventories carry the five names with labels `mapper,portable`.

---
*Phase: 07-saves-survive-a-restart*
*Completed: 2026-11-10*
