---
phase: 16-graphics-i-and-ii-draw
plan: 07
subsystem: vdp
tags:
  - legacy-sprites
  - sprite-scan
  - fifth-sprite
  - coincidence
  - class-rescan

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the Graphics I and II span renderer and the mode class of plan 04, the sprite oracle of plan 02, the widened slots, the saved fifth sprite index and the status read of plan 06
provides:
  - cogmaster__legacy_sprite_scan, the Graphics I and II scan of 32 entries into four slots with the colour byte under the mask 0x8F and the fifth match carried as its table index plus 1
  - a per-dot legacy sprite pixel in cogmaster__legacy_span, with the coincidence flag at its dot and the fifth sprite flag and its index at dot 0
  - the scan dispatch on the mode class in cogmaster__vdp_sprite_scan, with mode 4 storing attr 0 and sprite_legacy 0
  - the once-per-call rescan of the current line when the live class differs from the class the sprite list was scanned under
  - eleven vdp.legacy.sprites.* cases in both inventories and a scanned-sprite sub-case of core.state.legacy_roundtrip
affects:
  - 16-08
  - 16-09

actuals:
  tokens: 12290
  tasks: 2
  commits: 4
plan_head_before: acd6c3d9967267a056f7401e281b5013625cfd51
plan_head_after: e760fc946d703a6a2d84a0ed88a39a9566574255

tech-stack:
  added: []
  patterns:
    - "a status candidate that must survive a status read is carried in a saved counter as its index plus 1 and copied into the latch at the dot the flag asserts, gated on the flag bit rather than on the latch value"
    - "a list scanned under one class is checked once per render call against the live class and rescanned for the current line, never per dot"
    - "a sprite oracle that walks the table per line is compared with the renderer on every dot and on the status byte, with hand-computed literals for the dots that tell a rule from its neighbour"

key-files:
  created: []
  modified:
    - src/internal.h
    - src/vdp_legacy.c
    - src/vdp_render.c
    - tests/vdp/test_legacy.c
    - tests/vdp/tests.cmake
    - tests/core/test_state.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The fifth sprite candidate is stored as 1 + table index in sprite_overflow by the scan and copied into fifth_index at dot 0 only while status bit 6 is clear; gating on fifth_index being 31 would overwrite a latched index of 31 and gating on nothing would lose first-wins"
  - "Lines from 192 on in a legacy picture draw the backdrop and assert nothing: the span returns before the sprite code, so a picture that latched 224 lines and then selects Graphics I shows no sprite and no flag there, while the scan still runs per line and keeps its state valid for the loader"
  - "The rescan sits after the class is computed and before the fault check, and is skipped for the blocked class, which draws no sprite; a register write that changes the class between the scan at tick 600 and the dots rescans the current line at the first render after the write"
  - "The mode 4 scan stores sprite_legacy 0 and attr 0 for the slots it fills and does not touch sprite_size, which the loader bounds 0 to 1 in both classes"
  - "The legacy sprite tests put the generator at 0x1800 (R6 3), the one 2 KiB aligned block clear of the Graphics II pattern and colour tables, and the attribute table at 0x3F80 (R5 0x7F)"

patterns-established:
  - "mutation runs that restore from a pristine copy kept outside the tree, and refuse to run without one"

requirements-completed: []

