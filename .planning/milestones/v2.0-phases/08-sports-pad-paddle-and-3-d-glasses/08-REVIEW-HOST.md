---
phase: 08-sports-pad-paddle-and-3-d-glasses
reviewed: 2026-11-11T00:00:00Z
depth: standard
files_reviewed: 35
files_reviewed_list:
  - runner/args.c
  - runner/args.h
  - runner/exit_status.h
  - runner/input.c
  - runner/input.h
  - runner/main.c
  - runner/probe.c
  - libretro/libretro.c
  - libretro/options.h
  - host/eyes.c
  - host/eyes.h
  - tools/cartdb.c
  - src/cartdb.inc
  - tests/cart/fixture_facts.txt
  - scripts/compat-common.sh
  - scripts/library.cmake
  - tests/compat/tests.cmake
  - tests/compat/ratchet.cmake
  - tests/cmake/hashes.cmake
  - tests/runner/tests.cmake
  - tests/fixtures/tests.cmake
  - tests/host/test_eyes.c
  - tests/host/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/controls.h
  - tests/libretro/tests.cmake
  - docs/controllers.md
  - docs/runner.md
  - docs/embedding.md
  - docs/testing.md
  - docs/maintaining.md
  - docs/perf-fuzz.md
  - docs/conformance.md
  - README.md
  - quality.json
findings:
  critical: 0
  warning: 1
  info: 5
  total: 6
  necessity: 0
status: issues_found
---

# Phase 8: Code Review Report (host and tooling half)

**Reviewed:** 2026-11-11
**Depth:** standard
**Files Reviewed:** 35

## Summary

The host and tooling half is sound. I found no Critical issue and no bug in the production runner, libretro or
tooling code.

Verified, not just read:

- `input_read` / `input_axis`: every index read is below `n` (the highest is byte 74 of a 75-byte line). The
  buffer rejects any line of 95 bytes or more before the terminator. A value is at most 99999, so `int32_t` cannot
  overflow. `-32768` and `+32768` are refused. CRLF, a missing final newline and `-00000` are accepted. A short
  file, a surplus record and a NUL byte give exit 2.
- Ran the current `build/ci/cogmaster-run` by hand:
  - CRLF, no final newline and `-00000` pass.
  - A double CRLF at the end exits 2.
  - `--port2 sports --machine gg` exits 77 with the fix named.
  - `--eyes` prints its line.
- Resolved devices reach the input validator, so a motion record for a pad or `none` port is refused before
  frame 1. `--port1/--port2 auto` leaves the library default in force.
- libretro: `lr_axis` cannot overflow `int32_t` (stick x K is at most 196608). The D-pad bit order matches the
  button table (up 0, down 1, left 2, right 3, +y down). Only base device ids are polled (`JOYPAD`, `MOUSE`,
  `ANALOG`). No window, audio-device or input-device code was added beyond libretro callbacks.
- Eye pictures: `eye_pic[0..1]` and `eye_mix` come from one allocation of 3 x `dots` entries and never alias
  `native`. The anaglyph helper writes only `out[0..count)` and never writes its inputs.
- Both stereo views are sound: a view cannot read an eye picture that was never written, because `stereo` becomes
  non-zero only after two different eyes have both been copied.
- `tools/cartdb.c` and `src/cartdb.inc`:
  - The two new rows match `fixture_facts.txt` field for field (`021f62...` is ctrl 2/0, `2046da...` is 3/3, both
    board 1, machine 1, video 1, flags 0).
  - The rows are in sorted position.
  - `catalog.tsv` is unchanged.
  - The generated count is 1,660 rows, which is the number in `docs/maintaining.md`.
- Ledger widening: the awk, CMake and library regexes are identical, and the alternation is anchored by the
  following `;`. The captured tokens (`CMAKE_MATCH_2/3`) are limited to four words before they reach `--port1/--port2`,
  so a ledger row cannot inject an argument. Existing `port1=pad;port2=pad` rows keep their keys and statuses.
- `ctest -N` in `build/ci` lists exactly the 548 names in `tests/inventory/ci.txt`. The inventories are sorted
  with no duplicates. `quality.json` floors (548 and 511) equal the real non-comment counts. Every test family named
  in `docs/testing.md` exists in the inventory. No `.github` file changed. The repository-check budget is 723 x 10 =
  7230 lines against 8793 `src` lines, so it holds.
- No planning IDs appear in code, tests or user docs.

## Warnings

### WR-01: `docs/embedding.md` still describes the old input model

**File:** `docs/embedding.md:9-17` and `docs/embedding.md:27-29`
**Issue:** The paragraph introducing `cogmaster_set_input` was not updated for API 12:

- It lists the fields as `size, p1, p2, pause, reset, start, port1, port2`.
- It says the call "validates every field before replacing pending input". Axes are now summed with what is
  waiting and clamped, and a device change drops that port's motion (`src/io.c` `add_motion` and `axis_add`;
  `docs/controllers.md` "Input").
