---
phase: 16-graphics-i-and-ii-draw
plan: 04
subsystem: vdp
tags:
  - graphics-1
  - graphics-2
  - legacy-palette
  - render-dispatch
  - unsupported-modes
  - oracle-tests

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the classifier and the once-per-call dispatch from plan 03, the oracle functions and the base hashes file from plan 02
provides:
  - src/vdp_legacy.c, the Graphics I and II span renderer, the fixed palette and the Game Gear colour RAM palette
  - render_until that draws a legacy line as one span, with the fault and cogmaster__vdp_render_blocked narrowed to the blocked class
  - a backdrop that is the legacy palette entry only while the display is on in Graphics I or II
  - 15 new ci tests (vdp.legacy.* and vdp.unsupported.*) and a 16 mode by 4 profile literal table for the faulting modes
affects:
  - 16-05
  - 16-07
  - 16-09

actuals:
  tokens: 13700
  tasks: 2
  commits: 3
plan_head_before: 2ebad36a799f355240635c9e0ab744d539ccf890
plan_head_after: da72c5fd8be8b2e934c15866e3ac8317e7e27003

tech-stack:
  added: []
  patterns:
    - "a legacy line is one span call; the cell is fetched once per eight dots and the two colours it shows are converted once per cell"
    - "renderer tests load the generated scene at the table addresses the registers select, in the test, and compare with the tile-index oracle, so the address arithmetic of the renderer is checked by a second computation"
    - "a literal profile table (4 characters per nibble) drives the fault test, so the classifier is never asked to grade itself"

key-files:
  created:
    - src/vdp_legacy.c
  modified:
    - src/vdp_render.c
    - src/internal.h
    - CMakeLists.txt
    - tests/vdp/test_legacy.c
    - tests/vdp/tests.cmake
    - tests/vdp/test_background.c
    - tests/ports/test_gun.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - .planning/phases/16-graphics-i-and-ii-draw/deferred-items.md

key-decisions:
  - "the display latch, not the live R1 bit 6, decides whether a legacy line is drawn or is the colour RAM backdrop, exactly as the mode 4 dot path does"
  - "the legacy branch of render_until keeps the per-line sprite scan call with the mode 4 scan rules; no sprite pixel is drawn and no sprite status flag is raised in Graphics I or II until the sprite plans"
  - "cogmaster__vdp_background keeps its checks and still answers the colour RAM backdrop index for a legacy mode; nothing but the mode 4 tests calls it"
  - "the span converts the two colours of each cell, not all sixteen entries per call, because the Light Phaser sensor and status reads ask for spans of a few dots"
  - "vdp.unsupported.* are registered in a second foreach under their own prefix instead of the vdp.legacy.* foreach, because the plan names them vdp.unsupported.*"

requirements-completed:
  - MODE-01

