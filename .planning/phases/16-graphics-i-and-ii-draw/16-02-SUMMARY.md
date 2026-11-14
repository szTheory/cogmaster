---
phase: 16-graphics-i-and-ii-draw
plan: 02
subsystem: fixtures
tags:
  - oracles
  - fixtures
  - graphics-i
  - graphics-ii
  - legacy-sprites
  - mailbox

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: plan 01 riders (assembler fetch retry, dirty-tree refusal), so the base hashes file is taken from a clean tree that builds the same runner and images as origin/main
provides:
  - build/p16-base-hashes.txt, 173 lines, ignored build output, taken from the unmodified code tree for the subset compare
  - six owned images (legacy_modes_g1, g2, g2_masked, g2_late, g2_switch and legacy_sprites) built from two assembly sources by one CMake function and pinned in tests/built/expected.txt
  - tests/fixtures/legacy.c, the closed-form pixel oracle with legacy_status() and a literal self-check that also judges the csv rows
  - tests/vdp/test_legacy.c, the in-process oracles for Graphics I, Graphics II with the eleven register pairs, the palette and the sprite walk
  - 22 derived legacy_sprites rows in tests/diag/expected.csv, the tail-read row included
  - fixtures.legacy.oracle and vdp.legacy.oracle in the ci and wasm32 inventories
affects:
  - 16-03
  - 16-04
  - 16-05
  - 16-07
  - 16-08
  - 16-09

actuals:
  tokens: 22500
  tasks: 3
  commits: 3
plan_head_before: 0452732e6bf744f23bfea1d43006c574254ae482
plan_head_after: 4c4a6b0c611c98fd1fcd8c0d6bef47bfa3ae8204

tech-stack:
  added: []
  patterns:
    - "an oracle written in the tile-index domain from a generated scene, shared with the image only as formulas in a comment block, never as address arithmetic"
    - "one CMake function builds every variant of a source, each with its own link file and a -D define"
    - "a mailbox row that is computed by a named oracle rule and re-derived by the literal self-check, so no expectation is typed by hand"
    - "a timing-dependent read calibrated once on a deterministic core and judged by a range derived from the documented counter table"

key-files:
  created:
    - tests/roms/src/legacy_modes.asm
    - tests/roms/src/legacy_modes.link
    - tests/roms/src/legacy_sprites.asm
    - tests/roms/src/legacy_sprites.link
    - tests/fixtures/legacy.c
    - tests/vdp/test_legacy.c
  modified:
    - tests/fixtures/tests.cmake
    - tests/vdp/tests.cmake
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/roms/manifest.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "Every legacy_sprites scenario programs its own R7, so a picture names its scenario in its backdrop and a compare one frame off fails instead of passing by accident; the same R7 shows in the fault backdrop of the current core, which is how the frame schedule was measured"
  - "The tail H counter range is 0000-0013, not the whole tick 600 to 683 span: those ticks read F3 to FF and then 00 to 07, which wraps, so the range starts at the first status tick whose H read (45 ticks later) is 00, tick 607; the calibrated read lands at H 09, status tick about 644"
  - "The legacy_modes images carry no mailbox and no sprites (Y D0 at entry 0), so a later sprite engine cannot move their hashes; header.inc, mailbox.inc and boot_to_play.asm are byte-identical to HEAD"
  - "g2_late is compared at frame 40 (variant g2_late_before, mode 4, passes on the current core on all three profiles) and at 640 (variant g2_late); the picture switches at host frame 616 because the image spends 16 frames loading before its 600 counted frames"
  - "The switch line of g2_switch is 48 (line interrupt on 47, R0 write about dot 116 of the next line, measured at frame 20 of the current core); compare skips it and the renderer plans pin it by hash"
  - "The status expectations in the csv are written from legacy_status() output by the rows mode, and fixtures.legacy.oracle fails when a hex, a citation, a profile, a source or the row set differs; three controls (changed hex, missing row, changed range) were run and each exits 1"

requirements-completed: []

