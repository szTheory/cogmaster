---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "13"
subsystem: cartridge-database
tags:
  - cartdb
  - fixtures
  - device-defaults
  - region-trap
status: complete

requires:
  - 08-07
  - 08-08
  - 08-11
  - 08-12
provides:
  - "Two owned rows in the hash table, one per device fixture: sports_pad.sms (controllers 3 and 3, region 0) and paddle.sms (controllers 2 and 0, region 0), each a 32 KiB plain ROM board"
  - "tools/cartdb.c owned_fixture is a per-shape rule (battery, eeprom, sports pad, paddle) and the fixture file carries two controller columns"
  - "cogmaster-run with no option plays sports_pad.sms with the Sports Pad and paddle.sms under Japan with a Paddle; an export region degrades the Paddle default to a pad; a Paddle asked for on an export console exits 77"
  - "Build-time and ctest identity checks for both images, each with a wrong-hash control"
affects: [08-14, 08-17]

actuals:
  tokens: 12000
  tasks: 2
  commits: 2

plan_head_before: 66edb97ba0e59875757d894fb283da2e17a4209e
plan_head_after: 247812ee13f1d829df4a25d163653dd8e224bf25
commits: 2

tech-stack:
  added: []
  patterns:
    - "A fixture row pins its image by SHA-1 and the build refuses a later assembly edit until the row is re-pinned"
    - "A table-equivalence capture of the old table by SHA-1 before regeneration, compared by comm afterwards"

key-files:
  created: []
  modified:
    - tests/cart/fixture_facts.txt
    - tools/cartdb.c
    - src/cartdb.inc
    - tests/cart/test_cartdb.c
    - tests/cart/tests.cmake
    - tests/cart/generator.cmake
    - tests/fixtures/tests.cmake
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/maintaining.md

key-decisions:
  - "Both new rows say plain ROM board, machine SMS, video SMS, persistence 0, declared size 0, geometry 0 and region 0, which is what the images already probe as, so only the default controllers and the Paddle's Japan region change"
  - "The wrong-hash control for each new image is registered as its own test name beside the identity test, and both are in the inventories"
  - "The degrade control runs the still input, because a delta script on a plain pad is refused with exit 2 before any row is judged"

requirements-completed: []

coverage:
  - id: E1
    description: "The two device fixtures have rows in the table and the generated table differs from the previous one by exactly those two rows, shown per SHA-1 with comm, with the catalog byte-identical"
    requirement: PAD-06
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.(generator|database|fixture_identity)'"
      status: pass
    human_judgment: false
  - id: E2
    description: "The count pins in the cartdb test moved by exactly the two rows: paddle 4 to 5 and sports 3 to 4 on controller one, sports 3 to 4 and none 1653 to 1654 on controller two"
    requirement: PAD-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.database$'"
      status: pass
    human_judgment: false
  - id: E3
    description: "The fixture identity check runs at build time for both new images and as cart.fixture_identity.sports and cart.fixture_identity.paddle, each with a wrong-hash control that exits 1"
    requirement: PAD-06
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.fixture_(identity|wrong_hash)\\.(sports|paddle)$'"
      status: pass
    human_judgment: false
  - id: E4
    description: "The generator accepts the four owned shapes and refuses a Sports Pad row carrying the Paddle's controller codes with exit 1"
    requirement: PAD-06
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^cart\\.generator$'"
      status: pass
    human_judgment: false
  - id: E5
    description: "The Sports Pad image with no option is judged on the same rows as the explicit export case, and the runner execution line names port1=sports"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^(fixtures\\.sports_pad\\.default|runner\\.device\\.default)$'"
      status: pass
    human_judgment: false
  - id: E6
    description: "The Paddle image with no option runs under Japan and its detect and position rows pass; with an export region the default degrades to a pad and the detect rows fail"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^fixtures\\.paddle\\.(default|control\\.degrade)$'"
      status: pass
    human_judgment: false
  - id: E7
    description: "The probe reports the table defaults and region for both images, and a Paddle asked for on an export or PAL console exits 77 naming the region option"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^runner\\.device\\.(probe_default|paddle_export_refused)$'"
      status: pass
    human_judgment: false
  - id: E8
    description: "The whole ci and hygiene workflows pass with the inventories matching the registered tests and the check-code budget held"
    requirement: PAD-06
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0 (535 tests); cmake --workflow --preset hygiene exit 0 (48 tests); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 45min
completed: 2026-11-11
---

