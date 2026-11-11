---
phase: 07-saves-survive-a-restart
plan: "05"
subsystem: fixtures
tags:
  - eeprom
  - fixture
  - cartdb
  - z80
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-03 Sega EEPROM board and plan 07-04 board-10 table rows
provides:
  - an owned WLA-DX Game Gear image, eeprom.gg, that drives the 93C46 the way a game does and reports through the mailbox
  - a second fixture facts row (board 10, Game Gear, persistence 2, declared 128, geometry 1) validated by the generator
  - the regenerated table holding the eeprom fixture, the eighth changed row since the phase began
affects: [07-06]

actuals:
  tokens: 14000
  tasks: 2
  commits: 2

plan_head_before: a619d8e56735bed51f31b0e879d6ceab000899f9
plan_head_after: 7ca035277a095035659bc5857fbbb84353a6c5be
commits: 2

tech-stack:
  added: []
  patterns:
    - "fixture.cmake ROW picks which fixture facts line an image's identity check compares"

key-files:
  created:
    - tests/roms/src/eeprom.asm
    - tests/roms/src/eeprom.link
  modified:
    - tests/fixtures/tests.cmake
    - tests/cart/fixture_facts.txt
    - tests/cart/fixture.cmake
    - tools/cartdb.c
    - src/cartdb.inc
    - tests/cart/test_cartdb.c
    - tests/built/expected.txt
    - tests/roms/manifest.txt

key-decisions:
  - "The fixture's region fact is 1 (export), which is what Game Gear header region code 6 means"
  - "The generator accepts exactly two owned-row shapes, the battery one and the EEPROM one, and refuses any other row"

requirements-completed: []

coverage:
  - id: E1
    description: "eeprom.gg assembles from owned source, its SHA-256 matches tests/built/expected.txt, and no ROM or save bytes are committed"
    requirement: SAVE-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(fixtures\\.|built\\.|hygiene\\.|policy\\.)'"
        status: pass
    human_judgment: false
  - id: E2
    description: "The fixture's production SHA-1 is row 2 of tests/cart/fixture_facts.txt, the build's identity check compares it with ROW=2, and the table pins the row's facts"
    requirement: SAVE-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(cart\\.|fixtures\\.|eeprom\\.|battery\\.)'"
        status: pass
    human_judgment: false
  - id: E3
    description: "cartdb --fixture refuses an owned row of another shape, shown by a copy of the facts file whose row 2 says declared 64"
    requirement: SAVE-01
    verification:
      - kind: other
        ref: "build/ci/cogmaster-cartdb --pin <commit> <three lists> --fixture <copy with declared 64>, exit 1 with invalid owned fixture facts"
        status: pass
    human_judgment: false
  - id: E4
    description: "The built runner runs the fixture twice into an empty save directory and the save carries 34 12 then the counter, 01 00 then 02 00"
    requirement: SAVE-01
    verification:
      - kind: other
        ref: "build/ci/cogmaster-run build/ci/fixtures/eeprom.gg --frames 60 --save-dir <dir>, twice, exit 0 each; od of the first four save bytes"
        status: pass
    human_judgment: false
---

# Phase 7 Plan 05: The owned EEPROM fixture Summary

An owned Game Gear program now reads, increments, writes, polls, reads back and write-protects a 93C46 through the cartridge window, and the production table recognises its image by SHA-1 as a 128-byte board-10 cartridge.

## What was built