coverage:
  - id: D1
    description: "The base hashes file is written from the unmodified code tree before any edit to src, include, runner, tests/roms or CMakeLists.txt"
    verification:
      - kind: other
        ref: "git diff --quiet origin/main HEAD -- src include runner tests/roms CMakeLists.txt exit 0, then ctest --preset ci -R runner.hashes_file exit 0 and cp of build/ci/hashes.txt to build/p16-base-hashes.txt, 173 lines"
        status: pass
    human_judgment: false
  - id: D2
    description: "Six owned images build from two sources through one CMake function and each is pinned by name in tests/built/expected.txt; header.inc, mailbox.inc, boot_to_play.asm and src are unchanged"
    requirement: MODE-01
    verification:
      - kind: integration
        ref: "cmake --build --preset ci --target cogmaster-fixtures exit 0; grep -c legacy_ tests/built/expected.txt prints 6; git diff --quiet HEAD -- tests/roms/src/header.inc tests/roms/src/mailbox.inc tests/roms/src/boot_to_play.asm src exit 0"
        status: pass
      - kind: integration
        ref: "ctest --preset ci -R runner.hashes_file exit 0 and LC_ALL=C comm -23 of the sorted base and head hashes files prints nothing, exit 0; the head file adds only the six fixture legacy_ lines"
        status: pass
    human_judgment: false
  - id: D3
    description: "A closed-form pixel oracle exists before any renderer: pixel(x, y) per variant and profile from the generated scene, the digest of the expected RGB444 field, and a literal self-check against hand-computed pixels that passes now"
    requirement: MODE-01
    verification:
      - kind: unit
        ref: "ctest --preset ci -R fixtures.legacy.oracle exit 0 (64 pixel literals, 12 status literals, the tail range, three digest controls)"
        status: pass
      - kind: other
        ref: "scratch harness, not committed: each image run on sms1, sms2 and gg, video memory read back and drawn by address arithmetic, 0 mismatches against pixel() for g1, g2, g2_masked, g2_late at frame 640 and the g2_switch lines from the switch line on; the sprite generator and cleared tables equal the oracle and the 19 attribute tables and registers equal the scenario list frame by frame"
        status: pass
    human_judgment: false
  - id: D4
    description: "MODE-02 maps to the eleven (R3, R4) pairs run in process under one oracle (nine bite on the Master System 1, two do not) plus two ROM images through cogmaster-run, legacy_modes_g2 (all ones) and legacy_modes_g2_masked; the renderer case and the image comparisons are registered in plans 04 and 05"
    requirement: MODE-02
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.oracle exit 0: each pair checked on three profiles, SMS1 differs from SMS2 for nine pairs and for none of FF 03 and 7F 07, SMS2 and GG equal for every pair, hand-computed dots for every pair"
        status: pass
      - kind: other
        ref: "fixtures.legacy.oracle asserts the digest of g2_masked differs between sms1 and sms2 and that g2 does not"
        status: pass
    human_judgment: false
  - id: D5
    description: "In-process oracle functions exist for Graphics I, Graphics II with the eleven pairs on the three profiles, the fixed palette and a per-line legacy sprite walk, without any call into the core"
    requirement: MODE-03
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.oracle exit 0; grep -cE cogmaster__(vdp|legacy)_ tests/vdp/test_legacy.c prints 0; six mutations of the oracle (a wrong literal, a pattern mask bit, the Y threshold, the collision bound, masks applied to the Game Gear, the Game Gear palette offset) each exit 1"
        status: pass
    human_judgment: false
  - id: D6
    description: "The sprite scenario list (20 scenarios, 22 records) covers sizes, magnification, the early clock, the end of list, Y FF, E8 and E0, five on a line, two overflow lines, colour 0 collision, touching, off-edge overlap, a sprite after the end of list and a dropped fifth, and every record has a derived row computed by legacy_status"
    requirement: MODE-03
    verification:
      - kind: integration
        ref: "build/ci/cogmaster-run build/ci/fixtures/legacy_sprites.sms --frames 200 --mailbox exit 0, status=00, magic ok, crc ok, 22 records"
        status: pass
      - kind: unit
        ref: "ctest --preset ci -R fixtures.legacy.oracle exit 0 reads tests/diag/expected.csv: 22 legacy_sprites rows, 21 citing legacy_status, each hex equal to the computed value; a changed hex, a missing row and a changed range each exit 1"
        status: pass
    human_judgment: false
  - id: D7
    description: "The tail-read row is calibrated: the status read in the tail of line 80 and the H counter read next to it land in ticks 607 to 683, so the image proves the fifth sprite carry at image level"
    requirement: MODE-03
    verification:
      - kind: other
        ref: "cogmaster-run legacy_sprites.sms --frames 200 --mailbox with --machine sms, sms1 and gg --video-mode sms, all exit 0 and record 15 equals 0009, inside 0000-0013; record 14 equals 001F"
        status: pass
    human_judgment: false
  - id: D8
    description: "The late-entry and mid-line-switch images exist and their mode 4 halves are closed form on the current core"
    requirement: MODE-06
    verification:
      - kind: other
        ref: "cogmaster-test-legacy-fixture legacy_modes_g2_late.sms g2_late_before sms1, sms2 and gg compare 40 each exit 0 on the current core; the Graphics II halves are compared when the renderer lands"
        status: pass
    human_judgment: false