- It says `COGMASTER_DEVICE_NONE` is the only non-default device.
- The libretro paragraph says "D-pad/B/A map to the six pad bits" and "Pad/none ... descriptors". For a Sports
  Pad or Paddle port the D-pad becomes motion and the mouse and stick are read.

Failure scenario: an embedder reads the primary description, sends the absolute position every frame as if it
replaced the pending value, and the pad moves twice as far, or by far more over several calls between frames.
Only the appended API-12 sentence and `controllers.md` contradict it.
**Fix:** In the first paragraph add the four axes and the two new devices, replace "replacing pending input" with
"buttons and devices replace; axes add up until a frame takes them (see [controllers.md](controllers.md))", and in
the libretro paragraph add "on a Sports Pad or Paddle port the D-pad, left stick and mouse become axis deltas".

## Info

### IN-01: `--probe` silently takes `--port1` and `--port2`, but refuses `--eyes`

**File:** `runner/args.c:211-218`
**Issue:** `check_combination` rejects every run option with `--probe`, now including `--eyes`. It does not check
`port1_given` or `port2_given`, and `--machine`, `--region` and `--video-mode` are not checked either. `--probe`
now prints `port1_default` and `port2_default`, so a user can reasonably run `--probe f.sms --port1 paddle` and read
the output as if the option applied. It exits 0 and ignores it. I confirmed it with `build/ci/cogmaster-run --probe
<seed> --port1 sports`: exit 0, `port1_default=pad`. The other ignored options predate this phase, but the new
probe keys make the gap easier to hit. `docs/runner.md` says `--probe` "runs nothing" and takes no run option.
**Fix:** Add `o->port1_given || o->port2_given` to the usage condition, and reject the other ignored options in the
same place if the stricter reading is intended.

### IN-02: The stereo rule can hold a stale picture for four frames on a non-3-D title

**File:** `libretro/libretro.c:446-479` (`lr_stereo`, `lr_view`)
**Issue:** `stereo` is raised whenever a frame's eye differs from the last non-zero eye. The core reports the eye
sticky, so a title that writes `$FFF8-$FFFB` for its own reasons (the docs name a clear or a fill) can flip the eye
once or twice at a level load. During the next four frames with the `left` view, a RIGHT-eye frame returns
`eye_pic[0]`. That picture is only refreshed by LEFT-eye frames, so the screen shows a held older frame until
`stereo` expires. The docs state the trade-off in general terms but not this visible stall. The default view is
`left`, so every user of such a title sees it.
**Fix:** Judgment call. Either require two flips inside the window before switching to a held picture, or note the
stall in `docs/controllers.md` "The libretro core". The measured behaviour would need a title that clears this way.

### IN-03: The Paddle's effective dead zone is 25 percent, not 20

**File:** `libretro/libretro.c:288-294` and `docs/controllers.md` "The libretro core"
**Issue:** The dead zone is 6554 (20 percent), but a stick at the edge gives `6554 * 4 / 32768 = 0` counts for a
Paddle, and the first count needs `|stick| >= 8192`. The test comment at `tests/libretro/controls.h:644` records
it ("a Paddle by none"), but `docs/controllers.md` says "outside a 20 percent dead zone, K ... 4 for a Paddle".
There is no wrong behaviour, only a doc claim that is slightly off.
**Fix:** Add "(a Paddle needs 25 percent to move by one count)" to the docs, or use K 5 for the Paddle.

### IN-04: `hashes.cmake` splits its device arguments through a shell-style parser

**File:** `tests/cmake/hashes.cmake:86-100`
**Issue:** The row `"--port1 sports --input ${OUT}/tests/sports.input"` goes through `separate_arguments(... UNIX_COMMAND)`.
An `OUT` path containing a space, or on Windows a backslash, would be split or de-escaped, and the lane would then
fail with an unrelated runner error. This is tooling code, so it is a seed candidate only. CI build paths do not
contain spaces today.
**Fix:** Build the argument list with `list(APPEND ... --input "${OUT}/tests/${script}.input")` instead of
re-parsing a string.

### IN-05: Newly allocated eye buffers are copied from a partly uninitialised frame

**File:** `libretro/libretro.c:458-465`, `libretro/libretro.c:640-645`
**Issue:** `native` is `malloc`ed and `cogmaster_get_frame` fills only the `io.w x io.h` crop. `lr_view` then
`memcpy`s all `max_w x max_h` dots into `eye_pic`, and `cogmaster__eyes_anaglyph` reads them all. Only the filled
crop is ever displayed, so nothing wrong reaches the screen. A Valgrind or MSan run of the libretro lane would
report reads of uninitialised memory in the copy and the mix. There is no CI lane that does this today.
**Fix:** `calloc` `native`, or copy and mix only `io.h` rows of `io.w` dots.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
