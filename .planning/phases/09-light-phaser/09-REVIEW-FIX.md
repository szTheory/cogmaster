---
phase: 09-light-phaser
fixed_at: 2026-11-11T00:00:00Z
review_path: .planning/phases/09-light-phaser/09-REVIEW-CORE.md, .planning/phases/09-light-phaser/09-REVIEW-HOST.md
iteration: 1
findings_in_scope: 3
fixed: 3
skipped: 0
status: all_fixed
---

# Phase 9: Code Review Fix Report

**Fixed at:** 2026-11-11
**Source reviews:** `09-REVIEW-CORE.md` (WR-01), `09-REVIEW-HOST.md` (WR-01, WR-02)
**Iteration:** 1
**Runtime:** Claude Code, claude-sonnet-5-5

**Summary:**
- Findings in scope: 3 (the three Warnings; the Info findings are the orchestrator's seeds and were not touched, except the one test CORE IN-01 asks for, which CORE WR-01 needs)
- Fixed: 3
- Skipped: 0

## Fixed Issues

### CORE WR-01: the wrap finished the old picture's scan with the new picture's height

**Status:** fixed
**Files modified:** `src/gun.c`, `src/internal.h`, `src/vdp.c`, `tests/ports/test_gun.c`
**Commit:** a739450
**Applied fix:** a new `cogmaster__gun_finish` (just the finishing scan) is called at the top of the wrap branch of `cogmaster__vdp_event`, before the height update, as CONTEXT decision 5(d) says. `cogmaster__gun_wrap` keeps the pulse-bit drop and `arm`, and still runs after the height update. Nothing else moved.

**Test (also CORE IN-01):** `walk` gains `walk_height`, four height changes with a Light Phaser on port 1: 224 to 192 (aim row 220, so the disc is cut by the old bottom and its first and last rows have different chords), 224 to 240 (rows 224 to 239 of the field are made bright, as a leftover of a taller picture would be), 192 to 240 and 192 to 224. Two twins are compared with `same_sensor_state`: one read at every tick from the frame-end catch-up (line 192) to the wrap, one not read at all. The mode registers change after the old picture is drawn (line 230). The H latch of both twins must also equal the oracle's value for the last row inside the old picture. No new case name, so the inventories did not change.

**Evidence (commands and exit status):**
- `cmake --build build/ci --target cogmaster-test-ports-gun` exit 0, then `./build/ci/tests/cogmaster-test-ports-gun walk` exit 0 with the fix.
- Negative control: `git checkout -- src`, `git diff --stat -- src` empty (the revert took), `cmake --build build/ci --target cogmaster-test-ports-gun` exit 0 (built), `./build/ci/tests/cogmaster-test-ports-gun walk` exit 1 with `same_sensor_state(...)` and `hcounter == oracle_h(release)` (0x43 != 0x46) failing, in the 224 to 192 case and the 224 to 240 case. The two 192-origin cases stay green with and without the fix (the review says they are not affected, because the frame-end sync finishes a 192-line scan); they are kept as regressions. Patch re-applied, `git diff --stat -- src` shows `gun.c`, `internal.h`, `vdp.c`.
- `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake` exit 0 (no pin or hash changed).
- Container `gcc:13` `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `src/gun.c`, `src/vdp.c`, `tests/ports/test_gun.c` exit 0; `clang-format` 21 `--dry-run -Werror` over the four changed files exit 0.

**Not done:** the `fuzz_pad.c` op. Adding an op changes `OP_COUNT`, which reinterprets the operation byte of every stored seed and cannot be run locally (libFuzzer needs the hosted job), so it is left for the orchestrator to seed (rule 7: test code is a seed). The deterministic case above is the regression guard.

### HOST WR-02: the libretro cursor's stick step was +3 and -4, not 4

**Status:** fixed
**Files modified:** `libretro/libretro.c`, `tests/libretro/controls.h`, `tests/libretro/gun.h`, `docs/controllers.md`
**Commit:** 925c4a7
**Applied fix:** `lr_axis` takes a `nearest` flag. The phaser cursor passes 1 and gets `(stick * k + (stick < 0 ? -16384 : 16384)) / 32768`, which is 4 at +32767 and at -32768. The Sports Pad and Paddle pass 0 and keep the cut toward zero: the existing `libretro.host.devices.*` tests deliberately pin their behaviour (for example "6554 moves a Paddle by none"), and a shared rounding rule failed two of them when tried, so the change is scoped to the new, user-visible case. `docs/controllers.md` now says "rounded to the nearest count so it is 4 either way".

**Test model:** `dev_stick_nearest` in `controls.h` finds the count by comparing the deflection with half steps (n counts once `deflection * k` reaches `(2n - 1) * 16384`), not by the adapter's division, and `dev_sum_by` selects it for the phaser. `gun_cursor_test` asserts the model says literally +4 and -4 at +32767 and -32768, and asserts the cursor moved exactly +4 right and 4 up on the first full-deflection frame (the picture comparison then holds the adapter to that).

**Evidence:**
- `cmake --build build/ci` exit 0 and `ctest --test-dir build/ci -R libretro -j8`: 100 percent passed (40 tests).
- Negative control: the two phaser call sites changed to pass 0 (a one-line edit, `git diff --stat libretro/libretro.c` non-empty), `cmake --build build/ci` exit 0 (built), `ctest -R libretro.host.gun` failed: `tests/libretro/gun.h:377: bad == 0u failed` with 18 to 26 dots differing; restored, `ctest -R libretro` 100 percent passed again.
- `clang-format` 21 `--dry-run -Werror` over the three C files exit 0; container `gcc:13 ... -fsyntax-only` over `libretro/libretro.c` exit 0.

**Left (for the orchestrator to seed):** the Sports Pad and Paddle stick step still has the cut-toward-zero asymmetry (+5 and -6 at full deflection for K 6). It is pinned by existing tests and is not a defect of this phase's feature.

### HOST WR-01: `no_title_table` missed string-initialised arrays and was blinded by `//` or `/*` in a string

**Status:** fixed (it fit the budget)
**Files modified:** `tests/cmake/repository.sh`, `tests/policy/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`, `docs/testing.md`
**Commit:** a918e80
**Applied fix:** the stripper is string aware (a string literal, with its escapes, is copied whole into the scanned code, so a `//` or `/*` inside one starts nothing, and a keyword inside a string is still seen as before), and the `gun.c` array rule is `\] *= *[{"]`. Two self-tests are added, `policy.no_title_table.self_test.strarray` (a string-initialised table at `gun.c:5`) and `policy.no_title_table.self_test.strings` (a `"a//b\"//"` before a hook, and a `"/*"` before a hook on the next line; both hooks must be reported); the helper function now takes the expected regex instead of choosing it by name. Both are in `ci.txt` and `wasm32.txt`, sorted; the `docs/testing.md` row is reworded.

**Budget:** the counted check code is 737 lines (it was 735). The limit given as 737 was stale: the real rule in `tests/cmake/budgets.cmake` is `counted * 10 < src lines`, and `src` is 9237 lines, so up to 923 is allowed.

**Evidence:**
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (`737 check lines / 9237 src lines`).
- Negative control: `git show HEAD:tests/cmake/repository.sh > tests/cmake/repository.sh` (`git diff --stat` empty, the revert took), `ctest -R policy.no_title_table`: `strarray` and `strings` failed, the other four passed; the new file restored and all six passed.
- The check and the six scenarios were also run under `mawk` and `gawk` in a `gcc:13` container with `sh tests/cmake/repository.sh no_title_table` (array, hook, strarray and strings exit 1 with the expected lines; comments and `src` exit 0); macOS awk is what ctest used.

**Residual gap (documented, not closed):** a character literal holding a double quote (`'"'`) would still open a string for the stripper (no such literal exists in the five files), and a ROM header read through a helper (`cogmaster__cart_byte(m, 0x7ff0)`) matches none of the tokens, by design. `ports.gun.identity` is the behavioural backstop for both.

## Whole-phase evidence (after the last fix commit)

- `cmake --workflow --preset ci` exit 0.
- `cmake --workflow --preset hygiene` exit 0 (50 of 50 tests passed).
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0.
- `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake` exit 0.
- Verification ran in the main checkout (`build/ci`), no isolated worktree was created: the review files are untracked and the build tree lives there.

## Not touched (the orchestrator's seeds)

CORE IN-02, IN-03, IN-04; HOST IN-01, IN-02, IN-03, IN-04; `.planning/seeds/SEED-010-pad-device-notes.md` was modified by the orchestrator while this ran and was not committed here.

---

_Fixed: 2026-11-11_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