duration: 42 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 02: Oracles and fixtures Summary

**Six pinned Graphics I and II images, a closed-form pixel oracle and an in-process oracle with literal self-checks, and 22 sprite-status rows computed by one named rule, all committed before any renderer exists**

## Performance

- **Duration:** about 42 min (the start is taken from the base hashes file, so the first reading is included)
- **Tasks:** 3 (the first a tracer)
- **Files:** 13 changed, 6 new, 2,210 lines added
- **Commits:** 3 task commits; the SUMMARY and state commit follow

## Accomplishments

- **Base hashes.** `ctest --preset ci -R '^runner\.hashes_file$'` exited 0 on the unmodified code tree (`git diff --quiet origin/main HEAD -- src include runner tests/roms CMakeLists.txt` exited 0, so no worktree was needed) and `build/ci/hashes.txt` was copied to `build/p16-base-hashes.txt`, 173 lines. After the six images landed the head file differs only by the six `fixture legacy_*` lines; `comm -23` of the sorted base against the sorted head prints nothing, exit 0.
- **Six images from two sources.** `legacy_modes.asm` selects Graphics I (`V_G1`), Graphics II all ones (`V_G2`, R3 `$FF` R4 `$03`), Graphics II with clear low bits (`V_G2_MASKED`, R3 `$9F` R4 `$00`), a late entry (`V_G2_LATE`, mode 4 for 600 counted frames, then R2 and R0 in vertical blank) and a mid-line switch (`V_G2_SWITCH`, mode 4, a line interrupt on line 47 writes R0 in line 48, mode 4 restored in vertical blank). `legacy_sprites.asm` runs 20 scenarios and reports through the mailbox. One function in `tests/fixtures/tests.cmake` builds all six. Every table is generated by loops from formulas written once in the header comment of `legacy_modes.asm`; no `.DB` run reaches the hygiene tripwire.
- **Closed-form oracle.** `tests/fixtures/legacy.c` computes `pixel(variant, profile, scenario, x, y)` from the tile-index domain, prints the SHA-256 of the expected field (little-endian RGB444 words, the bytes `cogmaster-run --hash` hashes), and runs a literal self-check with hand-derived pixels on three profiles, one pixel on each side of lines 63/64 and 127/128, status bytes worked out by hand, the H counter anchors of the video notes (tick 600 is `F3`, 607 `F4`, 608 `F5`, 0 `08`) and three digest controls (Game Gear differs from Master System, Master System 1 differs from 2 only when both masks bite, and does not for the all-ones registers).
- **In-process oracle.** `tests/vdp/test_legacy.c` has `oracle_g1`, `oracle_g2` (any R3 and R4, masked on the Master System 1 only), `oracle_palette` and `oracle_sprites` (a per-line walk of 32 entries, `$D0` ends the list, four per line, lower index wins, colour 0 transparent, coincidence on dots 0 to 255 only). The case `oracle` checks 9 palette, 10 Graphics I, 27 Graphics II and 29 sprite literals, the 0 to 255 collision bound at the right edge, and all eleven pairs over the whole field.
- **The sprite status is one rule.** `legacy_status(scenario, read_line)` derives bits 7, 6, 5 and the fifth index from the per-line list walk. The `rows` mode prints `<id>:<name> <hex>` and the csv rows were written from that output; `fixtures.legacy.oracle` re-reads the csv and fails if any hex, profile, source, citation or the row set differs.
- **The tail read is calibrated.** Four sprites start on line 81 and a fifth ends on it; a line interrupt on line 79 waits a calibrated delay, reads the status once in the tail of line 80 (after the tick 600 scan, before dot 0) and the H counter 45 ticks later. The recorded H counter is `09` on sms, sms1 and gg-in-SMS-mode, inside the row range `0000-0013`. The frame-end read must report bit 6 with entry 4 (`00C4`); the tail read itself is `001F`.

