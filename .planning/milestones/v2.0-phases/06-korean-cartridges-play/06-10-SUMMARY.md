---
phase: 06-korean-cartridges-play
plan: "10"
subsystem: tests
tags:
  - fixtures
  - mapper
  - korean
  - zemina
  - 4pak
  - libretro
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-07 the Zemina, Nemesis and 4 PAK arms; plan 06-08 the board scan; plan 06-09 board_source in the runner probe
provides:
  - tests/roms/src/mapper_korean.asm, one WLA-DX source that builds three 128 KiB headerless images (Korean, Zemina, 4 PAK)
  - three pinned built images, mapper_korean, mapper_zemina and mapper_hes_4pak, judged through the mailbox
  - fixtures.korean_probe and its wrong-source control, and a frame mode in the libretro host with its wrong-digest control
  - a fix to the 4 PAK power-on banks found by the mailbox rows
affects: [06-11]

actuals:
  tokens: 7330
  tasks: 3
  commits: 4

plan_head_before: 3da9c0d
plan_head_after: 300f56ee9c8fbb2c88224394519a211e6f9eecdc
commits: 4

tech-stack:
  added: []
  patterns:
    - "One parameterised source builds several images with -D and one generated link file each, as vdp_lines does"
    - "A stamp at the same offset in every 8 KiB bank, with store sites for the scan kept after bank 0's aligned start stamp"
    - "A script case runs the same check with a wrong expected word as its own negative control"

key-files:
  created:
    - tests/roms/src/mapper_korean.asm
    - tests/roms/src/mapper_korean.link
  modified:
    - src/mapper_ext.c
    - tests/mapper/test_mapper_ext.c
    - tests/fixtures/tests.cmake
    - tests/diag/expected.csv
    - tests/built/expected.txt
    - tests/roms/manifest.txt
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The three images are detected by the instruction scan (board_source heuristic) with no setting, not by tests/cart/fixture_facts.txt rows"
  - "Expected mailbox values were derived from the boards and the stamp layout before any run; one record disagreed and the arm was fixed, not the row"
  - "The 4 PAK $FFFF write is made through LD (HL),A so the scan sees no Sega store site, and a write there moves slot 2 as MAME's inherited Sega register does"

requirements-completed: [MAP-03]

coverage:
  - id: C1
    description: "One parameterised source builds three 128 KiB images with no ROM header, and the runner's scan finds each board with source heuristic and no option set"
    requirement: MAP-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^fixtures\\.korean_probe$' exit 0 (board_source heuristic, mapper_guess korean, zemina and hes_4pak, header_magic 0)"
      status: pass
    human_judgment: false
  - id: C2
    description: "Each image reports its reset stamps, per-slot stamps after register writes, the power-of-two wrap, that writes to the Sega registers leave Korean and Zemina banks alone, and that a 4 PAK write to $FFFF moves slot 2, and every record has a cited row"
    requirement: MAP-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^fixtures\\.mapper_(korean|zemina|hes_4pak)$' exit 0 (13, 12 and 11 records inside their rows in tests/diag/expected.csv)"
      status: pass
    human_judgment: false
  - id: C3
    description: "The probe check fails with a named message when board_source is not the expected word"
    requirement: MAP-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^fixtures\\.korean_probe_control$' exit 0 (expect_fail, exit 1, regex korean_probe: board_source)"
      status: pass
    human_judgment: false
  - id: C4
    description: "The libretro core loads mapper_korean.sms with no core option set, its frame equals the runner's, and the frame depends on the banking because the backdrop is a stamp read through a paged window"
    requirement: MAP-03
    verification:
      kind: e2e
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.korean$' exit 0 (runner digest passed to the host frame mode)"
      status: pass
    human_judgment: false
  - id: C5
    description: "The frame mode fails with a line naming the frame digest when the digest is wrong"
    requirement: MAP-03
    verification:
      kind: e2e
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.korean_control$' exit 0 (expect_fail, exit 1, regex frame digest, all-zero digest of the runner digest's length)"
      status: pass
    human_judgment: false
  - id: C6
    description: "A 4 PAK slot powers up on page 0, showing banks 0 and 1, instead of bank 0 twice"
    requirement: MAP-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mapper\\.' exit 0 (the independent model in tests/mapper/test_mapper_ext.c follows); fixtures.mapper_hes_4pak records 01 to 03"
      status: pass
    human_judgment: false
  - id: C7
    description: "tests/built/expected.txt only gains three lines, no shipped hash or pin moves, and the whole local workflow, hygiene and budgets stay green"
    requirement: MAP-08
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0 (434 tests passed); cmake --workflow --preset hygiene exit 0 (48 of 48); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines of 7957 src lines); git diff origin/main -- tests/built exit 0 and grep -c '^-[^-]' prints 0, grep -c '^+[^+]' prints 3; git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 10: Korean-family fixtures and libretro load Summary

