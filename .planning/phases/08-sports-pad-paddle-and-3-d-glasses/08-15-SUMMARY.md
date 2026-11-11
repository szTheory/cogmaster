---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "15"
subsystem: libretro-adapter
tags:
  - libretro
  - sports-pad
  - paddle
  - glasses
  - options
status: complete

requires:
  - 08-13
  - 08-14
provides:
  - "Core options cogmaster_port1 and cogmaster_port2 (auto, pad, paddle, sports, none) and cogmaster_3d_view (left, right, alternate, anaglyph, off) with LR_OPTION_COUNT 9 in the v2, v1 and v0 tables"
  - "Device resolution at load: the user option wins, auto takes the library's table default, a choice the profile cannot take is a pad with one log line"
  - "Sports Pad and Paddle ports sum the mouse delta, the left stick past a 20 percent dead zone at K 6 or 4 and the D-pad at 2 counts, clamp each axis to 32767 and poll only base device ids"
  - "A stereo rule in the adapter: 4 whenever the shutter eye differs from the last one seen, counting down once a frame; left, right and anaglyph apply only while it is above 0"
  - "Six in-process cases libretro.host.devices.{options,sources,clamp,motion} and libretro.host.stereo.{alternate,one_eye} that compare the adapter's picture with the public library's"
affects: [08-16, 08-17]

actuals:
  tokens: 12600
  tasks: 3
  commits: 3

plan_head_before: 483fcdcc94b82f9753f2d172ce2ca54637307cec
plan_head_after: 3b53a73cd8ce005783af8371214804ff9a592147
commits: 3

tech-stack:
  added: []
  patterns:
    - "The adapter is judged against the public library driven by the same scripted input, the table's devices and closed-form sums written out again in the test, so a wrong device, sum or held picture is a different picture"
    - "Negative controls applied with a python edit, confirmed by git diff --stat and a clean build, then restored with git checkout"

key-files:
  created: []
  modified:
    - libretro/libretro.c
    - libretro/options.h
    - tests/libretro/host.c
    - tests/libretro/controls.h
    - tests/libretro/tests.cmake
    - tests/inventory/ci.txt

key-decisions:
  - "A device the profile cannot take is found by probing set_input once per port at load, not by the first frame's call, so the one log line comes at load and each port is judged on its own"
  - "A port with no device (option none or the frontend taking it away) polls no pad buttons at all; pad ports and moving ports poll the pad as before, so a pad-only run sends exactly what it sent"
  - "The eye pictures are kept whenever the core reports an eye, so the held picture of the chosen eye exists the moment the rule turns on; one allocation of three native frames at load, a failed allocation costs only the views"
  - "The stereo cases use two small guests built in the test (left eye with a red backdrop, right with a cyan one, the shutter written every frame or once) because glasses.sms draws nothing and its pictures cannot differ between views"

requirements-completed:
  - PAD-02
  - PAD-03

coverage:
  - id: F1
    description: "Each combination of port option and image (sports_pad.sms with table default Sports Pad on both ports, paddle.sms with Paddle on port 1 under Japan, a pad-only guest) gives the device the table says: the reads the adapter makes, the log lines and the final picture all match the library driven with that device, a Paddle asked for on an export console is a pad with one log line, a frontend taking port 1 away wins, and the picture tells the devices apart"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.devices\\.options$'"
      status: pass
    human_judgment: false
  - id: F2
    description: "Every source in both signs on a Sports Pad and a Paddle port equals the closed form (mouse plus stick times K over 32768 plus the D-pad, dead zone at 6554), the dead zone edge shows in the picture, mouse and RetroPad buttons reach the pins, and no device value beyond JOYPAD, MOUSE and ANALOG with index 0 is ever asked"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.devices\\.sources$'"
      status: pass
    human_judgment: false
  - id: F3
    description: "A mouse delta of -32768 and sums past the range on both axes, on a Sports Pad and a Paddle, are clamped to 32767 so the call is not refused, the buttons stay fresh and no line is logged"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.devices\\.clamp$'"
      status: pass
    human_judgment: false
  - id: F4
    description: "Adapter motion reaches the guest: on sports_pad.sms and paddle.sms a mouse delta, a stick and a D-pad press each end on a picture that differs from the still run, the opposite sign differs from both, and a repeat is identical"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.devices\\.motion$'"
      status: pass
    human_judgment: false
  - id: F5
    description: "The stereo rule and the views: on a guest that alternates the eye every frame each view shows exactly the model's picture (the chosen eye's latest, or the red-cyan mix, while a flip lies in the last four frames; raw otherwise), the library's frames are the same whatever the view, a reset starts the rule again, and glasses.sms ends on the library's picture in every view"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.stereo\\.alternate$'"
      status: pass
    human_judgment: false
  - id: F6
    description: "A guest that sets one eye and keeps it is shown raw in every view and by default, while the alternating guest in the left view is composed"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.stereo\\.one_eye$'"
      status: pass
    human_judgment: false
  - id: F7
    description: "descriptors_calls and options_calls stay 1 per load and per registration, LR_OPTION_COUNT is 9 in all three option tables, every existing libretro case keeps its result, and the ci and hygiene workflows pass"
    requirement: PAD-02
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; ctest -R '^libretro\\.' exit 0"
      status: pass
    human_judgment: false
  - id: F8
    description: "The cases can fail: an adapter that sends zero axes for a Sports Pad fails the options, sources, clamp and motion cases; an adapter that polls a subclass device id fails the same four; an adapter that skips the alternation gate fails both stereo cases"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.(devices|stereo)\\.' with libretro/libretro.c edited, exit 8 in each of the three controls, then restored, exit 0"
      status: pass
    human_judgment: false