## Task Commits

1. **Task 1: tracer, a Graphics I image with its pixel oracle** - `fcca930` (test). This is the commit that first adds `tests/fixtures/legacy.c` (`git log --format=%H --diff-filter=A -- tests/fixtures/legacy.c` prints exactly `fcca93090ce5c9c7e3436035b2620467cc7c520f`); plan 04 proves it is an ancestor of the renderer commit.
2. **Task 2: the other four mode images and the legacy sprite scenarios** - `8761069` (test)
3. **Task 3: in-process oracles for the Graphics I and II draw** - `4c4a6b0` (test)

**Plan metadata:** the SUMMARY commit and the STATE and ROADMAP commit that follow this file.

The tracer gate was met by re-running the Task 1 verify chain end to end (base file present, configure, build, `fixtures.legacy.oracle`, `git diff --quiet HEAD` over header.inc, mailbox.inc, boot_to_play.asm and src, all exit 0) before expanding.

## Files Created/Modified

- `tests/roms/src/legacy_modes.asm`, `legacy_modes.link` - the five Graphics I and II images (new)
- `tests/roms/src/legacy_sprites.asm`, `legacy_sprites.link` - the scenario image and its mailbox reports (new)
- `tests/fixtures/legacy.c` - closed-form oracle, digest, compare, rows and literal self-check (new)
- `tests/vdp/test_legacy.c` - the in-process oracles and the `oracle` case (new)
- `tests/fixtures/tests.cmake` - the image function, `cogmaster-test-legacy-fixture`, `fixtures.legacy.oracle`
- `tests/vdp/tests.cmake` - `cogmaster-test-legacy`, `vdp.legacy.oracle`
- `tests/built/expected.txt`, `tests/roms/manifest.txt`, `tests/diag/expected.csv` - six pins, one manifest line, 22 derived rows
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the two new test names, sorted, identical in both

## Decisions Made