One parameterised WLA-DX source builds headerless Korean, Zemina and 4 PAK images that the runner's scan finds with no setting, each judged through the mailbox against cited rows, and the libretro core loads the Korean image with no option and shows the runner's frame; the mailbox exposed a 4 PAK power-on bank bug, now fixed.

## What was built

- `tests/roms/src/mapper_korean.asm` and `mapper_korean.link`: `FIXTURE_BOARD` 1, 2, 3 selects Korean, Zemina or 4 PAK. Sixteen 8 KiB banks, each with a start stamp at `$0080` (number, complement, text) and an end stamp at `$1FF0`. Bank 0's start pair `$00 $FF` decodes as two one-byte instructions, so the scan stays aligned before the store sites. No ROM header. Records cover reset stamps, per-slot stamps after writes, the wrap, `$FFFC-$FFFF` writes through `LD (HL),A`, and a sweep over every page or bank. The 4 PAK's slot-0 write runs from a work RAM stub. The image ends by painting the backdrop with the stamp read through a paged window and turning the display on.
- `tests/fixtures/tests.cmake`: the three images are built as `vdp_lines` is (one generated link file each), a `probe` case, a `frame` case, `fixtures.mapper_korean`, `fixtures.mapper_zemina`, `fixtures.mapper_hes_4pak`, `fixtures.korean_probe` and `fixtures.korean_probe_control`.
- `tests/diag/expected.csv`: 36 rows (13 Korean, 12 Zemina, 11 4 PAK), derived from the boards before any run, citing MAME `rom.cpp` at 1d7a454ed79e and SMS-CARTRIDGES section 2.4 for the wrap rows; the `$FFFF` row cites `sega8_rom_device::write_mapper`, inherited by `sega8_4pak_device`.
- `tests/built/expected.txt`: three lines added, none changed. `tests/roms/manifest.txt`: one comment line.
- `tests/libretro/host.c`: a `frame` mode (`<module> frame <image> <frames> <digest>`) that answers no core option, loads the image, runs the frames, hashes the last frame and compares it, printing a line naming `frame digest` on a mismatch. `libretro.host.korean` and `libretro.host.korean_control` register it. `tests/retroarch` is untouched.

## Detection

The three images are detected by the scan (`board_source` heuristic, no header), not by `tests/cart/fixture_facts.txt` rows. Store-site counts from `--probe`: Korean 22, Zemina 23, 4 PAK 23, with no Sega or runner-up sites.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] A 4 PAK slot powered up on bank 0 twice**
- **Found during:** Task 2, judging the mailbox against values derived from the board
- **Issue:** `cogmaster__mapext_reset` left the cleared state for the 4 PAK, so windows 1, 3 and 5 showed bank 0, not bank 1, until a register write. A 16 KiB slot on page 0 shows banks 0 and 1. The independent model in `tests/mapper/test_mapper_ext.c` had the same error, so the exhaustive test could not see it.
- **Fix:** a 4 PAK case in `cogmaster__mapext_reset` sets each window to `i & 1`; the model follows.
- **Files modified:** src/mapper_ext.c, tests/mapper/test_mapper_ext.c
- **Commit:** a710e15

**2. [Rule 3 - Blocking] Plan wording of one check**
- The Task 3 acceptance line `grep -c 'libretro' tests/inventory/wasm32.txt` prints 2, both from the file's own header comment (it was 2 before this plan). The intent holds: no libretro test line was added to the wasm32 inventory (`grep -vc '^#'` over libretro matches is 0 of the new lines).

**3. [Plan reading] `requirements-completed`**
- The plan's `requirements:` lists MAP-03 and MAP-08; its output block asks for `requirements-completed: [MAP-03]`. MAP-08's proof (the hosted hash-equality run) belongs to the next plan, so only MAP-03 is marked.

**Total deviations:** 3 (1 bug fix, 1 blocking wording, 1 plan reading). **Impact:** the fix touches `src/mapper_ext.c` (ext boards only, behind `default:` of the mapper switches), so the Sega, Codemasters and ROM-only paths and every shipped hash are unchanged.

## Known Stubs

None.

## Threat Flags

None. Fixture and test code only, plus a reset-state fix in an arm that already existed.

## Self-Check: PASSED

- `tests/roms/src/mapper_korean.asm`, `tests/roms/src/mapper_korean.link` and this summary exist.
- Commits a710e15, ecb34a4, aa486f1 and 300f56e are ancestors of HEAD.
- `cmake --workflow --preset ci` exit 0 (434 passed) and `cmake --workflow --preset hygiene` exit 0 (48 of 48) on the head after the last code commit.
