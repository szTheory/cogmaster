---
phase: 06-korean-cartridges-play
plan: "01"
subsystem: cartridge-generator
tags:
  - cartdb
  - slot-table
  - catalog
  - compact-rows
status: complete

requires:
  - phase: 05-tune-up
    provides: expect_fail.cmake negative controls, the counted check budget and the quality gates
provides:
  - tools/cartdb.c with one static slot table of 20 stable slot ids, each a board constant or an unsupported reason
  - per-slot seen and emitted counts on stderr, sorted by name, counted before multipart and compound exclusion
  - compact output rows (20-byte binary SHA-1 plus fixed-width integers, no title) and a separate catalog file of public rows
  - --list-slots, --catalog PATH and --pin HEX40 options; slot, ram_bytes, ctrl1, ctrl2 and eeprom persistence facts
  - cart.generator on authored XML with the zz_new unmapped-slot negative control
affects: [06-03, 06-05, 06-06, 06-07]

actuals:
  tokens: 5300
  tasks: 2
  commits: 2

plan_head_before: 9585da2e0920ef4afc459eb4854cdd0809fad99b
plan_head_after: cab60da2cdbde0764210e4cf0d65cd447ab63019
commits: 2

tech-stack:
  added: []
  patterns:
    - "A generator takes its board numbers from the public header, so a slot row can never name a board the header lacks"
    - "A table self-check runs before any input is read; a negative control builds its bad input from the same authored XML plus one record"

key-files:
  created: []
  modified:
    - tools/cartdb.c
    - tests/cart/generator.cmake
    - tests/cart/tests.cmake

key-decisions:
  - "The six Korean-board slots (korean, korean_nb, hicom, zemina, nemesis, 4pak) stay board 0 with the reason mapper arm not present until each arm lands, so no row names a board without an arm"
  - "Slot ids are the row position plus one and are checked by the start-up self-check, so a reordered or duplicated row fails before any output"
  - "emitted counts the unique rows that reach the table, so a multipart or incomplete record adds to seen only"
  - "The catalog board name for board 1 is none and for board 0 is unsupported; names come from designated initializers keyed by the header constants"
  - "The committed src/cartdb.inc is untouched; plan 06-03 regenerates it with the library side"

requirements-completed: []

coverage:
  - id: C1
    description: "Every slot of the table maps through one static table; an authored XML with one software per listed slot exits 0 and prints a seen=1 emitted=1 line for each"
    requirement: MAP-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$' exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "A slot not in the table (zz_new) exits 1 naming it, through the shared negative-control script; an unknown controller and an oversized RAM size exit 1 too"
    requirement: MAP-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$' exit 0 (expect_fail.cmake with CODE 1)"
      status: pass
    human_judgment: false
  - id: C3
    description: "A multipart record carrying slot korean raises seen to 2 and emitted stays 1; the --list-slots listing is sorted and holds 20 names without the dead sega slot"
    requirement: MAP-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$' exit 0; build/ci/cogmaster-cartdb --list-slots piped through awk NR==20 exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "Output rows are integer-only designated initializers with a binary SHA-1 and no title; titles go to a catalog file of public rows with entities decoded and the owned fixture row absent"
    requirement: MAP-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$' exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "An eeprom slot record emits persistence 3 and sharedfeat controllers emit ctrl1 and ctrl2, with row counts equal to what the authored XML carries; an alias without a device keeps the device and two different devices conflict"
    requirement: MAP-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$' exit 0"
      status: pass
    human_judgment: false
  - id: C6
    description: "The whole plan builds and every existing check still passes with src/cartdb.inc unchanged"
    requirement: MAP-01
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (729 of 745)"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 01: Cartridge generator slot table and compact rows Summary

The cartridge generator now maps every MAME slot through one static table of 20 stable ids to a board constant or an unsupported reason, fails on an unknown slot or controller, counts every slot including excluded records, and emits integer-only rows (binary SHA-1 plus fixed-width integers) with titles moved to a separate catalog file.

## What was built

- `tools/cartdb.c`: the slot table (`codemasters` is `COGMASTER_BOARD_CODEMASTERS`; the six Korean-board slots, `eeprom`, `mgear`, `xterminator`, the seven SG-1000 slots, `janggun`, `korean_188in1` and `seojin` are board 0 with a reason), a start-up self-check (mapped and unsupported are exclusive, no duplicate name, ids equal row position plus one, reasons plain ASCII within 95 bytes), the dead `rom` and `sega` slot branches removed, a pre-exclusion scan of every `slot` feature and `ctrl1_default` / `ctrl2_default` sharedfeat, eeprom persistence 3, a 16-bit RAM size check, alias coalescing that keeps a device and rejects two different ones, `--list-slots`, `--catalog` and `--pin`.
- `tests/cart/generator.cmake`: rewritten around authored XML. It keeps the conflicting-identity, CC0, determinism, sort, duplicate and compound cases and adds the per-slot count run, the multipart `seen=2 emitted=1` run, the `zz_new`, `zz_device` and oversized-RAM controls through `expect_fail.cmake`, eeprom and controller counts, the catalog shape and the owned fixture row staying out of the catalog.
- `tests/cart/tests.cmake`: include directories for `cogmaster-cartdb` and `-DFACT` for the generator test (needed by the fixture-row assertion).

