---
phase: 06-korean-cartridges-play
plan: "03"
subsystem: cartridge-database
tags:
  - cartdb
  - compact-rows
  - catalog
  - unsupported-rows
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-01 generator with slot table, compact rows and catalog output; plan 06-02 API 10 cart_info fields
provides:
  - src/cartdb.inc regenerated from the three pinned CC0 lists plus the owned fixture fact, as 36-byte binary rows and 20 slot rows
  - src/cartdb.c with static asserts on the row size, its offsets and a table under 100,000 bytes, and a binary search over 20-byte memcmp
  - cogmaster__cart_fact_to_info, driven in a test with the real Seo Jin, 188-in-1 and Janggun rows
  - tests/compat/catalog.tsv with sha1, machine, board name, slot and title for every public row, and a compat update that reads titles from it
affects: [06-05, 06-06, 06-07, 06-08, 06-09]

actuals:
  tokens: 9500
  tasks: 2
  commits: 2

plan_head_before: 7c5216e8c79c2533c5e9ed54abaad6a4005c8324
plan_head_after: 1384815a13075984adc36339fbd1cef7333e69cc
commits: 2

tech-stack:
  added: []
  patterns:
    - "Read-only static const tables with no pointer, so the library keeps no writable data"
    - "An equivalence capture of the old table before regeneration, compared by comm afterwards"

key-files:
  created:
    - tests/compat/catalog.tsv
  modified:
    - src/cartdb.inc
    - src/cartdb.h
    - src/cartdb.c
    - src/cart.c
    - tests/cart/test_cartdb.c
    - scripts/compat-common.sh
    - scripts/compat-update.sh
    - docs/boards.md
    - docs/runner.md
    - docs/conformance.md
    - include/cogmaster.h

key-decisions:
  - "Persistence value 3 (EEPROM) reads as unknown in the probe, since EEPROM has no public value until the saves phase"
  - "The slot-by-name lookup is a bounded loop, because the library may not link strcmp (abi.undefined)"
  - "The row counts per persistence 3, ctrl1 and ctrl2 are constants in cart.database, equal to the regeneration's counts"

requirements-completed: []

coverage:
  - id: C1
    description: "src/cartdb.inc is regenerated from the pinned lists: first line names the pinned MAME commit, 1657 unique fact rows of 36 bytes with no string, 20 slot rows"
    requirement: MAP-02
    verification:
      kind: integration
      ref: "build/ci/cogmaster-cartdb with the pin, catalog and fixture arguments exit 0; head -1 src/cartdb.inc piped through grep exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "cartdb.c static-asserts the row size 36, the offsets and a table under 100,000 bytes (59,652 bytes), and finds rows by binary search over 20-byte memcmp"
    requirement: MAP-02
    verification:
      kind: unit
      ref: "cmake --build --preset ci exit 0; ctest --test-dir build/ci -R '^cart\\.database$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "The Seo Jin, 188-in-1 and Janggun rows probe as ERR_UNSUPPORTED_BOARD with board 0, their slot id and a reason that starts with the slot name and holds the named text"
    requirement: MAP-05
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.database$' exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "tests/compat/catalog.tsv holds 1656 five-field public rows including the named-unsupported rows, and compat.public_update reads the Crazy Faces title from it"
    requirement: MAP-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^compat\\.' exit 0; awk over tests/compat/catalog.tsv for seojin, korean_188in1 and janggun exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "A hash hit leaves title empty and sets board_source to the database; the owned battery fixture still hits and keeps its battery facts; no frame, audio or built pin moves"
    requirement: MAP-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.fixture_identity$' exit 0; git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0; git diff origin/main -- tests/built exit 0 with 0 removed lines"
      status: pass
    human_judgment: false
  - id: C6
    description: "Every row of the old table survives regeneration with equal bytes, board, machine, region, video, persistence (EEPROM rows excepted), ram bytes, geometry and public flag"
    requirement: MAP-01
    verification:
      kind: other
      ref: "comm -23 build/cartdb-input/old.m build/cartdb-input/new.facts exit 0; test ! -s build/cartdb-input/lost.facts exit 0"
      status: pass
    human_judgment: false
  - id: C7
    description: "The whole plan passes the full local workflow and the check-line budget"
    requirement: MAP-02
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines)"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 03: Compact table, catalog and named unsupported rows Summary

