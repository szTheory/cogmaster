---
phase: 06-korean-cartridges-play
plan: "08"
subsystem: cart
tags:
  - cart
  - probe
  - scan
  - board-detection
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-03 slot rows and the slot-by-name lookup; plan 06-07 the Zemina, Nemesis and 4 PAK boards
provides:
  - src/cart_scan.c, an instruction-aware sweep that counts LD (nn),A store sites by register family over at most the first 32 KiB
  - guess_board in src/cart.c in place of other_board, with board_source, board_candidate and the three scan counts filled
  - cart.probe.scan, the decoy and family images
  - a geometry property over every table row and the new detection prose in docs/boards.md
affects: [06-09, 06-11]

actuals:
  tokens: 9100
  tasks: 3
  commits: 4

plan_head_before: 21e4b57dfd18ef06c4f327cc1801b6060c888ada
plan_head_after: 8b2245d373246cf9c60b3de4a75dd2276c12e37a
commits: 4

tech-stack:
  added: []
  patterns:
    - "The scan is a linear sweep with a 256-entry length table and prefix rules; a store counts only at an instruction boundary"
    - "A family wins only with a threshold, a margin over the Sega count, a margin over the runner-up and a fitting size; anything between refuses naming the candidate"

key-files:
  created:
    - src/cart_scan.c
    - .planning/seeds/SEED-009-scan-thresholds.md
  modified:
    - src/cart.c
    - src/internal.h
    - CMakeLists.txt
    - tests/cart/test_cart.c
    - tests/cart/tests.cmake
    - tests/cart/test_cartdb.c
    - tests/core/test_core.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/boards.md

key-decisions:
  - "Thresholds stay at T = 3 for every family and the margins at 2s + 2 and 2y + 2; no private library was configured, so SEED-009 holds the measurement"
  - "The Sega margin rule (Sega wins over a leader only at s >= 2x + 2) is the planner's reading of never silently plays; it is Claude's discretion and is recorded here"
  - "An image with no store sites at all takes the Sega board with source default, so a plain 64 KiB or larger Sega image still loads"
  - "A scan family that wins on count but fails its size rule, or sits under a Game Gear header, does not win and falls to the doubt zone with the candidate named"

requirements-completed: []

coverage:
  - id: C1
    description: "The board is chosen database first, then the Codemasters header, then the 8 KiB Game Gear refusal, then a valid ROM header, then the 48 KiB size rule, then the scan, and board_source says which step decided"
    requirement: MAP-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.probe\\.' exit 0 (probe.branches and probe.scan check status, board, source and candidate for each step)"
      status: pass
    human_judgment: false
  - id: C2
    description: "A sealed valid ROM header with many Korean-looking stores stays on the Sega board with source header, and a header whose checksum range reaches past the body is no valid header"
    requirement: MAP-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.probe\\.scan$' exit 0 (decoy and range past body cases)"
      status: pass
    human_judgment: false
  - id: C3
    description: "The scan counts stores only at decoded instruction boundaries over at most the first 32 KiB with no allocation; 3E 32 00 A0 repeated counts nothing, and 1 MiB of DD or ED prefix bytes probes and returns"
    requirement: MAP-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.probe\\.scan$' exit 0 (operand decoy, prefix-only images, late stores); the library has no allocation call in src/cart_scan.c"
      status: pass
    human_judgment: false
  - id: C4
    description: "A family wins only with at least 3 sites, at least 2s + 2 over the Sega count, at least 2y + 2 over the runner-up and a fitting geometry; below threshold, below margin, a tie, or a wrong size refuses naming the candidate"
    requirement: MAP-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.probe\\.scan$' exit 0 (below threshold, tie, tie with Sega, bad checksum, Korean 136 KiB and Zemina 64 KiB geometry cases, with scan_sega, scan_candidate and scan_runner_up checked)"
      status: pass
    human_judgment: false
  - id: C5
    description: "A headerless Korean, Zemina, Nemesis or 4 PAK image selects its board with source heuristic and plays with no setting; a Janggun-looking image refuses with Janggun's slot id and reason; a Game Gear region header never selects a Korean board"
    requirement: MAP-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.probe\\.scan$' exit 0 (korean, zemina, nemesis, nemesis byte 0, 4 PAK, janggun, game gear, copier korean cases)"
      status: pass
    human_judgment: false
  - id: C6
    description: "Every table row's size fits its board's geometry and no shipped fixture changes board"
    requirement: MAP-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^cart\\.database$' exit 0 (row_fits over every row, all rows conform on the pinned data); ctest --test-dir build/ci -R '^(cart\\.|fixtures\\.|runner\\.|libretro\\.host\\.|compat\\.fixtures$|battery\\.)' exit 0, 132 of 132 passed"
      status: pass
    human_judgment: false
  - id: C7
    description: "The whole plan passes the full local workflow and hygiene, stays inside the check-line budget, and leaves tests/audio, tests/testcard_pins.cmake and tests/built untouched"
    requirement: MAP-06
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0 (427 tests passed); cmake --workflow --preset hygiene exit 0 (48 of 48); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines of 7953 src lines); git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0; grep -c '^-[^-]' over the tests/built diff prints 0; grep -c other_board src/cart.c prints 0"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 08: Board scan and guess_board Summary

