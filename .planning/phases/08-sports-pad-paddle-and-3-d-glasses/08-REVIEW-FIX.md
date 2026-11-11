---
phase: 08-sports-pad-paddle-and-3-d-glasses
fixed_at: 2026-11-11T00:00:00Z
review_path: .planning/phases/08-sports-pad-paddle-and-3-d-glasses/08-REVIEW-CORE.md, 08-REVIEW-HOST.md
iteration: 1
findings_in_scope: 4
fixed: 4
skipped: 0
status: all_fixed
---

# Phase 8: Code Review Fix Report

**Fixed at:** 2026-11-11
**Source reviews:** `08-REVIEW-CORE.md`, `08-REVIEW-HOST.md`
**Iteration:** 1
**Verification ran in:** the main checkout (`build/ci`, `build/hygiene`); no worktree was used.

**Summary:**
- Findings in scope: 4 (CORE WR-01, HOST WR-01, HOST IN-05, HOST IN-03, as the orchestrator chose them)
- Fixed: 4 (1 code, 3 doc-corrected; CORE WR-01 is doc-corrected plus a new pin)
- Skipped: 0
- The other Info findings were not touched (rule 7: seeds).

## Fixed Issues

### CORE WR-01: The Sports Pad never returns to idle after a read — doc-corrected, behaviour pinned

**Files modified:** `docs/controllers.md`, `src/ports.c` (comment only), `tests/ports/test_devices.c`
**Commits:** `4478fe9`, `e94bae0` (comment wrap for the formatter)
**Decision from primary evidence:** the code already matches both references, so the doc sentence and the test
comment were wrong, not the code.

- MAME `src/devices/bus/sms_ctrl/sports.cpp` @`1d7a454ed79e` (BSD-3, read with `gh api`): `m_output` changes only
  inside `out_w` on a TH edge. The timeout callback does `m_phase = 0` and nothing else, so the last nibble stays on
  the pins after a read and after a timeout. `in_r` returns `m_buttons | m_output`.
- ares `ares/ms/controller/sports-pad/sports-pad.cpp` @`9408cb43d494` (ISC): `read` returns `status[index]`; the
  timeout only resets `index` and refills `status` while TH is low. It never presents `$F` either.
- Only the power-on state is `$F` (`m_output(0x0f)`), which is step 0 here. No time-aware pin function is needed,
  so `ports_pins` stays pure with no new state, and no hash changed.

**Applied fix:** `docs/controllers.md` now says the pad shows `$F` before its first read after a load, a reset or a
device change, and afterwards keeps the last nibble until the next TH edge. The `sports_nibble` comment in
`src/ports.c` says the same (no code change). In `tests/ports/test_devices.c` the false "Idle between reads, the pad
presents $F" comment is replaced by checks in `sports_walk` (every machine, region, port and held byte): after a
read the pins show the low nibble of Y (3 for the scripted -3, then 0 after the empty read), and still show it
after `frame()` has run.

**Negative control:** in `src/ports.c` `sports_nibble`, `step > 4u` changed to `step >= 4u` (step 4 presents `$F`,
the reviewer's alternative; `git diff --stat` showed the one-line change), built, and
`ctest --test-dir build/ci -R ports` failed six tests: `ports.devices.sports_walk` (the new pin),
`ports.devices.clamp`, `ports.devices.profiles`, `fixtures.sports_pad.mailbox.export`,
`fixtures.sports_pad.mailbox.japan` and `fixtures.sports_pad.default`. It was reverted (`git diff` on `src/ports.c`
shows only the comment) and the same selection passes again (48 of 48).
**Frame hashes and pins:** unchanged. No fixture hash, mailbox row or `tests/io`, `tests/audio`,
`tests/testcard_pins.cmake` pin was edited.

### HOST WR-01: `docs/embedding.md` still describes the old input model — fixed (doc)

**Files modified:** `docs/embedding.md`
**Commit:** `ca7db7f`
**Applied fix:** the `cogmaster_set_input` paragraph now lists the axes `x1`, `y1`, `x2`, `y2`; says buttons and
devices replace what was pending while axes add up, clamped, until a frame takes them, and that a device change drops
that port's waiting motion (details by link to `controllers.md` "Input"); names the four device constants and
`COGMASTER_STATUS_ERR_PROFILE_REJECTED` for a device the machine and region cannot take; gives the tag/size copy rule
(the smaller of the tag and the struct size, whole fields only, rest zeroed; the struct is 32 bytes without and 48
with the axes). The libretro paragraph says the D-pad, left stick and mouse become axis deltas on a Sports Pad or
Paddle port. Checked against `src/io.c` `cogmaster_set_input`. `docs.api_names` and the doc budgets pass.

### HOST IN-05: Eye buffers copied from a partly uninitialised frame — fixed (code)

**Files modified:** `libretro/libretro.c`
**Commit:** `556017d`
**Applied fix:** `native` is allocated with `calloc(dots, sizeof *native)` instead of `malloc`, so the whole-frame
`memcpy` into `eye_pic` and the anaglyph mix read zeroes outside the filled crop. Behaviour on screen is unchanged
(only the crop is shown). Proof: the existing libretro stereo and control tests still pass (`ctest -R libretro`,
included in the full `ci` run). No new case was added: the fault is a read of uninitialised memory that no CI lane
detects (no MSan or Valgrind lane), and a test that observes it would need a new lane, which rule 7 does not
justify. No negative control applies, since no assertion can see the difference.

### HOST IN-03: The Paddle's effective dead zone is 25 percent, not 20 — fixed (doc)

**Files modified:** `docs/controllers.md`
**Commit:** `38636ae`
**Decision:** the code's dead zone (6554) is the project's own choice, not a reference value, and the integer
division `axis * 4 / 32768` is what the existing test pins ("6554 moves a Sports Pad by one count and a Paddle by
none", `tests/libretro/controls.h`). The doc was corrected rather than the constant K changed, which would have moved
a tested edge and the controller test hashes. The stick bullet now says the first Paddle count starts at 25 percent.

## Verification (command and exit status, main checkout)

| Command | Exit |
|---|---|
| `cmake --workflow --preset ci` | 0 (544 of 544 tests passed in the test stage) |
| `cmake --workflow --preset hygiene` | 0 (50 of 50; a first run failed `hygiene.format` on an over-long comment, fixed in `e94bae0`) |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (723 check lines, 8795 `src` lines; limit 735) |
| `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` | 0 |
| container GCC 13 `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `src/ports.c`, `libretro/libretro.c`, `tests/ports/test_devices.c` | 0 each |

---

_Fixed: 2026-11-11_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
