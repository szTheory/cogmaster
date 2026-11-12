---
phase: 12-whole-list-in-the-ledger
plan: "01"
subsystem: catalog-generator
tags:
  - cartdb
  - catalog
  - graphic-board
  - ledger
  - requirements
status: complete

requires: []
provides:
  - Slot 21 (graphic, board 0) so the one Sega Graphic Board dump is refused by the table, behaviour revision 17
  - Catalog v2 (ten fields) with status, closed reason, region and ports for all 1,677 identities, derived compound ids and the measured pin counts
  - Generator title hardening and a catalog test that covers every status, compound id, count and refusal
  - Amended requirement, roadmap and legal wording that says what a pull request can prove
affects:
  - 12-02
  - 12-03
  - 12-05
  - 12-06
  - 12-07

actuals:
  tokens: 8900
  tasks: 3
  commits: 4

plan_head_before: c52a072cd10130a8529cffd0fb6b26b4244eb739
plan_head_after: 8acbc452330b6170e9a001f2ea584dbe6c9f0b40
commits: 4

tech-stack:
  added: []
  patterns:
    - "One status function in the generator; every identity gets its status and reason from the first matching rule"
    - "Compound identity is the SHA-1 of mame/<list>/<name>, computed with the project's own src/sha1.c and proven against a typed literal"
    - "Pin counts are measured by the generator and checked against the entries-minus-aliases arithmetic, failing closed"

key-files:
  created:
    - tests/cart/catalog.cmake
    - tests/compat/pin-counts.txt
  modified:
    - tools/cartdb.c
    - src/cartdb.inc
    - src/cart.c
    - include/cogmaster.h
    - tests/api/v14.c
    - tests/cart/test_cartdb.c
    - tests/cart/generator.cmake
    - tests/cart/tests.cmake
    - tests/runner/tests.cmake
    - tests/compat/catalog.tsv
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/boards.md
    - docs/embedding.md
    - docs/maintaining.md
    - THIRD-PARTY-NOTICES.md
    - PROVENANCE.md
    - ASSET_POLICY.md
    - .gitattributes
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md

key-decisions:
  - "A fact whose ctrl1 or ctrl2 code is 4 gets slot 21 and board 0 in the generator; a graphic default controller on a slot cartridge is a generator error"
  - "Compound machine comes from the softwarelist name (sms, gamegear, sg1000), not from a part interface, so a record with no part still has a machine"
  - "A same-SHA-1 group takes the worst status: any record with supported no makes the identity skipped with mame_no"
  - "The reconciliation arithmetic is checked inside the generator, not only in a test, so a count that does not add up fails the run"
  - "src/state.c is left alone: its 16 is a header byte offset, not the behaviour revision"

requirements-completed: []

coverage:
  - id: C1
    description: "The Sega Graphic Board is table data of board 0 (slot 21 graphic): the generator emits it from the pinned lists, the table refuses its SHA-1 as unsupported, and behaviour revision 17 refuses older states"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/cart/test_cartdb.c board-0 loop over every public sms and gg row, cart.database"
        status: pass
    human_judgment: false
  - id: C2
    description: "Every public board-0 row of the table refuses with the unsupported-board status, its own slot id and a reason that starts with the slot name; the graphic row is the one row with controller code 4"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/cart/test_cartdb.c, negative control: origin/main src/cartdb.inc makes cart.database exit 8, restored exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The runner refuses images the core cannot play with exit 77 and a board message, and an image of a supported board runs with exit 0 (synthetic images, no dump bytes)"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake case probe_unsupported (runner.probe.unsupported)"
        status: pass
    human_judgment: false
  - id: C4
    description: "The catalog has ten fields on all 1,677 lines: status, closed reason, region and the two ports, with the 19 compound entries as lines with derived ids and the SG-1000 lines excluded"
    requirement: LEDG-01
    verification:
      - kind: unit
        ref: "tests/cart/catalog.cmake (cart.generator.catalog) and the awk checks over tests/compat/catalog.tsv recorded in this summary"
        status: pass
    human_judgment: false
  - id: C5
    description: "The pin counts are measured by the generator, written to pin-counts.txt and reconciled (1,463 identities = 1,476 entries - 13 aliases) with the SG-1000 count printed"
    requirement: LEDG-01
    verification:
      - kind: unit
        ref: "tests/cart/catalog.cmake counts and reconciliation line; pinned re-run cmp of table, catalog and counts exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "A compound identity is the SHA-1 of mame/<list>/<name>, proven against a literal typed in cart.generator so every lane checks the C SHA-1 independently"
    requirement: LEDG-01
    verification:
      - kind: unit
        ref: "tests/cart/generator.cmake (cart.generator) and tests/cart/catalog.cmake string(SHA1) comparisons"
        status: pass
    human_judgment: false
  - id: C7
    description: "The generator refuses a control byte, invalid UTF-8, zero-width and bidirectional format characters, a byte order mark, a leading formula character, an unnamed or unknown list under a compound record, an unusable or repeated compound name and a pin without a CC0 header"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/cart/catalog.cmake (cart.generator.catalog), negative controls recorded in this summary"
        status: pass
    human_judgment: false
  - id: C8
    description: "REQUIREMENTS and ROADMAP carry the amended wording for LEDG-01 to LEDG-05 with the milestone line, the goal sentence, checkboxes and traceability untouched; the legal documents describe the ledger and the generator; the generated data files are marked linguist-generated"
    requirement: LEDG-05
    verification:
      - kind: other
        ref: "git diff acceptance greps against origin/main, ctest -R policy. and docs. exit 0"
        status: pass
    human_judgment: false