The library now reads a regenerated 36-byte-row table by binary search over 20-byte digests, a hash hit reports source DB with an empty title, and the Seo Jin, 188-in-1 and Janggun rows probe through the real table as unsupported with a named reason; titles live in `tests/compat/catalog.tsv`.

## What was built

- `src/cartdb.inc`: regenerated by `build/ci/cogmaster-cartdb --pin 1d7a454ed79e97d35d2a9f127ac029ead94711d6 --catalog tests/compat/catalog.tsv build/cartdb-input/sms.xml build/cartdb-input/gamegear.xml build/cartdb-input/sg1000.xml --fixture tests/cart/fixture_facts.txt` (exit 0, from the repository root, lists fetched with `scripts/fetch-pin.sh` which verified each SHA-256). 1657 unique fact rows (1670 candidates, 21 compound or incomplete records excluded; 1657 equals the old table), 20 slot rows. `grep '\.sha1 = {' src/cartdb.inc | grep -c '"'` prints 0.
- `src/cartdb.h` and `src/cartdb.c`: the 36-byte `cogmaster__cart_fact`, the `cogmaster__cart_slot` row, `_Static_assert` on size, offsets and `sizeof cartdb_facts < 100000u` (1657 x 36 = 59,652 bytes), `cogmaster__cartdb_find` (binary search), `_image`, `_row`, `_slot` and `_slot_named`.
- `src/cart.c`: `cogmaster__cart_fact_to_info` (public identity from flags bit 0, source DB, empty title, SHA-1 as lowercase hex, EEPROM reads as unknown, board 0 names slot id and `<slot>: <reason>`); `cogmaster__cart_probe` calls it on a hit.
- `tests/compat/catalog.tsv` (1656 public rows; the owned fixture row stays out), read by `scripts/compat-update.sh` through `COMPAT_CATALOG`; the octal title decoder is deleted.
- `tests/cart/test_cartdb.c`: the real named rows, slot rows exclusive board or reason, each supported board owns a row, per-value counts, and in the fixture branch an empty title and source DB.
- Docs: `docs/boards.md`, `docs/runner.md` and the one stale line in `docs/conformance.md`.

## Slot counts

From the regeneration's stderr (`cartdb: slot ...` lines):

```
slot 4pak seen=1 emitted=1 -> unsupported (mapper arm not present)
slot cardcatcher seen=1 emitted=0 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot castle seen=1 emitted=1 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot codemasters seen=22 emitted=21 -> codemasters
slot dahjee_typea seen=11 emitted=11 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot dahjee_typeb seen=5 emitted=5 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot eeprom seen=7 emitted=7 -> unsupported (93C46 EEPROM save chip not emulated)
slot hicom seen=9 emitted=9 -> unsupported (mapper arm not present)
slot janggun seen=1 emitted=1 -> unsupported (reverse bit 6 vs 7 unresolved (MAME and SMS Power disagree))
slot korean seen=6 emitted=6 -> unsupported (mapper arm not present)
slot korean_188in1 seen=1 emitted=1 -> unsupported (menu entries 6 and 13 fail in MAME and ares)
slot korean_nb seen=5 emitted=5 -> unsupported (mapper arm not present)
slot level3 seen=2 emitted=2 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot mgear seen=2 emitted=1 -> unsupported (Master Gear converter, not a game)
slot nemesis seen=1 emitted=1 -> unsupported (mapper arm not present)
slot othello seen=2 emitted=2 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot seojin seen=1 emitted=1 -> unsupported (game select at $FFF0 and data bit 7 inverted; no hardware dump evidence)
slot terebi seen=1 emitted=1 -> unsupported (SG-1000 board; the SG-1000 is not emulated)
slot xterminator seen=2 emitted=2 -> unsupported (X-Terminator cheat device)
slot zemina seen=8 emitted=8 -> unsupported (mapper arm not present)
cartdb: 1670 candidate facts; 21 compound/incomplete records excluded
```

The seen counts equal the planning-time counts (4pak 1, codemasters 22 for 5 plus 17, eeprom 7, hicom 9, janggun 1, korean 6, korean_188in1 1, korean_nb 5, nemesis 1, seojin 1, zemina 8, mgear 2, xterminator 2, the seven SG-1000 slots); no slot was unmapped and no identity conflicted.

## Row counts

Counted from `src/cartdb.inc` (also in `build/cartdb-input/counts.txt`, which is not committed); `cart.database` asserts them.