# Phase 8 Plan 13: Table rows for the Sports Pad and Paddle fixtures Summary

**The two device fixtures are in the hash table, so the runner picks a Sports Pad or a Japan-region Paddle from the table with no option, degrades the Paddle to a pad under an export region, and refuses an explicit Paddle there with exit 77.**

## What was built

- `tests/cart/fixture_facts.txt`: each line grows two columns, `ctrl1 ctrl2`, before the title. The battery and eeprom rows gain `0 0`. The new rows: `2046da4a... 32768 1 1 0 1 0 0 0 3 3` (Sports Pad) and `021f62aa... 32768 1 1 0 1 0 0 0 2 0` (Paddle), titles plain owned strings.
- `tools/cartdb.c`: the fixture reader parses twelve fields; `owned_fixture` is a per-shape rule. Battery and eeprom keep their hard-coded size, persistence, geometry and declared values and must name no controller; the Sports Pad shape is 32768 bytes, board 1, SMS, no region, no save, controllers 3 and 3; the Paddle shape is the same with controllers 2 and 0. Anything else is "invalid owned fixture facts".
- `src/cartdb.inc`: regenerated by the generator from the three pinned lists (SHA-256 of each equals `pins.txt`). The diff against `origin/main` is two added lines and no removed line: the header names the same MAME commit and the file carries no count line. `tests/compat/catalog.tsv` is byte-identical (the owned rows are not public).
- `tests/cart/test_cartdb.c`: four numbers moved by exactly the two rows (controller one: paddle 4 to 5, sports 3 to 4; controller two: sports 3 to 4, none 1653 to 1654). The file has no row-total pin, so no total moved.
- `tests/fixtures/tests.cmake`: the build loop's identity check has `ROW=3` for sports_pad and `ROW=4` for paddle beside battery and eeprom; three cases added (`fixtures.sports_pad.default`, `fixtures.paddle.default`, `fixtures.paddle.control.degrade`).
- `tests/cart/tests.cmake`: `cart.fixture_identity.sports` and `.paddle` (the hash-only mode of `fixture.cmake`) and one wrong-hash control each, `cart.fixture_wrong_hash.sports` and `.paddle`, which must exit 1.
- `tests/cart/generator.cmake`: gains an assertion that the two new rows carry their controller codes and a control that a Sports Pad row with the Paddle's codes is refused with exit 1. The existing row-count regexes did not need to change.
- `tests/runner/tests.cmake`: `runner.device.default` (execution line per option set), `runner.device.probe_default` (probe keys for both images) and `runner.device.paddle_export_refused` (exit 77 on NTSC-U and PAL, a named pad still plays).
- `tests/inventory/ci.txt` and `wasm32.txt`: the ten new names with the labels of their neighbours.
- `docs/maintaining.md`: the fixture row layout, the four accepted shapes, the re-pin rule, and the conversion counts (1,673 candidates, 21 excluded, 1,660 unique rows; the old text was stale by the eeprom fixture row).

## Table equivalence

- `git show HEAD:src/cartdb.inc > build/cartdb-input/old.inc` taken before regeneration, exit 0. An awk script in `$TMPDIR` (not committed) wrote the sorted fact lists: 1658 rows before, 1660 after.
- `build/ci/cogmaster-cartdb --pin 1d7a454ed79e97d35d2a9f127ac029ead94711d6 --catalog <tmp> <three lists> --fixture tests/cart/fixture_facts.txt`: exit 0; 1673 candidate facts, 21 excluded; the catalog `cmp` against `tests/compat/catalog.tsv`: exit 0.
- `comm -23 old.facts new.facts`: 0 lines. `comm -13 old.facts new.facts`: 2 lines, the two new SHA-1s. 2 added rows, 0 other differences.
- `git diff origin/main -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt`: no removed lines.

## Negative controls

- Red first: the four pins in `test_cartdb.c` were moved before regeneration; `ctest -R '^cart\.database$'` failed (exit non-zero) and passed after the table was regenerated.
- Region rule: in `src/cart.c` the `else if (paddle)` region branch was changed to `else if (paddle && 0)` (confirmed with `git diff --stat src/cart.c`: 1 insertion, 1 deletion, and the build linked). `ctest --test-dir build/ci -R '^fixtures\.paddle\.default$'`: exit 8 (failed); `runner.device.default` also failed. `git checkout -- src/cart.c`, rebuild, the same ctest: exit 0.
- A wrong declared hash for each image fails the identity check with exit 1 (`cart.fixture_wrong_hash.sports` and `.paddle`).