duration: 70min
completed: 2026-11-11
---

# Phase 8 Plan 15: The libretro device options and 3-D view Summary

**The libretro adapter picks Sports Pad, Paddle, pad or nothing per port from three core options and the table, feeds them summed and clamped mouse, stick and D-pad motion, and shows a 3-D title through one eye or an anaglyph only while the shutter really alternates; six in-process cases compare its picture with the public library's.**

## What was built

- `libretro/options.h`: `cogmaster_port1` and `cogmaster_port2` (auto, pad, paddle, sports, none; default auto) and `cogmaster_3d_view` (left, right, alternate, anaglyph, off; default left). `LR_OPTION_COUNT` is 9 and the static asserts keep the v2, v1 and v0 tables equal.
- `libretro/libretro.c`:
  - `lr_choose` picks the device at load: the option, else the library's `default_port1/2`. A single `cogmaster_set_input` probe per port tells whether the profile takes it; if not the port is a pad and `lr_log` writes one line through the frontend's log interface.
  - `lr_input` polls only base device ids. A moving port sums the mouse delta, `(stick * K) / 32768` outside a 6554 dead zone (K 6 for a Sports Pad, 4 for a Paddle) and 2 counts for the D-pad, clamps each axis to 32767, and takes buttons 1 and 2 from RetroPad B and A or the mouse buttons into bits 4 and 5. Pad and none ports send what they sent before; a non-OK status of the call is logged once.
  - `lr_stereo` and `lr_view`: `stereo` is 4 when this frame's non-zero eye differs from the last non-zero eye and counts down once a frame; reset at load and `retro_reset`. While it is above 0, left and right present the held picture of that eye and anaglyph calls `cogmaster__eyes_anaglyph` into a retained buffer; alternate, off, a single eye and every other frame go out as the core drew them. The pictures of both eyes are copies of the native RGB444 frame, kept from one allocation made at load.
- `tests/libretro`: the fake frontend gained a log interface and the three options; `controls.h` holds the new cases and a `control_image` split out of `control_load`. Each case compares the adapter with a second library instance fed the same scripted input through the closed forms; the stereo cases compare each frame with a model of the rule built from the library's own eyes and pictures.
- `tests/libretro/tests.cmake` registers `libretro.host.devices.{options,sources,clamp,motion}` and `libretro.host.stereo.{alternate,one_eye}` (labels `libretro portable`, TIMEOUT 60, provisional, with the fault-prevented comment); `tests/inventory/ci.txt` lists them.

## Negative controls

Each was applied with a python edit of `libretro/libretro.c`, confirmed with `git diff --stat libretro/libretro.c` (the build linked), run, and restored with `git checkout -- libretro/libretro.c` and a rebuild.

- Zero axes for a Sports Pad port (3 lines added): `ctest --test-dir build/ci -R '^libretro\.host\.(devices|stereo)\.'` exit 8; options, sources, clamp and motion failed, both stereo cases passed.
- A subclass device id polled for the mouse (`RETRO_DEVICE_MOUSE | (1u << 8)`, 1 line changed): exit 8; the same four failed.
- The alternation gate removed (`if (eye != COGMASTER_GLASSES_NONE)` for the differs-from-last test, 1 line changed): exit 8; `libretro.host.stereo.alternate` and `libretro.host.stereo.one_eye` failed, the four device cases passed.
- Restored, rebuilt: the same ctest exit 0, 6 of 6 passed.

## Verification