---

# Phase 12 Plan 01: Catalog v2, compound identities and the Graphic Board slot Summary

The generator now says everything the whole-list ledger needs about every catalog identity (status, closed reason, region, ports, derived compound ids, measured pin counts), and the Sega Graphic Board is refused by the table instead of running as a pad (behaviour revision 17).

## Performance

- **Duration:** about 13 min of measured wall time from the first command (the plan read and the planning was before that)
- **Tasks:** 3 of 3, plus one note fix commit
- **Files changed:** 23

## Accomplishments

1. **Graphic Board as table data (task 1).** `tools/cartdb.c` has slot 21 `graphic`, board 0, reason "Sega Graphic Board peripheral is not emulated". A fact whose controller code is 4 gets slot 21 and board 0; the run prints 21 slot lines and `cartdb: slot graphic seen=1 emitted=1`. The regenerated `src/cartdb.inc` differs from the old one by exactly the slot row and the one `segagfx` fact (`.board = 0u, .slot = 21u`); the catalog differs by that one line. `COGMASTER_BEHAVIOUR_REVISION` is 17 (API 14 and state 16 stay; two `#define` lines changed). `tests/api/v14.c` pins `>= 16u` for the constant and the reported value.
2. **Unit loop over every board-0 row.** `tests/cart/test_cartdb.c` walks every table row with `flags & 1`, machine sms or gg and board 0 (7 rows today: janggun, korean_188in1, seojin, graphic and three xterminator/mgear rows) and asserts the unsupported-board status, board 0, `unsupported_id` equal to the row's slot and a reason that starts with the slot name; it also asserts the row with controller code 4 sits in the graphic slot and that there is exactly one. The slot census loop covers 21 slots and `cogmaster__cartdb_slot(22)` is null.
3. **Runner (exit 77).** `runner.probe.unsupported` gained two runs: the 8 KiB Game Gear image with `--machine gg` exits 77 with an `unsupported: ... board` message, and the supported-board image `di-halt.sms` exits 0. The existing Janggun-shaped seed (`janggun-board.sms`, built by the test build, no dump bytes) already runs in that case and exits 77 with `board janggun: reverse bit ...`.
4. **Catalog v2 and compound entries (task 2).** Each line of `tests/compat/catalog.tsv` now has ten tab-separated fields (the first five unchanged) and the file has 1,677 lines: sms and gg status counts are unknown 1,434, unsupported 5, skipped 24; skipped reasons mame_no 6, multi_chip 12, bad_dump 2, short_dump 3, no_rom 1; unsupported reasons janggun 1, seojin 1, xterminator 2, graphic 1; the 21 compound lines (sms 12, gg 7, sg 2) carry board `compound`, slot `<list>/<name>` and the id sha1 of `mame/<list>/<name>`. SG-1000 lines are `excluded`.
5. **Pin counts.** `--counts FILE` writes `tests/compat/pin-counts.txt` (entries_sms 653, entries_gg 823, entries_sg 214, merged_sms_gg 13, merged_sg 0, compound_sms_gg 19, compound_sg 2, mame_no 6) from what the run measured, and the generator prints the reconciliation line `cartdb: ledger identities 1463 = 1476 entries - 13 same-image aliases (1444 images + 19 compound); SG-1000 excluded 214 entries (212 images + 2 compound)` and exits 1 if the arithmetic does not hold. All figures came out of the real run equal to the truth list on the first run.
6. **Title hardening.** `title()` refuses bytes below 0x20 or 0x7F, invalid UTF-8, U+200B to U+200F, U+202A to U+202E, U+2066 to U+2069, U+FEFF and a first byte in `= + - @`; none of the 1,656 current titles trips it (the real run exits 0).
7. **Wording and legal notes (task 3).** `REQUIREMENTS.md` LEDG-01 to LEDG-05 and ROADMAP Phase 12 criteria 1 to 5 carry the amended wording (scoped edits; line 8, the goal sentence, checkboxes and traceability untouched). `THIRD-PARTY-NOTICES.md`, `PROVENANCE.md` and `ASSET_POLICY.md` say what the ledger and the generator hold; `.gitattributes` marks the four generated data files `linguist-generated`. Titles stay out of `src/`, the 77 message (slot name only) and release zips: no edit was needed.