coverage:
  - id: D1
    description: "Graphics I draws the name, pattern and colour tables with the R7 backdrop and the fixed palette on the Master System 1, the Master System 2 and the Game Gear in Master System video mode, equal to the oracle on every dot of lines 0 to 191, at two table layouts, and an all-zero scene shows the backdrop on every dot"
    requirement: MODE-01
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.g1 exit 0; tests/vdp/test_legacy.c g1 compares 192 by 256 dots per profile and layout with oracle_g1 and the hand-computed literal cases"
        status: pass
    human_judgment: false
  - id: D2
    description: "Graphics II draws the three thirds with the Master System 1 masks and unmasked on the Master System 2 and the Game Gear for the eleven R3 and R4 pairs, equal to the oracle; the D-22 mapping is eleven in-process pairs under the plan 02 oracle plus the two ROM images plan 05 runs, so the ROM half of this coverage lands in plan 05"
    requirement: MODE-02
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.g2 and vdp.legacy.masks exit 0; masks runs 33 frames (11 pairs by 3 profiles) and compares the Master System 1 and Master System 2 pictures, which differ for exactly the nine biting pairs"
        status: pass
    human_judgment: false
  - id: D3
    description: "Palette entry n is the fixed table entry on the Master System 1 and 2 and colour RAM entry 16 plus n on the Game Gear in Master System video mode, as a function and on screen"
    requirement: MODE-01
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.palette exit 0; sixteen entries by three profiles against the literal table and the oracle, plus a sixteen cell scene"
        status: pass
    human_judgment: false
  - id: D4
    description: "The backdrop of a picture (cogmaster__vdp_backdrop and completed.backdrop) is the legacy palette entry only with the display on in Graphics I or II and the colour RAM entry otherwise, in 0 to 0xFFF"
    requirement: MODE-01
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.backdrop exit 0; five mode and display configurations by three R7 values by three profiles"
        status: pass
    human_judgment: false
  - id: D5
    description: "The fault and cogmaster__vdp_render_blocked both read display and the blocked class; text, multicolour, the mixed modes, the invalid text modes and native Game Gear record kind 6, device VDP and the nibble as address, the first fault stays, and Light Phaser sensor agrees with the renderer for 16 nibbles by 4 profiles"
    requirement: MODE-04
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.unsupported, vdp.background.unsupported and ports.gun.fault_neutral exit 0; the table is typed literals and covers display-off rows"
        status: pass
    human_judgment: false
  - id: D6
    description: "A register write is exact to its dot (the dot before draws mode 4, the write dot Graphics II), a transient Graphics I bring-up draws and records no fault, and hostile legacy registers over garbage video memory return within the overshoot bound with a monotonic cursor"
    requirement: MODE-06
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.switch, vdp.legacy.bringup and vdp.legacy.hostile exit 0; the in-process half of the mode switch requirement, the ROM fixtures are plan 05"
        status: pass
    human_judgment: false
  - id: D7
    description: "Legacy modes are 192 lines with no scroll and no left-column mask: rows from 192 on are the backdrop (marker rows prove a 224 line picture was rendered), the display bit clear gives the colour RAM backdrop, R8, R9 and R0 bits 5 to 7 change nothing, and calls in any pieces give one picture"
    requirement: MODE-01
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.rows and vdp.legacy.intervals exit 0; whole, per line, every 37th dot and interleaved twins are byte equal, repeated calls move no cursor"
        status: pass
    human_judgment: false
  - id: D8
    description: "No shipped frame or audio hash moved: every line of the base hashes file is in the regenerated head file"
    verification:
      - kind: other
        ref: "build/ci/hashes.txt deleted and regenerated by cmake --workflow --preset ci, then test -z of LC_ALL=C comm -23 of the sorted build/p16-base-hashes.txt and the sorted build/ci/hashes.txt exit 0 (173 base lines, 179 head lines)"
        status: pass
    human_judgment: false
  - id: D9
    description: "The whole ci lane, the five release tests, the hygiene lane (analyzer, no float, format, tree), the budgets and the format dry run pass on the final commit"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 at da72c5f: 702 of 702 tests and the 5 release tests; cmake --workflow --preset hygiene exit 0, 84 of 84, with COGMASTER_TIMEOUT_SCALE 4 under machine load; cmake -DROOT with tests/cmake/budgets.cmake exit 0 (887 check lines, 9555 src lines); clang-format 21 dry run over git ls-files exit 0"
        status: pass
    human_judgment: false

duration: 20 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 04: Graphics I and II draw Summary

**Graphics I and II now draw through render_until with a fixed or colour RAM palette and a display-gated backdrop, equal to the oracles on three profiles, while text, multicolour, the mixed and invalid modes and native Game Gear keep the fault and no shipped hash moved**

## Performance

- **Duration:** 20 min
- **Started:** 2026-11-14T04:15:10Z
- **Completed:** 2026-11-14T04:35:09Z
- **Tasks:** 2 (the first a tracer)
- **Files changed:** 10 in the three code and test commits (1 new, 9 modified), 932 insertions and 36 deletions

## Accomplishments