- `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(libretro\.|abi\.|policy\.)'`: exit 0 after task 1 (75 of 75); `ctest --test-dir build/ci -R '^libretro\.'`: exit 0 after task 2 (31 of 31) and after task 3.
- `cmake --workflow --preset ci`: exit 0 (542 tests passed). `cmake --workflow --preset hygiene`: exit 0 after each task and after task 3.
- `ctest --test-dir build/ci -N -R '^libretro\.host\.(devices|stereo)\.'`: exit 0, six names; the same six names with `libretro,portable` are in `tests/inventory/ci.txt`; `git diff --quiet origin/main -- tests/retroarch tests/io tests/audio tests/testcard_pins.cmake`: exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0 (723 check lines, 8793 source lines, unchanged).
- `clang-format --dry-run -Werror` over the four C files: exit 0. `gcc:13` in a container, `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `libretro/libretro.c` and `tests/libretro/host.c`: exit 0 (it caught a sign conversion in the test, fixed before the commit).
- A local AddressSanitizer and UndefinedBehaviorSanitizer build of the module and the host (Apple clang, outside the tree): the six cases each exit 0.
- No hosted run was made and nothing was pushed; the plan names no dispatch. The last hosted evidence is run 42973268415 at `60c97a6`.

## Task Commits

1. `9b0d03d` feat(08-15): port device options and summed motion for Sports Pad and Paddle
2. `4d0a9d1` feat(08-15): stereo rule and 3-D views in the libretro adapter
3. `3b53a73` test(08-15): device, clamp and stereo cases on the in-process frontend

## Deviations from Plan

**1. [Rule 1 - Mechanism] The profile check is a probe at load, not the first frame's call**
- **Found during:** Task 1
- **Issue:** The plan says the adapter learns that a device is refused from the first `set_input`; judged per port that needs a retry inside the frame loop and re-polls the frontend.
- **Fix:** `lr_choose` probes `set_input` once per port at load and degrades that port to a pad with one log line. The behaviour the plan states (an unavailable explicit choice is a pad, one log line) is the same.
- **Files modified:** libretro/libretro.c
- **Commit:** 9b0d03d

**2. [Rule 3 - Blocking] glasses.sms cannot show a view difference**
- **Found during:** Task 3
- **Issue:** The fixture draws no picture, so its left and anaglyph frames are the same blank picture and the plan's "buffers differ" control cannot be observed on it.
- **Fix:** The stereo cases run two small guests the test builds (shutter and backdrop written together, every frame or once) and compare each frame with a model of the rule; `glasses.sms` is run in every view and must end on the library's own picture. The "library hashes equal" claim is checked as the library's frame sequence being the same fold whatever the view.
- **Files modified:** tests/libretro/controls.h
- **Commit:** 3b53a73

**3. [Rule 3 - Scope] The log-once for a refused input call has no case**
- **Found during:** Task 3
- **Issue:** After the load probe the public library cannot refuse a clamped call, so the fake frontend has no way to make `set_input` fail; the code path (`status_logged`) is covered by inspection and by the clamp case asserting no line is logged.
- **Fix:** None; stated here as a known gap. The degrade line, which uses the same helper, is asserted.

**4. Order of work.** The code was committed before its cases (red first was not followed); the three negative controls prove the cases can fail instead.

**Total deviations:** 3 (one mechanism, two test-shape), no behaviour change from the plan's truths.

## Known Stubs

None.

## Threat Flags

None. Input arrives through the libretro callbacks already used; each axis is clamped before it reaches the library, and the new allocation is checked.

## Issues Encountered

- The fixture table gives `sports_pad.sms` a Sports Pad on both ports and `paddle.sms` a Paddle on port 1 and a pad on port 2; the first test run assumed a pad on port 2 and the reads check caught it.
- The Paddle fixture saturates under the mixed input, so its cases use a short push and compare the position it reads back.
- `docs/controllers.md` does not exist yet and the plan lists no doc file, so the user-facing sentences (the pad option, the unmeasured mouse delivery) are left to the plan that writes it.

## Next Phase Readiness

Ready for the next plan of phase 8. The branch holds the 08-13, 08-14 and 08-15 commits beyond the pushed `60c97a6`; nothing was pushed here.

## Self-Check: PASSED

- `libretro/libretro.c`, `libretro/options.h`, `tests/libretro/host.c`, `tests/libretro/controls.h`, `tests/libretro/tests.cmake` and `tests/inventory/ci.txt` exist; commits `9b0d03d`, `4d0a9d1` and `3b53a73` are ancestors of HEAD.
- `git rev-list --count 483fcdcc94b82f9753f2d172ce2ca54637307cec..HEAD` printed 3 before this SUMMARY was committed.