## Evidence (commands, exit statuses, no log committed)

- Pinned conversion: `sh scripts/fetch-pin.sh mame-sms $TMPDIR/p1/sms.xml`, the same for `mame-gamegear` and `mame-sg1000`: exit 0 each. Generator over the three files with `--counts`, the table, the catalog and the counts file written to a fresh directory: exit 0; `cmp` of table to `src/cartdb.inc`, catalog to `tests/compat/catalog.tsv`, counts to `tests/compat/pin-counts.txt`: exit 0 each. The same re-run before task 2 (table only, catalog five-field): exit 0, `cmp` exit 0.
- `cmake --workflow --preset ci` exit 0 (653 tests plus 5 release tests) after each of tasks 1 to 3; `cmake --workflow --preset hygiene` exit 0 (74 tests) after tasks 2 and 3.
- `ctest --test-dir build/ci -R '^(cart|api|abi|core|state|runner\.probe)'` exit 0 (73 tests) after task 1.
- Container check: `docker run gcc:13 ... gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I include -I build/ci/generated -I src` over `tools/cartdb.c`, `tests/cart/test_cartdb.c`, `tests/api/v14.c`, `src/cart.c`: exit 0. `clang-format` 21.1.8 `--dry-run -Werror` over the same four files: exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` prints `860 check lines / 9403 src lines` (counted check code unchanged; src is one comment line longer, which only raises the limit).
- Catalog awk acceptance checks (status counts, reason counts, 10 fields on every line, 1,677 lines, 8 count lines): exit 0 each. Inventories: `git diff --numstat origin/main` shows 1 added and 0 deleted line in `ci.txt` and in `wasm32.txt`.
- Hosted dispatch: none in this plan (the first is after plan 5).

## Negative controls (each a python edit, rebuild, red, restore, green)

| Control | Mutation | Result |
|---|---|---|
| Old table | `git show origin/main:src/cartdb.inc > src/cartdb.inc`, rebuild | `cart.database` exit 8 (graphic slot, the one-row and census assertions), restored exit 0 |
| Status function | `if (f->no)` became `if (0 && f->no)` in `tools/cartdb.c` | `cart.generator*` exit 8; restored exit 0 |
| Compound id | `"mame/%s/%s", list, name` became `"mame/%s", name` | `cart.generator*` exit 8 (both the typed literal and the string(SHA1) comparisons); restored exit 0 |
| Bidi title | the U+202A to U+202E range ended at U+202D | `cart.generator*` exit 8; restored exit 0 |

## Deviations from Plan

### Auto-fixed Issues

None needed code-wise; the following differ from the plan's text and are recorded for the verifier.

**1. [Plan text] The hard-coded slot count is at one place in `test_cartdb.c`, not two**
- **Found during:** Task 1
- **Issue:** the plan says "the two hard-coded 20" become 21; line 22 holds `digest[20]` (a SHA-1 length), not a slot count, and only the slot loop (`id <= 20u`) changed.
- **Fix:** changed the one slot count; added `cogmaster__cartdb_slot(22)` is null.

**2. [Plan text] No runner image can name the Graphic Board's slot**
- **Found during:** Task 1
- **Issue:** a database hit needs the exact SHA-1 of the one real dump, which cannot be built without its bytes (rule 3). The runner's exit 77 for that slot is therefore shown as a chain, not by an image: the unit loop proves `cogmaster__cart_fact_to_info` returns the unsupported-board status for the graphic row; `cogmaster__cart_probe` returns that status for a database hit (`src/cart.c`), `cogmaster_load` returns the probe status (`src/instance.c`), and the runner's `load_status` maps that status to exit 77 (the Janggun-shaped and the 8 KiB images exercise exactly that mapping and message path).
- **Janggun builder:** none built in this task: `tests/roms/src` already has `janggun-board.sms`, and `runner.probe.unsupported` already runs it (exit 77 with the reason text). The plan's two new runs were added as the 8 KiB `--machine gg` image (exit 77, board message) and a supported-board image (exit 0).

**3. [Plan text] `comm -13` of the old and new first five fields prints 22 lines, not 21**
- **Found during:** Task 2
- **Issue:** 21 lines are the added compound and SG compound entries; the 22nd is the intentional graphic line (board and slot changed in task 1).

**4. [Plan acceptance] The `git diff ... | grep -v '^[-+][-+]'` filter in task 3 drops the markdown bullets**
- **Found during:** Task 3
- **Issue:** a changed requirement line starts with `- [ ]`, so after the diff marker it begins `+-` or `--` and the plan's filter removes every one; its count printed 0 instead of at least 10.
- **Fix:** the same count with the filter `grep -v '^\(---\|+++\)'` prints 10, and the checkbox and traceability count prints 0.

**5. [Plan text] The wasm32 preset registers non-portable tests**
- **Found during:** Task 2
- **Issue:** none; the plan's "1 added line in `wasm32.txt`" was right (that inventory lists every wasm32 test; `tests.wasm32` floor 612 stays below its count). Recorded because the CONTEXT said wasm32.txt unchanged.

**6. [Rule 3 - Blocking] A timeout note used a placeholder figure**
- **Found during:** Task 2
- **Fix:** commit `8acbc45` records the measured local run (0.13 s slowest of five) in the provisional note of `cart.generator.catalog`.

**Total deviations:** 6 recorded (none changed a behaviour). **Impact:** none on scope; every figure of the truth list matched the generator's real output on the first run.

### Generator facts the truth list did not state

- The generator prints one `cartdb: compound record kept as a catalog line: <title>` per compound record where it used to print an exclusion line; the `1673 candidate facts; 21 compound/incomplete records excluded` line is unchanged (candidates are the table facts before merging).
- A compound record's machine comes from its list name, so mgear (a Game Gear list entry with no ROM) is `gg`; sms 12, gg 7, sg 2 match the truth list.
- `docs/maintaining.md` needed 32 bytes cut to stay at 19,870: the stale "10.6 minutes" and "(5.5 minutes)" figures, the two measured hash-file timings and the derived "1,660 unique rows" figure were removed; nothing was rewritten.

## Counted check code and docs bytes

- Counted check code: **860** (cap for this plan 860; limit 940 with 9,403 src lines).
- `docs/embedding.md` 36,521 bytes (cap 36,700), `docs/boards.md` 9,221 (cap 9,700), `docs/maintaining.md` 19,862 (cap 19,870).
- `tests.ci` inventory 660 lines (one added), `tests.wasm32` one added; no `quality.json` change in this plan.

## Known Stubs

None.

## Threat Flags

None: no new network endpoint, auth path or file-access surface; the generator reads files the caller names and writes the named outputs.

## Self-Check: PASSED

- Files exist: `tests/cart/catalog.cmake`, `tests/compat/pin-counts.txt`, `tests/compat/catalog.tsv`, `src/cartdb.inc` (all found).
- Commits are ancestors of HEAD: `9002c0f`, `963d6f2`, `6d60f74`, `8acbc45` (all found).