coverage:
  - id: D1
    description: "The Graphics I and II sprite scan reads 32 entries at (R5 and 0x7F) shifted left by 7, fills four slots with x, name (0xFC for 16x16), row and the colour byte under 0x8F, ends the list at Y 0xD0 unconditionally and carries the fifth match as its index plus 1"
    requirement: MODE-03
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.sprites.basic and vdp.legacy.sprites.terminator exit 0; mutations that dropped the 0xFC mask, gated the terminator on a 192 line height and offset the fifth index each failed a named case"
        status: pass
    human_judgment: false
  - id: D2
    description: "The per-dot sprite pixel draws 8x8 and 16x16 sprites (quadrants TL, BL, TR, BR), magnification with plain doubling on every profile, the early clock from attribute bit 7 with R0 bit 3 ignored, colour 0 transparent, the lower table index first, always over the background"
    requirement: MODE-03
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.sprites.sizes, vdp.legacy.sprites.zoom and vdp.legacy.sprites.early_clock exit 0; each compares every dot of lines 0 to 191 with oracle_sprites on three profiles, over a blank, a Graphics I and a Graphics II background, and checks hand-computed dots; mutations of the quadrant order, the horizontal zoom, the early clock bit and an R0 bit 3 shift each failed"
        status: pass
    human_judgment: false
  - id: D3
    description: "Y 0xFF shows row 0 on line 0, Y 0xE0 or more is negative (16x16 magnified at 0xE8 shows rows 11 to 15 on lines 0 to 8, at 0xE0 line 0 shows row 15), and a Y of 0xD0 at entry 0 draws no sprite, sets no flag and leaves the low status bits at 0x1F, also when a mode 4 frame latched 224 lines"
    requirement: MODE-03
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.sprites.y_wrap and vdp.legacy.sprites.terminator exit 0; mutations to above 0xE0 and to 0xF0 or more each failed y_wrap"
        status: pass
    human_judgment: false
  - id: D4
    description: "The fifth Y match on a line sets status bit 6 at the line's first dot with its table index in bits 4 to 0, the first since a read stays, bits 4 to 0 read 0x1F with bit 6 clear, a second read returns 0x1F and the latched index is 31 after any read, and the candidate survives a status read between the tick 600 scan and dot 0"
    requirement: MODE-03
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.sprites.fifth, vdp.legacy.sprites.fifth_carry and vdp.legacy.sprites.status_low exit 0; mutations that let the later line win, offset the index, allowed five slots, dropped the copy at dot 0, moved the assertion off dot 0 and dropped the index reset each failed a named case"
        status: pass
    human_judgment: false
  - id: D5
    description: "Coincidence follows the datasheet model: two set pattern bits on one dot of 0 to 255 set bit 5 at that dot, colour 0 sprites included, with sprites that only touch, sprites after the terminator, sprites dropped as fifth and overlaps before dot 0 or past dot 255 not colliding"
    requirement: MODE-03
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.sprites.collision exit 0; mutations to opaque-only counting, a single-sprite hit, a touching overlap and a wrapping X each failed"
        status: pass
    human_judgment: false
  - id: D6
    description: "A sprite list scanned under one class is rescanned at the start of a render call under the other, so a mid-line R0 write between mode 4 and Graphics I draws the live class's sprites from the write dot, in both directions"
    requirement: MODE-06
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.sprites.class_switch exit 0; the case failed at the test commit c2a2824 without the rescan and passes at e760fc9"
        status: pass
    human_judgment: false
  - id: D7
    description: "The mode 4 sprite engine is unchanged and no shipped pin moved: mode 4 scans store attr 0 and sprite_legacy 0, the shipped mode 4 sprite tests pass, and the subset compare of the base and head hashes files prints nothing"
    requirement: MODE-05
    verification:
      - kind: other
        ref: "test -z of LC_ALL=C comm -23 of the sorted build/p16-base-hashes.txt and the sorted build/ci/hashes.txt exit 0 (173 base lines, 192 head lines) after runner.hashes_file"
        status: pass
      - kind: unit
        ref: "ctest --preset ci -R vdp.sprites and vdp.background exit 0 as part of cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D8
    description: "A legacy frame with scanned sprites, one colour byte having bits 6 to 4 set in video memory, saved mid-line loads into a fresh instance and saves byte-equal, and the slots hold the colour byte under 0x8F"
    requirement: MODE-05
    verification:
      - kind: unit
        ref: "ctest --preset ci -R core.state.legacy_roundtrip exit 0; dropping the mask or the class flag at the scan failed the case"
        status: pass
    human_judgment: false
  - id: D9
    description: "The whole ci lane, the budgets, the format dry run and the hygiene lane (hygiene.analyzer and hygiene.no_float included) pass on the final commit, and no sprite model is keyed on a cartridge id, digest or title"
    requirement: MODE-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 at e760fc9 with 731 of 731 tests and 5 of 5 release tests"
        status: pass
      - kind: other
        ref: "cmake -DROOT with tests/cmake/budgets.cmake exit 0 (887 check lines, 9738 src lines); clang-format 21 dry run over git ls-files exit 0; cmake --workflow --preset hygiene exit 0, 84 of 84; ctest --preset ci -R policy.no_title_table exit 0"
        status: pass
    human_judgment: false

duration: 18 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 07: Legacy sprite engine Summary

**Graphics I and II sprites (32 entries into four slots, 8x8 and 16x16, magnification, early clock, colour 0 transparent, fifth sprite flag with its index carried across a status read, datasheet coincidence) with the class-change rescan, eleven in-process cases against the plan 02 oracle on three profiles, and the mode 4 engine and every shipped pin untouched**