- **The renderer.** `src/vdp_legacy.c` (97 lines) holds `cogmaster__legacy_span(m, line, x0, x1)` and `cogmaster__legacy_colour(m, n)`. A cell is fetched once per eight dots (and once at the start of a span, which may begin mid-cell): Graphics I reads the name, then the pattern row and the colour byte shared by eight names; Graphics II reads the name with the third of the screen, `line / 64`, as the table index, and on the Master System 1 ANDs that index with `((R4 & 3) << 8) | 255` for the pattern and `((R3 & 127) << 3) | 7` for the colour. Every address is a `uint32_t` masked to 14 bits and none of the three mode 4 address helpers is called. Lines from 192 on, and a display latch of zero, write the backdrop. The comments say model where a choice is a model (palette, Game Gear offset, masks, backdrop gating) and cite the source.
- **The dispatch.** `render_until` now has three branches by class, decided once per call: the blocked class records the first fault and runs the backdrop loop, the legacy class runs one span per line (the per-line sprite scan call stays), and the mode 4 loop is untouched. `cogmaster__vdp_render_blocked` is `display && class == blocked`, the same expression, so the Light Phaser check cannot drift from the fault. `cogmaster__vdp_backdrop` returns the legacy palette entry `R7 & 15` only with the display on in a legacy class, so a power-on frame with the display off still shows the colour RAM backdrop and the 173 base hash lines are all still present.
- **The tests change with the renderer commit.** `vdp.background.unsupported` is a 16 nibble by 4 profile table of typed literals (`4`, `L` or `F`), with display-on and display-off rows: fault rows assert kind 6, device VDP, the nibble as address and the colour RAM backdrop at `field[0][0]` (a native Game Gear word `5AB`), legacy rows assert no fault and the legacy backdrop, and every row asserts `cogmaster__vdp_render_blocked` before the run. `fault_run` in `tests/ports/test_gun.c` now selects the text nibble. The whole ci lane ran green on that commit; no other test flipped.
- **Fifteen named tests.** `vdp.legacy.g1`, `g2`, `palette`, `backdrop`, `masks`, `rows`, `intervals`, `switch`, `bringup`, `hostile` and `vdp.unsupported.text`, `multicolour`, `mixed`, `invalid_text`, `native_gg`, all in `tests/vdp/test_legacy.c`, each under a `# Fault prevented:` comment in `tests/vdp/tests.cmake` (7 such lines in the file now), two inventory lines each in `ci.txt` and `wasm32.txt` (16 `vdp.legacy.*` and `vdp.unsupported.*` names each, with `oracle`).
- **Mutation checks, run by hand and reverted.** Masks off on the Master System 1 fails `masks`; dropping the 192 row limit fails `rows`; reading the old blocked expression fails `native_gg` and `vdp.background.unsupported`; ungating the backdrop fails `backdrop`; reading colour RAM entry n instead of 16 plus n fails nine of the new cases; the third from `line / 32` fails `g2`, `masks`, `rows`, `intervals` and `switch`; swapping the set and clear nibbles fails `g1` and `g2`. The tree was restored after each (`git status` showed only test files changed).
- **Seen by the agent.** `cogmaster-run --trace first-fault` on the five `legacy_modes_*` images on `sms1` and `sms` printed `fault none` for all ten runs, and `--png 30` of `legacy_modes_g1` showed the generated picture, eight cells per colour group with every palette entry present. The ROM hash judging against the oracle digest is plan 05.

## Task Commits

1. **Task 1: Tracer, Graphics I and II drawn end to end through render_until, with the two tests flipped in the same commit** - `bbe5038` (feat)
2. **Task 2: Masks, rows, intervals, switch, bring-up, hostile input and the five unsupported-mode tests** - `088bbb9` (test)
3. **Release-scan size collision and the span cost, found by the final workflow run** - `da72c5f` (refactor, deviation 2)

**Plan metadata:** the SUMMARY commit and the state and roadmap commit that follow this file, both local; no push (the next scheduled push is after the sprites).

## Files Created/Modified