- The legacy_sprites picture is a backdrop that changes with each scenario (R7), not a constant. Two reasons: a compare one frame off meets another scenario's backdrop and fails, and the current core, which still faults on legacy modes, paints the backdrop from colour RAM entry 16 plus R7, so the schedule could be measured before any renderer existed (scenario 1 is the completed picture after frame 9, then every 2 frames).
- The H counter row cannot span "ticks 600 to 683": the counter reads `F3` to `FF` and then wraps to `00` to `07`. The row is `0000-0013`, derived in `legacy.c` from the video-notes table (`h_counter`), with the status read 45 ticks before the H read (IN 33 ticks with its access at tick 30, LD 12, the next IN accessed at tick 30). A value inside proves the status read was in ticks 607 to 683 of line 80.
- The Graphics II scene indexes the pattern and colour tables by `idx = third * 256 + name` with the third folded into the formulas (`101 th`, `3 th`, `5 th`), so each third has distinct data and a masked read that drops the third is visible.
- `g2_masked` uses R3 `$9F` and R4 `$00`: both masks bite on the Master System 1 and the generated data differs between thirds, so the model digests differ.
- No new PROVENANCE seed row: no external expression was copied.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Plan variant] The tail H counter range wraps, so the row is 0000-0013**
- **Found during:** Task 2 (deriving `TAIL_HCOUNT_LO` and `TAIL_HCOUNT_HI`)
- **Issue:** the plan names the inclusive range of H counter values for ticks 600 to 683. Those ticks read `F3` to `FF` then `00` to `07`, which the judge's `lo-hi` form cannot express, and the H read follows the status read by 45 ticks, not zero.
- **Fix:** the constants hold the contiguous range reached when the H read is at or after the wrap (`00` to `13`, status ticks 607 to 683); the derivation is in the comment on `TAIL_HCOUNT_LO`. The calibrated value is `09`, in the middle of the status window.
- **Files modified:** `tests/fixtures/legacy.c`, `tests/diag/expected.csv`, `tests/roms/src/legacy_sprites.asm`
- **Verification:** record 15 is `0009` on all three profiles; a csv range changed to `0000-00FF` makes `fixtures.legacy.oracle` exit 1.
- **Committed in:** `8761069`

**2. [Rule 3 - Plan variant] The plan's mailbox command omits --frames**
- **Found during:** Task 2 acceptance
- **Issue:** `cogmaster-run legacy_sprites.sms --mailbox` exits 2 (`--frames N or --bench N is required`).
- **Fix:** the working command is `build/ci/cogmaster-run build/ci/fixtures/legacy_sprites.sms --frames 200 --mailbox`, exit 0, which stops when the mailbox is done.
- **Committed in:** not a file change

**3. [Rule 1 - Bug avoided] The csv citations carry no semicolon**
- **Found during:** Task 2 (reading `tests/cmake/mailbox.cmake`)
- **Issue:** the plan's example citation uses semicolons; `file(STRINGS)` rows are CMake list elements, so a semicolon splits a row and `list(GET f 1 ...)` fails.
- **Fix:** citations use colons and parentheses only and no comma; `check_csv` in `legacy.c` splits on commas into exactly six fields and fails otherwise.
- **Committed in:** `8761069`

**4. [Rule 3 - Plan variant] g2_late is compared at frame 640, with a mode 4 variant at frame 40**
- **Found during:** Task 2 (measuring the image on the current core)
- **Issue:** the plan suggests frame 610. The image loads for 16 frames before it counts, so the picture switches at host frame 616.
- **Fix:** `g2_late` (Graphics II) is compared at 640 and a second variant name `g2_late_before` (mode 4, colour RAM entry 0 everywhere) at 40; the second already passes on the current core on all three profiles.
- **Files modified:** `tests/fixtures/legacy.c`
- **Committed in:** `8761069`

**5. [Rule 3 - Plan variant] `literals` takes no image, variant or profile**
- **Found during:** Task 1
- **Issue:** the CLI in the plan is `<image> <variant> <profile> literals|digest|compare`, but one test has one command and the self-check covers every variant on all three profiles.
- **Fix:** `literals` and `rows` ignore the first three arguments, so the registered command is `cogmaster-test-legacy-fixture - all all literals tests/diag/expected.csv`.
- **Committed in:** `fcca930`, `8761069`

---

**Total deviations:** 5 (3 plan variants measured against the core, 1 command correction, 1 bug avoided before it was written)
**Impact on plan:** none widens scope; the tail row, the six pins and every acceptance count are met. No file under `src/` or `include/` changed, and no file the plan did not list changed.

## Issues Encountered