## Performance

- **Duration:** 18 min
- **Started:** 2026-11-14T05:11:21Z
- **Completed:** 2026-11-14T05:29:35Z
- **Tasks:** 2 (the first a tracer; the second split into a test commit that fails and the commit that makes it pass), plus one comment fix
- **Files changed:** 8 (812 insertions, 20 deletions)

## Accomplishments

- **The scan** (`cogmaster__legacy_sprite_scan`, `src/vdp_legacy.c`). It reads 32 entries of Y, X, name and colour byte at `(R5 & 0x7F) << 7`, ends the list at Y `0xD0` before any other test, treats Y `0xE0` and up as negative (`top = Y - 255`, else `Y + 1`) and takes four slots. A slot keeps X, the name (`& 0xFC` for 16x16), the row (`delta >> zoom`) and the colour byte under `0x8F`, so a game that writes bits 6 to 4 still saves a loadable state. The fifth match stops the scan and leaves its table index plus 1 in `sprite_overflow`. The scan latches `sprite_size` and `sprite_zoom`, sets `sprite_legacy` and `sprite_shift` 0 (R0 bit 3 is ignored).
- **The pixel and the flags** (`cogmaster__legacy_span`). For each slot, `left = x - (EC ? 32 : 0)`, the column is `dx >> zoom`, a 16x16 sprite adds `2 * (column >> 3) + (row >> 3)` to the name, and the pattern address is `((R6 & 7) << 11 | name << 3 | row & 7) & 0x3FFF`. The first slot with a set bit and a non-zero colour gives the dot over the background, with no priority; two set bits on a dot set status bit 5 at that dot. At dot 0 with the display on, a non-zero `sprite_overflow` with status bit 6 clear sets `fifth_index` to `sprite_overflow - 1` and the flag, so a read in the 84 ticks after the scan at tick 600 of the line before does not lose the index.
- **The dispatch and the rescan** (`src/vdp_render.c`). `cogmaster__vdp_sprite_scan` calls the legacy scan when the class is LEGACY; the mode 4 body stores `sprite_legacy` 0 and `attr` 0 and is otherwise as it was. `render_until` rescans the current line once when the list's class differs from the live one: one compare per call, none per dot.
- **Eleven cases.** `vdp.legacy.sprites.basic`, `sizes`, `zoom`, `early_clock`, `terminator`, `y_wrap`, `fifth`, `fifth_carry`, `collision`, `class_switch` and `status_low` drive the renderer in process on the Master System 1, Master System 2 and Game Gear in Master System mode, compare every dot of lines 0 to 191 and the status read with `oracle_sprites` and `scene_status` (a per-line walk of the table), and add hand-computed dots chosen to separate a rule from its neighbour (the quadrant order, the early clock bit, Y `0xE0` against above `0xE0` and `0xF0`, a lone magnified sprite on the Master System 1). `core.state.legacy_roundtrip` gained a frame whose scan filled four slots from colour bytes with bits 6 to 4 set, saved mid-line.
- **Mutations.** Each rule has at least one mutation of the engine that failed a named case: 32 in all, among them the 0xFC mask, the quadrant order, horizontal zoom, an R0 bit 3 shift, the early clock offset, the terminator gated on height, Y above 0xE0 and 0xF0 or more, first-wins, the index offset, five slots, the carry, the dot 0 assertion, opaque-only collision, a single-sprite hit, touching sprites, a wrapping X, the index reset and the missing rescan. One survivor, a status read that ignores bit 6, is equivalent on every reachable state (the latched index is 31 whenever the bit is clear), as plan 06 noted.

## Task Commits

1. **Task 1: Tracer, one legacy sprite from the table through the scan, the slots and the pixel** - `8118091` (feat)
2. **Comment fix (deviation 1)** - `45ec9b1` (fix)
3. **Task 2, tests: every legacy sprite rule and the class change** - `c2a2824` (test; `class_switch` fails here, the other ten pass)
4. **Task 2, rescan: scan a line's sprites again when a register write changes the class** - `e760fc9` (feat; all eleven pass)