`other_board` is replaced by `guess_board` and an instruction-aware scan in `src/cart_scan.c`: the database wins, a valid ROM header outranks the scan, a family needs three sites and two margins and a fitting size, and doubt refuses with the candidate named; `board_source`, `board_candidate` and the three scan counts are filled.

## What was built

- `src/cart_scan.c`: `cogmaster__cart_scan` sweeps linearly over `min(body, 32 KiB)` with a 256-entry length table for unprefixed opcodes and prefix rules (CB two bytes; ED four bytes for `43/4B/53/5B/63/6B/73/7B`, else two; DD and FD add a displacement byte to opcodes that name `(HL)`, take `DD CB d op` as four bytes, and count a prefix followed by a prefix as one byte). Each step advances at least one byte, so at most 32,768 steps; locals only, no allocation. A `$32` at a boundary is classified by target: Sega `$FFFC`-`$FFFF`, Korean `$A000`, Zemina `$0001`-`$0003`, 4 PAK `$3FFE`/`$7FFF`/`$BFFF`, Janggun `$6000`. `$0000`, `$4000` and `$8000` count for no family, since the Codemasters board uses them.
- `src/cart.c`: `guess_board` runs after the hash lookup: Codemasters header (header); 8 KiB image loaded as Game Gear refused (default); a valid header (magic, region 3 to 7, `checksum_ok`, checksum range within the body) gives no mapper up to 48 KiB else Sega (header); at most 48 KiB is no mapper (default); otherwise `scan_board`. The scan picks the leading family F (Korean, Zemina, 4 PAK, Janggun break ties), with x, the runner-up y and the Sega count s. F wins on `x >= 3`, `x >= 2s + 2`, `x >= 2y + 2`, a fitting size and no Game Gear region in the header; Zemina becomes Nemesis for a 128 KiB image with `$F3` at `body - 8 KiB` and not at byte 0; Janggun refuses with its slot id and reason from `cogmaster__cartdb_slot_named("janggun")`. Sega wins at `s >= 2x + 2`. Anything else refuses with `ERR_UNSUPPORTED_BOARD`, board 0, source heuristic and `board_candidate` set. With no store sites the Sega board is chosen with source default. The unsupported-reason text builder is now shared by the table path and the scan (`name_unsupported`).
- `src/internal.h`, `CMakeLists.txt`: `struct cogmaster__scan` (counts per family) and the declaration; `src/cart_scan.c` added to the library.
- `tests/cart/test_cart.c`, `tests/cart/tests.cmake`, `tests/inventory/{ci,wasm32}.txt`: `fill` writes only single-byte opcodes (`$40` to `$64`, never `$32`) so every `stores()` site sits on a boundary; `cart.probe.scan` covers the decoy, bad checksum, threshold, tie, operand decoy, each family, Nemesis signature and byte 0, Janggun, Game Gear header, both geometry refusals, the Sega margin, a 512-byte-prefixed Korean image, a 1 MiB + 512 body and 1 MiB of `$DD` and `$ED`. Two `test_branches` cases follow the new rule (four `$A000` against two Sega is a doubt; `$FFFF` needs ten sites against four to win).
- `tests/cart/test_cartdb.c`: `row_fits` checks every table row against its board's geometry and prints the slot and size of a misfit; all rows conform on the pinned data.
- `docs/boards.md`: the new order with `board_source` for each step, the scan, the three new fields in the table, and Seo Jin and 188-in-1 added to the hash-only sentence.

## Threshold decision

T = 3 for every family, margins `2s + 2` and `2y + 2`, left conservative. No private library was configured (`COGMASTER_ROM_DIR` unset), so nothing was measured; `.planning/seeds/SEED-009-scan-thresholds.md` records the measurement on real dumps. The Sega-wins margin (`s >= 2x + 2`) is the planner's reading of "it never silently plays" and is Claude's discretion.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] core.load.board relied on the old rule**
- **Found during:** the full ci workflow after Task 3
- **Issue:** `tests/core/test_core.c` `test_load_board` used four `$A000` stores in a zero-filled 64 KiB image as its "refused" image; under the new rule that is a Korean image (x = 4, s = 0) and loads.
- **Fix:** two stores, below the threshold of three, so the image is a doubt and stays refused; the comment says why.
- **Files modified:** tests/core/test_core.c
- **Commit:** 8b2245d

**2. [Rule 3 - Blocking] local clang-format**
- `clang-format` was not on PATH; formatting used the Homebrew `llvm@21` binary the hygiene preset also finds. No behaviour change.

The plan's file list did not name `tests/core/test_core.c`; it is the only file added to the plan's list.

## Known Stubs

None.

## Threat Flags

None. The scan reads at most the first 32 KiB of an image already bounded by the size rules, with no allocation and no new input path.

## Self-Check: PASSED

- `src/cart_scan.c`, `.planning/seeds/SEED-009-scan-thresholds.md` and this summary exist.
- Commits f501fae, 9f1d43d, dbac7e8 and 8b2245d are ancestors of HEAD.
- `cmake --workflow --preset ci` exit 0 and `cmake --workflow --preset hygiene` exit 0 on the head after the last code commit.