## Verification

- `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(cart\.|fixtures\.|runner\.probe\.)'`: exit 0, 85 of 85 passed.
- `cmake --workflow --preset ci`: exit 0, 535 of 535 passed. `cmake --workflow --preset hygiene`: exit 0, 48 of 48 passed.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0 (723 check lines, 8793 source lines). `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake`: exit 0.
- `gcc:13` in a container, `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `tools/cartdb.c`: exit 0. `clang-format --dry-run -Werror` over the two changed C files: exit 0.
- No hosted run was made for this plan; the branch was not pushed again (the plan names no dispatch). The last hosted evidence is run 42973268415 at `60c97a6`.

## Task Commits

1. `95ada41` feat(08-13): table rows for the Sports Pad and Paddle fixtures
2. `247812e` test(08-13): run the device fixtures with no option, degraded and refused

## Deviations from Plan

**1. [Rule 2 - Missing critical] Generator coverage for the new shapes**
- **Found during:** Task 1
- **Issue:** the plan changes `generator.cmake` only if its row-count regexes require it, but the per-shape rule is a behaviour change and nothing asserted that a wrong-shaped row is refused or that the new rows carry their controller codes.
- **Fix:** two small assertions in `tests/cart/generator.cmake`, one positive and one refused-with-exit-1.
- **Files modified:** tests/cart/generator.cmake
- **Commit:** 95ada41

**2. [Rule 3 - Blocking] The wrong-hash controls need their own inventory lines**
- **Found during:** Task 1
- **Issue:** the plan names two inventory lines (the identity tests) but also asks for one control per image; the inventories must match the registered tests exactly.
- **Fix:** the two controls are registered as `cart.fixture_wrong_hash.sports` and `.paddle` and listed with `cart,portable` in `ci.txt` and `wasm32.txt`, four lines in each file instead of two.
- **Files modified:** tests/inventory/ci.txt, tests/inventory/wasm32.txt
- **Commit:** 95ada41

**3. [Rule 1 - Bug] The degrade control cannot run the delta script**
- **Found during:** Task 2
- **Issue:** with the default degraded to a pad, the runner refuses a delta script before judging any row (exit 2 "port 1 is pad, which takes no motion"), so the control failed for the wrong reason.
- **Fix:** the control runs the all-zero script, as the existing export case does, so it fails on the detect rows with "outside its row".
- **Files modified:** tests/fixtures/tests.cmake
- **Commit:** 247812e

**4. [Rule 1 - Stale docs] The conversion counts in the maintainer doc**
- **Found during:** Task 1
- **Issue:** `docs/maintaining.md` still said 1,670 candidates and 1,657 rows, stale since the eeprom fixture row.
- **Fix:** updated to the measured 1,673 candidates and 1,660 rows, with the fixture row layout.
- **Files modified:** docs/maintaining.md
- **Commit:** 95ada41

**Total deviations:** 4 auto-fixed (1 missing coverage, 1 inventory consequence, 1 bug in a new control, 1 stale doc). **Impact:** none outside the plan's intent; `docs/maintaining.md` is the only file outside the plan's list.

The plan's acceptance line allowed one removed line in `src/cartdb.inc` (a count or header line); there is none, since the file carries no count line.

## Phase notes

- Any later edit of `sports_pad.asm` or `paddle.asm` changes the image's SHA-1: the build fails at the identity check until the row in `tests/cart/fixture_facts.txt` is re-pinned, `src/cartdb.inc` regenerated and the two digest lines in `tests/built/expected.txt` updated. That re-runs this plan's rows.
- The glasses image has no table row (the plan asks for two), so its identity is pinned only by `tests/built/expected.txt`.
- The picture of the owned rows: SMS board 1, persistence 0 (unknown), so a probe of either image still reports no save, as before.

## Known Stubs

None.

## Threat Flags

None. The plan adds two owned data rows, test cases and a generator rule; no network, auth or file-access surface is new.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-14. The branch holds two new local commits (`95ada41`, `247812e`) beyond the pushed `60c97a6`, plus the closeout commit; no push or hosted dispatch was made here.

## Self-Check: PASSED

- `tests/cart/fixture_facts.txt`, `tools/cartdb.c`, `src/cartdb.inc` and the other files in the key-files list exist; commits `95ada41` and `247812e` are ancestors of HEAD.
- `git rev-list --count 66edb97ba0e59875757d894fb283da2e17a4209e..HEAD` printed 2 before this SUMMARY was committed.