## Table format

For plan 06-03 to compile (the pin shown is the committed MAME pin).

First line:

```
/* Generated by tools/cartdb.c from the MAME hash lists at 1d7a454ed79e97d35d2a9f127ac029ead94711d6; CC0 facts, see PROVENANCE.md. */
```

Then `static const struct cogmaster__cart_slot cartdb_slots[] = {`, one row per slot in id order, and `};`:

```
{15u, 0u, "korean", "mapper arm not present"},
```

Fields are id, board, name, reason (empty string when the board is non-zero). Then `static const struct cogmaster__cart_fact cartdb_facts[] = {`, one row per unique SHA-1 in sorted order, and `};`:

```
{.sha1 = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11}, .bytes = 32768u, .ram_bytes = 8192u, .board = 2u, .slot = 0u, .machine = 2u, .region = 0u, .video = 1u, .persistence = 2u, .geometry = 1u, .ctrl1 = 0u, .ctrl2 = 0u, .flags = 1u},
```

Fact rows hold no title and no pointer. Slot ids: 1 codemasters, 2 eeprom, 3 mgear, 4 xterminator, 5 cardcatcher, 6 castle, 7 dahjee_typea, 8 dahjee_typeb, 9 level3, 10 othello, 11 terebi, 12 4pak, 13 hicom, 14 janggun, 15 korean, 16 korean_188in1, 17 korean_nb, 18 nemesis, 19 seojin, 20 zemina. Persistence values: 0 unknown, 1 none, 2 battery, 3 EEPROM. Controller codes: 0 none, 1 lphaser, 2 paddle, 3 sports, 4 graphic. The catalog is tab-separated: sha1, machine (sg, sms, gg), board name (unsupported, none, sega, codemasters), slot name or `-`, title; `board_names` in the tool grows with each new board constant.

A non-committed run over the three pinned lists in a temporary directory exited 0 with all 20 slots seen (for example codemasters seen=22 emitted=21, korean seen=6, hicom seen=9, zemina seen=8, eeprom seen=7), no unmapped slot or controller, and the committed `src/cartdb.inc` unchanged. That is a head start for 06-03, not evidence for this plan.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The generator test needed the fixture facts path**
- **Found during:** Task 2
- **Issue:** The plan's fixture-row assertion needs `-DFACT`, which `cart.generator` did not pass.
- **Fix:** Added `-DFACT=${CMAKE_CURRENT_LIST_DIR}/fixture_facts.txt` to the existing `cart.generator` command in `tests/cart/tests.cmake` (already a listed file).
- **Files modified:** tests/cart/tests.cmake
- **Commit:** 0c1ab39

**2. [Rule 1 - Bug] Two first-draft faults in the new test and tool**
- **Found during:** Task 1 and 2 verification
- **Issue:** a `-Wshadow` error (a local named `s` in `parse`), and the duplicate-count regex matched each byte of a SHA-1 rather than the row.
- **Fix:** renamed the local to `row`; the regex now anchors on `sha1 = {0x11,`.
- **Files modified:** tools/cartdb.c, tests/cart/generator.cmake
- **Commit:** 0c1ab39, cab60da

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug). **Impact:** none on scope.

### Process note

Task 1 landed before Task 2, so the commit 0c1ab39 leaves `cart.generator` failing against the old test until cab60da (every commit builds; the plan keeps the old `src/cartdb.inc`). I wrote the test first and the tool second in the working tree but committed in task order, so the RED step was not a separate commit.

## Verification

- `cmake --preset ci > /dev/null && cmake --build --preset ci --target cogmaster-cartdb && build/ci/cogmaster-cartdb --list-slots` piped through `awk 'END {exit NR != 20}'`, `grep -qx korean_nb`, `! grep -qx sega`: exit 0
- `ctest --test-dir build/ci -R '^cart\.'`: exit 0 (8 tests)
- `cmake --workflow --preset ci`: exit 0
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, 729 check lines of 745 (no counted check file changed)
- Acceptance greps: dead `rom`/`sega` slot branches 0; `COGMASTER_BOARD_` references 4; `git diff --quiet origin/main -- src/cartdb.inc` and `-- tests/inventory` both exit 0; `unmapped slot zz_new`, `expect_fail.cmake` and `Fault prevented` each present in `tests/cart/generator.cmake`.

## Known Stubs

None. The six Korean-board slot rows carry the reason "mapper arm not present" by design; plans 06-06 and 06-07 flip them.

## Threat Flags

None. The tool reads local XML and writes local files only; no network, auth or trust-boundary surface was added.

## Self-Check: PASSED

- FOUND: tools/cartdb.c, tests/cart/generator.cmake, tests/cart/tests.cmake
- FOUND commits: 0c1ab39, cab60da