- `docs/conformance.md` documents each fixture family (Pitfall 8, cap 38,912, now 35,855) and the plan's file list does not include it, so the paragraph for the six images is not written here. Plan 09 owns the docs.
- The new images get only their `fixture legacy_*` lines in `runner.hashes_file`; the frame rows and the exact row list (Pitfall 9) are left to the plan that registers the comparisons.
- The pins are checked by the CASE script mode only when plans 05 and 08 register the comparisons that name them; until then a pinned digest is a committed line, not yet a failing test.

## External files read

- ares `ares/ms/vdp/dac.cpp` at pin `9408cb43d494` (ISC), through `gh api`, printed and not copied: confirms the sixteen six-bit palette values `00 00 08 0C 10 30 01 3C 02 03 05 0F 04 33 15 3F` and that its channel expansion `c | c << 2` equals the `c * 5` lattice. No other external source file was opened in this plan; the TI manual, Sean Young's notes, msvdp, the SMS Power palette page and MAME at `1d7a454ed79e` are cited from `16-RESEARCH.md`. No GPL or LGPL source was opened and no PROVENANCE row is needed.

## Evidence

Commands run from the repository root, exit status 0 unless stated, nothing pushed so no run URL: `cmake --preset ci`; `cmake --build --preset ci --target cogmaster-fixtures cogmaster-test-legacy-fixture cogmaster-test-legacy`; `ctest --preset ci -R '^(fixtures\.legacy|vdp\.legacy)'` (2 of 2); `ctest --preset ci -R '^(fixtures|vdp|compat)\.' -j8`; `ctest --preset ci -R '^(fixtures|docs|runner\.hashes_file|compat|policy|vdp|runner)\.' -j8` (351 of 351); `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9386 src lines); `sh scripts/hygiene.sh --tree`; clang-format 21 dry run over `git ls-files '*.c' '*.h'` and over the two new C files; `git diff --name-only origin/main...HEAD -- src include` prints nothing; `cmake --workflow --preset ci` at the head of Task 3 (4c4a6b0): exit 0, 687 of 687 tests and the 5 release tests passed.

## Known Stubs

None. The sprite and mode images record their measurements and judge nothing; the judge rows exist and their comparisons against the renderer are registered in plans 04, 05, 07 and 08.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Measured on the current core (legacy modes fault and paint the backdrop), for the plans that register the comparisons:

- **CLI.** `cogmaster-test-legacy-fixture <image> <variant> <profile> literals|rows|digest|compare [frames or scenario]`; variants `g1`, `g2`, `g2_masked`, `g2_late`, `g2_late_before`, `g2_switch`, `sprites`; profiles `sms1`, `sms2`, `gg` (Game Gear in Master System video mode, NTSC export). `g2_switch` has no digest, its switch line is skipped by `compare` and pinned by hash.
- **Frames.** Static images: the display is on from frame 17, so any frame from 40 on is stable. `g2_late`: 40 for the mode 4 half, 640 for Graphics II (the switch is at 616). `g2_switch`: `SWITCH_LINE` 48; at frame 20 the mixed dots start at x 116, and the dot differs frame to frame, so name the frame when the line's hash is pinned. Sprites: scenario k (1 to 20) is the completed picture at frame `9 + 2 (k - 1)` and the next one; `digest k` and `compare k` use it.
- **Judge.** `legacy_sprites` rows are `*` profile, so the mailbox test needs one registration per profile that names the image and its pin (the CASE mailbox script mode reads `tests/built/expected.txt`).
- **Open items for plan 09.** The conformance paragraph for the six images; wording that the Y threshold, the collision rule and the fifth index are models.
- Nothing is pushed; the work tree is clean after the SUMMARY and state commits.

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

All 13 changed files and the ignored `build/p16-base-hashes.txt` (173 lines) exist; commits fcca930, 8761069 and 4c4a6b0 are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-02 --commits-only` lists the same three; `grep -c '^legacy_' tests/built/expected.txt` is 6, the csv holds 22 `legacy_sprites` rows of which 21 cite `legacy_status`; header.inc, mailbox.inc, boot_to_play.asm and src are unchanged and `git diff --name-only origin/main...HEAD -- src include` prints nothing; the coverage block has no flow style and no inline comment.