- `src/vdp_legacy.c` - Graphics I and II cell fetch and span, the fixed and Game Gear palettes
- `src/vdp_render.c` - legacy branch of `render_until`, blocked-only fault and `cogmaster__vdp_render_blocked`, gated `cogmaster__vdp_backdrop`
- `src/internal.h`, `CMakeLists.txt` - the two prototypes and the source in the library
- `tests/vdp/test_legacy.c` - instance, scene and comparison helpers; 15 cases beside the oracle case
- `tests/vdp/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - registration and inventory
- `tests/vdp/test_background.c`, `tests/ports/test_gun.c` - the literal table and the text nibble
- `.planning/phases/16-graphics-i-and-ii-draw/deferred-items.md` - two out-of-scope findings

## Decisions Made

- **Display latch.** A legacy line is drawn when `display` is 1 and the class is legacy; otherwise the existing mode 4 loop writes the colour RAM backdrop, which is the unchanged power-on behaviour. R1 bit 6 reaches `display` at the next tick 600, as in mode 4, so `vdp.legacy.rows` sets the register and the latch together.
- **Sprites in the legacy branch.** The per-line sprite scan call stays and uses the mode 4 scan rules; nothing reads its slots in a legacy line and no status bit is raised there. The class-aware scan, the rescan after a class change and the legacy sprite pixels are plans 07 and 08.
- **Two colours per cell.** The first span function converted all sixteen palette entries on every call; the Light Phaser sensor and the status read ask for spans of a few dots, so the converted colours moved to the cell.
- **Test technique for 224 lines.** A fresh instance starts the beam at line 192, so the first `cogmaster_run_frame` ends at a wrap that latches the height from the registers. `vdp.legacy.rows` therefore runs one frame, sets `vdp.height`, fills the rows 192 to 223 with a marker value above `0xFFF`, and runs the second frame: only the renderer can replace the marker.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The no-float scan read a section number in the module header as a literal**
- **Found during:** Task 2 (`cmake --workflow --preset hygiene`)
- **Issue:** `hygiene.no_float` failed on `src/vdp_legacy.c:3`: the header comment wrapped between "section" and "2.4.1", and the scan skips `section N.N` only on one line.
- **Fix:** reflowed the comment (clang-format keeps "section 2.4.1" together). The Task 1 commit carried the wrapped form because Task 1's verify does not include the hygiene lane.
- **Files modified:** `src/vdp_legacy.c`
- **Verification:** `ctest --preset hygiene -R hygiene.no_float` and the whole hygiene workflow exit 0
- **Committed in:** `088bbb9`

**2. [Rule 3 - Blocking] The release scan read the library archive as a copier-headered firmware image**
- **Found during:** final `cmake --workflow --preset ci` (step 5, `release.firmware_scan`)
- **Issue:** `lib/libcogmaster.a` of the macOS arm64 package was exactly 262656 bytes (256 KiB plus 512), one of the five sizes the scan rejects when the member has a NUL byte; the renderer commit landed on it by chance. The 702 tests of the ci lane had passed.
- **Fix:** replaced the sixteen-entry palette conversion per span by two conversions per cell (the cost reason is in Decisions Made). The archive is 262024 bytes and the workflow exits 0. The scan itself is not changed: it is outside this plan's files, and the latent false positive is logged in `deferred-items.md` for the owner or a later plan.
- **Files modified:** `src/vdp_legacy.c`
- **Verification:** `cmake --workflow --preset ci` exit 0 with 702 of 702 and 5 of 5 release tests
- **Committed in:** `da72c5f`

**3. [Plan shape] `vdp.unsupported.*` is registered in a second foreach**
- **Found during:** Task 2
- **Issue:** the plan says to register every new name in the same foreach as `oracle`, but that loop names the tests `vdp.legacy.${case}` and the plan names the new tests `vdp.unsupported.*`.
- **Fix:** a second foreach with its own `# Fault prevented:` line registers `vdp.unsupported.${case}` against the same executable. The counts in the acceptance criteria (16 names in each inventory, at least 7 `Fault prevented` lines) hold.
- **Files modified:** `tests/vdp/tests.cmake`
- **Committed in:** `088bbb9`

---

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking), 1 shape note
**Impact on plan:** none on behaviour. Deviation 2 means the release scan is a gate that can trip on any commit that moves an archive onto one of five sizes; see Issues Encountered.

## Issues Encountered