- `tests/roms/src/eeprom.asm` and `eeprom.link`: four 16 KiB banks like `battery.asm`, Game Gear header (region code 6), ROM id 10. It opens the window with `$FFFC` = `$08`, reads word 0 (the signature `$1234`) and word 1 (the counter), treats a missing signature as a fresh chip, then EWEN, WRITE word 1 = counter + 1, raises chip select and polls Data Out until high, READs the word back, EWDS, and tries a WRITE of `$BEEF`. The mailbox gets four records: 1 the new counter, 2 read-back equals written, 3 the post-EWDS write changed nothing, 4 ready seen while polling. Small subroutines (`ee_bit`, `ee_cmd`, `ee_data`, `ee_read`, `ee_write`) keep the code short; the wiring is in `docs/saves.md`.
- `tests/fixtures/tests.cmake`: `eeprom` joins the fixture list with extension `gg`; its build step runs `fixture.cmake` with `HASH_ONLY=1` and `ROW=2`, so a changed assembler output fails the build. The battery image uses `ROW=1`.
- `tests/cart/fixture.cmake`: `ROW` (default 1) picks the fact line compared; blank lines are skipped.
- `tests/cart/fixture_facts.txt`: row 2, `0170c40f75d8ed3f3bbc9547fa2d9e89192fc3f0 65536 10 2 1 2 2 128 1 cogmaster eeprom fixture`.
- `tools/cartdb.c`: `--fixture` reads rows to end of file; a row must match the battery shape or the EEPROM shape (65536 bytes, board 10, Game Gear, persistence 2, declared 128, geometry 1), otherwise "invalid owned fixture facts". An unparsable line after the first row is still "extra fixture facts".
- `src/cartdb.inc`: regenerated with the pinned lists (SHA-256 of each list equals its `pins.txt` row); one row added, 1658 rows. Against `origin/main` the table now has 8 added and 7 removed fact lines: the seven EEPROM rows of plan 07-04 plus this one.
- `tests/cart/test_cartdb.c`: a hash lookup of the fixture gives board 10, BATTERY, 128 bytes, SUPPORTED, Game Gear; persistence-2 count 70 to 71, ctrl1 zero-count 1636 to 1637, ctrl2 zero-count 1652 to 1653 (the fixture row has default controllers).
- `tests/built/expected.txt` (one added line, `eeprom 1f7ed7d1...`) and `tests/roms/manifest.txt` (one comment line). No image is committed; `git ls-files -- '*.gg' '*.srm'` lists nothing.

## Evidence

- Negative control: `build/ci/cogmaster-cartdb --pin 1d7a454ed79e97d35d2a9f127ac029ead94711d6 build/cartdb-input/sms.xml build/cartdb-input/gamegear.xml build/cartdb-input/sg1000.xml --fixture <copy of fixture_facts.txt with row 2 declared 64>` exits 1 and prints `cartdb: invalid owned fixture facts`.
- Runner, first run: `build/ci/cogmaster-run build/ci/fixtures/eeprom.gg --frames 60 --save-dir <fresh dir>` exit 0; `eeprom.srm` is 128 bytes, first four bytes `34 12 01 00`.
- Runner, second run into the same directory: exit 0; first four bytes `34 12 02 00`. The counter reaching 2 also shows the rejected `$BEEF` write after EWDS changed nothing. The recurring test is plan 07-06's.
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: 723 check lines of 8268 src lines (no check code was added).
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0; `git diff origin/main -- tests/built` has no removed line.
- `clang-format` 21 reports nothing on `tools/cartdb.c` and `tests/cart/test_cartdb.c`.
- No CI run URL: the branch was not pushed in this plan, and the plan names no dispatch.

## Task Commits

1. **Task 1: The eeprom.asm fixture and its build** - `a869acf` (feat)
2. **Task 2: The second facts row, its validation and the regenerated table** - `7ca0352` (feat)

**Plan metadata:** the docs commit that holds this file.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None.

## Threat Flags

None. The change adds an owned test image, a facts row and a stricter parser of a build-time input; no network, auth or trust-boundary surface.

## Self-Check: PASSED

- FOUND: tests/roms/src/eeprom.asm, tests/roms/src/eeprom.link, tests/cart/fixture_facts.txt, tests/cart/fixture.cmake, tools/cartdb.c, src/cartdb.inc, tests/cart/test_cartdb.c, tests/built/expected.txt, tests/roms/manifest.txt
- FOUND commits: a869acf, 7ca0352