| Field | Value 0 | 1 | 2 | 3 | 4 |
|---|---|---|---|---|---|
| persistence | 0 | 1587 | 63 | 7 | 0 |
| ctrl1 | 1636 | 13 | 4 | 3 | 1 |
| ctrl2 | 1652 | 1 | 1 | 3 | 0 |

Board totals: 255 rows of board 0, 88 of board 1, 1293 of board 2, 21 of board 3. Rows with slot 0 number 1571.

## Table equivalence

- `git show HEAD:src/cartdb.inc > build/cartdb-input/old.inc`: exit 0. An awk script in `$TMPDIR` (not committed) wrote `build/cartdb-input/old.facts` with 1657 rows (sha1 bytes board machine region video persistence ram geometry public, sorted).
- A second awk script wrote the same columns from the new `src/cartdb.inc` (SHA-1 rebuilt from the 20 `0x..` bytes, public from flags bit 0), 1657 rows. For the 7 SHA-1s whose new row has slot eeprom (id 2) the persistence column is `e` in both files; the slot, ctrl1 and ctrl2 columns are not in the line.
- `comm -23 build/cartdb-input/old.m build/cartdb-input/new.facts > build/cartdb-input/lost.facts`: exit 0, and `test ! -s build/cartdb-input/lost.facts`: exit 0 (0 lost rows).
- `comm -13` rows only in the new table: 0. So no row has a board of 4 or above and no Sega game became a Korean board; the new table has the same 1657 unique facts as the old one (old 1657, new 1657).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The library may not link strcmp**
- **Found during:** Task 1 (`abi.undefined` failed with "unexpected undefined symbol: strcmp")
- **Issue:** the first draft of `cogmaster__cartdb_slot_named` used `strcmp`.
- **Fix:** a bounded compare loop over the fixed-width name.
- **Files modified:** src/cartdb.c
- **Commit:** f298f91

**2. [Rule 1 - Bug] hygiene.format flagged lines from this plan and from plan 06-02**
- **Found during:** Task 2 hygiene run
- **Issue:** `include/cogmaster.h` (a double space before a comment from plan 06-02), `src/cart.c`, `src/cartdb.c` and `tests/cart/test_cartdb.c` were not clang-formatted.
- **Fix:** ran clang-format 21 on the four files; whitespace only in the public header.
- **Files modified:** include/cogmaster.h, src/cart.c, src/cartdb.c, tests/cart/test_cartdb.c
- **Commit:** 1384815

**3. [Rule 1 - Bug] One doc line still named the table as the title source**
- **Found during:** Task 2
- **Issue:** `docs/conformance.md` said titles come from `src/cartdb.inc`.
- **Fix:** it now names `tests/compat/catalog.tsv`.
- **Files modified:** docs/conformance.md
- **Commit:** 1384815

### Plan-directed deviation

`tests/api/v3.c` takes no image and no ROM byte may be committed, so it cannot probe a hit. The "title empty on a hit" check lives in the fixture branch of `tests/cart/test_cartdb.c` (`cart.fixture_identity`, the owned battery image) instead, as the plan directs; the CONTEXT file list named v3.c.

### Process note

Task 1's commit leaves `compat.public_update` failing (it still read titles from the table) until Task 2's commit, as the plan states; every commit builds.

## Verification

- `cmake --preset ci && cmake --build --preset ci` exit 0
- `ctest --test-dir build/ci -R '^(cart\.|abi\.(globals|api_v3|api_v10)$|battery\.|fixtures\.)'` exit 0 (58 tests)
- `ctest --test-dir build/ci -R '^(cart\.(database|fixture_identity|fixture_hash)|compat\.)'` exit 0
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: 723 check lines (729 before; the compat title decoder was removed)
- `grep -c 'function title' scripts/compat-common.sh` prints 0; `grep -c 'catalog.tsv' scripts/compat-update.sh` prints 1; `grep -c '_Static_assert' src/cartdb.c` prints 7; the planning-ID grep over `src/cartdb.c` and `src/cart.c` prints 0 for each
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0; `git diff origin/main -- tests/built` exit 0 with 0 removed lines; `git status --porcelain -- build` prints nothing

## Known Stubs

None.

## Threat Flags

None. The change reads a read-only table and adds no network, auth or trust-boundary surface.

## Self-Check: PASSED

- FOUND: src/cartdb.inc, src/cartdb.c, src/cartdb.h, src/cart.c, tests/compat/catalog.tsv, tests/cart/test_cartdb.c
- FOUND commits: f298f91, 1384815