- `hygiene.tree` timed out at its 60 s limit three times while unrelated processes held the machine at a load average of 30 to 40; `sh scripts/hygiene.sh --tree` exits 0 in 69 s, and `cmake --workflow --preset hygiene` exits 0 with 84 of 84 under `COGMASTER_TIMEOUT_SCALE=4` in `build/hygiene` (reset to 1 after). Same item as 16-01 and 16-03; logged, not fixed.
- The latent size rule of `release.firmware_scan` (deviation 2) is logged in `deferred-items.md`.

## Evidence

Commands from the repository root, exit status 0 unless stated; no push, so no run URL:

- `cmake --preset ci`, `cmake --build --preset ci` (warnings are errors; clean)
- `ctest --preset ci -R '^(vdp\.legacy|vdp\.background\.unsupported|ports\.gun\.fault_neutral)'` (7 of 7 after Task 1), then `ctest --preset ci -R '^vdp\.(legacy|unsupported)'` (16 of 16)
- `ctest --preset ci -j8` after Task 1 (691 of 691)
- `ctest --preset ci -R '^(vdp\.|ports\.gun|core\.|bios\.|policy\.no_title_table)' -j8` (140 of 140)
- `test -z "$(LC_ALL=C comm -23 <(sort build/p16-base-hashes.txt) <(sort build/ci/hashes.txt))"` after deleting and regenerating `build/ci/hashes.txt` (three times, the last at `da72c5f`)
- `cmake --workflow --preset ci` at `da72c5f`: 702 of 702 tests and 5 of 5 release tests (the first run exited 8 on `release.firmware_scan`, deviation 2)
- `cmake --workflow --preset hygiene`: exit 8 on `hygiene.no_float` before the header fix (deviation 1), exit 8 on the `hygiene.tree` timeout under load, exit 0 with `COGMASTER_TIMEOUT_SCALE=4`
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9555 src lines); `git ls-files -z '*.c' '*.h' ':!:libretro/libretro.h' | xargs -0 clang-format --dry-run --Werror` exit 0
- acceptance counts: `src/vdp_legacy.c` tracked; `grep -c src/vdp_legacy.c CMakeLists.txt` 1; the three mode 4 address helpers 0; `0x3FFF` 1; `model` 4; `cart|title|crc32|sha1|serial` 0; inventory names 16 and 16; `Fault prevented` 7; `native_gg` 3; the oracle commit `fcca930` is an ancestor of the renderer commit `bbe5038`
- `cogmaster-run --machine sms1|sms --region ntsc-u --frames 30 --trace first-fault` on the five `legacy_modes_*` images: `fault none` ten times

## Known Stubs

None. Graphics I and II draw no sprites yet and raise no sprite status flag; that is the scheduled work of plans 07 and 08, not a placeholder shown to a user, and the legacy branch keeps the per-line scan call so the saved sprite fields move exactly as they did before this plan.

## Threat Flags

None. The new reads index video memory with a 14-bit mask and add no endpoint, file access or trust boundary.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Plan 05** can judge the `legacy_modes_*` images with this renderer: they run on `sms1` and `sms` with no fault. The `fixtures.legacy.*` family and the MODE-02 ROM half of the D-22 mapping are its work.
- **Plans 06 to 08** (state 17, the sprite engine and the rescan across a class change) start from a legacy branch that calls the mode 4 scan; the sprite pixel and the status flags of legacy modes are theirs. `src/vdp_legacy.c` is the module that grows; it is 97 lines now.
- **`requirements-completed`** names MODE-01 only, as the plan says; MODE-02 and MODE-04 complete with plan 09's docs and MODE-06 with plan 05's fixtures and plan 07's rescan.
- **Release scan.** The deferred item about `release.firmware_scan` sizes matters for the next hosted push (after plan 08); the library grows with the sprite engine, so the macOS arm64 archive is unlikely to stay on 262656.
- `build/p16-base-hashes.txt` (173 lines, ignored build output) is still the base for the subset compare.

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

`src/vdp_legacy.c`, `tests/vdp/test_legacy.c`, `tests/vdp/test_background.c` and `tests/ports/test_gun.c` exist; commits bbe5038, 088bbb9 and da72c5f are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-04 --commits-only` returned them; the acceptance counts above were re-run on the final tree; the coverage block uses block style with no flow mappings and no inline comments.