**Plan metadata:** the SUMMARY commit and the state and roadmap commit that follow this file, both local; no push (the next scheduled push is plan 08's).

The tracer gate was met by re-running the Task 1 verify chain (format, configure, build, the targeted ctest, `runner.hashes_file`, the subset compare) before expanding: exit 0, 113 of 113 and 1 of 1.

## Files Created/Modified

- `src/internal.h` - the `cogmaster__legacy_sprite_scan` prototype
- `src/vdp_legacy.c` - the scan, the sprite pixel, the coincidence flag and the dot 0 assertion, with the model choices and the disagreeing sources named in comments
- `src/vdp_render.c` - the class dispatch of the scan, the mode 4 stores, the rescan in `render_until`
- `tests/vdp/test_legacy.c` - the eleven cases and their helpers (`sprite_frame`, `sprite_scene`, `scene_status`, `fifth_match`), `struct scene` widened to 12 entries, the hostile case varying the sprite size and magnification bits
- `tests/vdp/tests.cmake` - the cases in the legacy foreach under the sprite group's `# Fault prevented:` comment
- `tests/core/test_state.c` - `legacy_sprite_instance` and the scanned-sprite sub-case
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - eleven lines each

## Decisions Made

- **The candidate and its gate.** The scan keeps the fifth match as table index plus 1 in `sprite_overflow`; dot 0 copies it only while status bit 6 is clear. A gate on `fifth_index == 31` would overwrite a latched index of 31 with a later line's, and no gate would lose first-wins.
- **Lines from 192 on.** The span returns the backdrop before any sprite code for them, so a picture that latched 224 lines and selects Graphics I mid-frame shows no sprite and sets no flag there. The scan still runs, which keeps the saved fields inside the loader's bounds.
- **Where the rescan sits.** After the class is computed, before the fault check, skipped for the blocked class (it draws no sprite).
- **Mode 4.** The scan stores `sprite_legacy` 0 and `attr` 0 for the slots it fills and leaves `sprite_size` alone (0 to 1 in both classes at the loader).
- **Test layout.** The sprite generator is at `0x1800` (R6 3), the one 2 KiB block clear of the Graphics II tables, the attribute table at `0x3F80` (R5 `0x7F`), so one scene loader serves a blank, a Graphics I and a Graphics II background.
- **Test-first split for Task 2.** The engine rules of Task 2 were written in Task 1 as the plan orders (the tracer carries the whole scan), so only the rescan was new behaviour; the test commit leaves `vdp_render.c` as it was and `class_switch` fails there, then the rescan commit makes it pass.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Three comment lines read as floating-point numbers by hygiene.no_float**
- **Found during:** Task 2, the hygiene workflow (`hygiene.no_float` failed, 83 of 84)
- **Issue:** the scan strips `section 2.4.1` forms but reads `2.4.5` or `2.3.2` after any other word as a number with a decimal point; three comment lines of Task 1 had that form. Task 1's verify chain does not run the hygiene lane, so it passed there.
- **Fix:** each citation says `section` before the number, as the existing ones do. No code changed.
- **Files modified:** `src/vdp_legacy.c`
- **Verification:** `ctest --preset hygiene -R hygiene.no_float` exit 0, then `cmake --workflow --preset hygiene` exit 0, 84 of 84
- **Committed in:** `45ec9b1`

**2. [Rule 2 - Missing test coverage] The hostile case never set the sprite size or magnification bits**
- **Found during:** Task 2, reading `vdp.legacy.hostile` against the new engine
- **Issue:** it forces R1 to `0xC0` or `0xD0`, so the bounded call over a hostile table was only ever checked with 8x8 unmagnified sprites; the 16x16 and magnified paths (a row up to 15, a 32 dot width) had no hostile run.
- **Fix:** R1 keeps the random low two bits (`0xC0 | (value[1] & 3)`), which draws the same random sequence as before.
- **Files modified:** `tests/vdp/test_legacy.c`
- **Verification:** `ctest --preset ci -R vdp.legacy.hostile` exit 0
- **Committed in:** `c2a2824`

### Plan variants

- **Names of the cases.** They are registered in the existing legacy foreach as `sprites.<case>` arguments, so the test names are `vdp.legacy.sprites.<case>` as the plan lists and the foreach stays one loop with the `# Fault prevented:` comment before the sprite names.
- **Scene capacity.** `struct scene` holds 12 entries instead of 8: the two-line fifth sprite scene needs ten (five matches on line 100 whose fifth is entry 4 and five on line 40 whose fifth is entry 9).
- **Task 2 as test then fix.** See Decisions Made.

---

**Total deviations:** 2 auto-fixed (1 bug, 1 missing test)
**Impact on plan:** no scope widening; the hostile change adds coverage to the new engine and the comment change touches no code.

## Issues Encountered

- A mutation run restored the file from a pristine copy that did not exist yet, so a mutated `src/vdp_render.c` (the dispatch removed) stayed in the working tree for one command. The changed-file notice showed it; the dispatch was put back by hand, the diff against the intended text read, the mutation script now refuses to run without a pristine copy, and nothing mutated was ever committed.
- `clang-format` is not on the PATH of this shell; the dry runs used `/opt/homebrew/opt/llvm@21/bin/clang-format` (version 21), exit 0 over every tracked `.c` and `.h` file.
- `hygiene.tree` was not the slow test this time: the hygiene workflow took 54 s in all, 84 of 84.

## Evidence

Commands from the repository root, exit status 0, no push so no run URL:

- `cmake --preset ci`, `cmake --build --preset ci` (warnings are errors)
- `ctest --preset ci -R '^(vdp\.|core\.state|bios\.|ports\.gun)' -j8` (113 of 113, Task 1), `ctest --preset ci -R '^(vdp\.|core\.state|bios\.|policy\.no_title_table|runner\.hashes_file$)' -j8` (112 of 112, Task 2), `ctest --preset ci -R '^(vdp\.|core\.|bios\.)' -j8` (129 of 129)
- `rm build/ci/hashes.txt`, `ctest --preset ci -R '^runner\.hashes_file$'`, then `test -z "$(LC_ALL=C comm -23 <(sort build/p16-base-hashes.txt) <(sort build/ci/hashes.txt))"` (exit 0; 173 base lines, 192 head lines, as in plans 05 and 06: this plan adds no frame row)
- `cmake --workflow --preset ci` at `e760fc9`: 731 of 731 tests and 5 of 5 release tests
- `cmake --workflow --preset hygiene`: 84 of 84; `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9738 src lines); clang-format dry run over `git ls-files '*.c' '*.h'`
- acceptance counts from both task blocks (`cogmaster__legacy_sprite_scan` 1 in `src/vdp_render.c`, `0x8F` 1, `Fault prevented` 8, `SAT index|fifth match` 2, 11 `vdp.legacy.sprites.` lines in each inventory, `model` 6 in `src/vdp_legacy.c`, none of cart, title, crc32, sha1, serial there), all as expected

## Known Stubs

None. Every field the scan writes is read by the pixel, the flags or the saved record; nothing returns a placeholder.

## Threat Flags

None. The scan and the pixel index video memory only through the 14-bit mask, loop over at most 32 entries and four slots and 256 dots, and add no endpoint, file access or trust boundary; the hostile case runs random tables with random sizes and magnification.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Plan 08** judges the `legacy_sprites` image against `legacy_status` in `tests/fixtures/legacy.c`. The engine reproduces its rules in process: the status after a frame is the frame flag, bit 5 when any line of 0 to 191 has two set pattern bits on a dot of 0 to 255, and bit 6 with the index of the first fifth match in raster order, else `0x1F`; `fifth_carry` shows the tail read (tick 640 of line L, then dot 0 of line L+1) in process, but only the image reads it from a real line interrupt, which is plan 08's evidence. Two sprites at X `0xFF` and the Y `0xE0` row are in the in-process cases too.
- **Plan 09** owns the sentences for `docs/hardware/video.md`; the model choices to name, with their disagreeing sources, are in the comments of `src/vdp_legacy.c`: the Y threshold (`>= $E0`; MAME's TMS file above `$E0`, its Master System path `$F0` or more), the early clock from attribute bit 7 with R0 bit 3 ignored (msvdp says it shifts by 8), the first fifth sprite wins and bits 4 to 0 read `$1F` otherwise, colour 0 collides (the Master System path of MAME counts opaque dots, ares an asymmetric rule), collision only on dots 0 to 255, plain magnification with no slot rule, and no sprite or flag on lines from 192 on.
- **Timeouts.** Each new case runs in about 0.01 s against the 10 s tier; nothing to re-derive.
- **Requirements.** `requirements-completed` is empty as the plan says: MODE-03 completes with plan 08's fixture and MODE-06 with plan 08's evidence.

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

The eight changed files exist; commits 8118091, 45ec9b1, c2a2824 and e760fc9 are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-07 --commits-only` lists them; `git diff --name-only acd6c3d..HEAD` lists only the eight files; the acceptance counts of both tasks and the plan-level verification were re-run on the final tree (all exit 0, the subset compare printed nothing); the coverage block uses block style with no flow mappings and no inline comments.
