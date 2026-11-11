---
phase: 09-light-phaser
plan: "10"
subsystem: libretro-adapter
tags:
  - libretro
  - gun
  - tests
status: complete

requires:
  - 09-04
  - 09-09
provides:
  - a Light Phaser controller type (RETRO_DEVICE_LIGHTGUN as a base id) and the lphaser value of cogmaster_port1 and cogmaster_port2 in libretro, with three controller types per port and 18 input descriptors
  - a pointer aim through the last frame's rectangle and a RetroPad, stick and mouse cursor that always works, both sent as the 1-based field aim; Pause and Reset read the gun's Start and Select on port 0
  - a crosshair drawn into the converted output copy only, in the native RGB444 layout's colours, with the cogmaster_crosshair and cogmaster_crosshair_colour options
  - libretro.host.gun.aim, libretro.host.gun.cursor and libretro.host.gun.crosshair on their own fake frontend and an assembled guest
affects: [09-11, 09-12]

actuals:
  tokens: 8900
  tasks: 3
  commits: 2

plan_head_before: 98a6c73340ad7045438ec51c2ef409c7b3dcec85
plan_head_after: 8c69daa6c36ef6ef189aee5baf83d6d81ac7e93f
commits: 2

tech-stack:
  added: []
  patterns:
    - "A fake frontend per device family: gun.h answers the light gun, control pad, mouse and stick queries and counts which ids were asked, while the shared fake in controls.h stays pad-only"
    - "A guest assembled in the test that reports what it saw as the colour of a patch in the picture (sensor, trigger and Pause bits), read back through the presented frame"
    - "The presented picture compared dot for dot with a direct library instance driven by the same aims, so a crosshair is proven to differ from the library's frame in exactly the cross dots"

key-files:
  created:
    - tests/libretro/gun.h
  modified:
    - libretro/libretro.c
    - libretro/options.h
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/inventory/ci.txt

key-decisions:
  - "The crosshair is drawn inside retro_run after the conversion loop (not in a helper above it) so the order is visible in the file, and it uses the rectangle just completed; the aim itself is mapped through the previous frame's rectangle"
  - "A light gun on a port whose option is not the phaser is ignored; a Sports Pad or Paddle also reads the mouse and stick only when the frontend device is a control pad"
  - "The crosshair colour table is the project's native layout 0000BBBBGGGGRRRR: white 0x0FFF, red 0x000F, green 0x00F0, yellow 0x00FF"

requirements-completed: []

coverage:
  - id: C1
    description: "libretro offers a Light Phaser controller type as a base RETRO_DEVICE_LIGHTGUN id (no subclass), three types per port, 18 input descriptors (trigger for both ports, Pause / Start and Console Reset for port 0), the lphaser value of both port options in the v2, v1 and v0 option tables and 11 options in all; the edited host cases expect 3 types, descriptors of JOYPAD or LIGHTGUN and 18 of them, with one descriptor call and one options call"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.' exit 0 (40 tests); cmake --build --preset ci with the option-count static asserts exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "A frontend pointer maps through the picture's rectangle to the field aim the guest senses: the lit square's centre, left, right, top and bottom edge dots sense light, dots 9 or more off sense none, 16 off with the trigger shows only the trigger, IS_OFFSCREEN with the pointer on the square and either coordinate -32768 sense none, the reload id is never asked, and a port whose option is a pad ignores the gun"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.gun\\.aim$' exit 0; control that maps through a 240-line rectangle instead of the crop turned it red (ctest exit 8), reverted, exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "The cursor path works with a control pad: it starts centred, the D-pad moves it 2 a frame, the stick 4 counts at full deflection and nothing inside the dead zone, the mouse delta adds, the sum is held at the edge in all four corners with the cross clipped there, retro_reset re-centres it, RetroPad B or A or the left mouse button reaches the guest's TL, no gun id is asked of a pad port, and with a gun on port 0 one Pause press reaches the guest's NMI once while Start and Select of the gun are read"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.gun\\.cursor$' exit 0; three one-shot controls (no cursor reset on retro_reset, Pause read only from the pad, a stick scale of 5) each turned it red (exit 8), reverted, exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "The crosshair is drawn into the presented output only: with the option off the presented picture equals the library's picture byte for byte through three phases; on, it differs from the library's picture in exactly the 17 dots around each aim (two guns draw both, 34 dots), in the exact colour of each of the four choices in the native layout, a moved aim leaves no dot of the old cross, an off-screen aim draws nothing, and with no phaser attached the picture is byte-equal to the library's"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^libretro\\.host\\.gun\\.crosshair$' exit 0; controls that draw into native instead of the output buffer, that use the R-high colour order and that map through the wrong rectangle each turned it red (exit 8), reverted, exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The three cases are registered with the same names in the ci inventory only, the shared fake and the hosted RetroArch tests are unchanged, the changed C compiles without warnings under gcc 13 in all three flavours of the core, and every other ci and hygiene test is unchanged"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (584 tests); cmake --workflow --preset hygiene exit 0 (50 tests); docker gcc:13 -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only on libretro/libretro.c (default, RGB565 and Game Gear flavours) and tests/libretro/host.c exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; git diff --quiet origin/main -- tests/libretro/controls.h tests/retroarch tests/io tests/audio tests/testcard_pins.cmake exit 0"
        status: pass
    human_judgment: false
---

# Phase 9 Plan 10: Light Phaser in the libretro core Summary

The libretro core now plays Light Phaser titles: a LIGHTGUN controller type aims by the frontend pointer through the crop, a RetroPad, stick and mouse cursor always works, Pause and Reset survive a gun on port 0, and a crosshair is drawn into the presented picture only.

## Accomplishments

- `RETRO_DEVICE_LIGHTGUN` appended to the controller table as a base id; `cogmaster_port1` and `cogmaster_port2` gain `lphaser` in all three option tables; `cogmaster_crosshair` (default on) and `cogmaster_crosshair_colour` (default white) added; 11 options, 18 descriptors, 3 controller types per port.
- Gun source: `SCREEN_X`, `SCREEN_Y` and `IS_OFFSCREEN` through `cogmaster__aim_from_screen` with the last frame's rectangle (initially `0,0,crop_w,crop_h`), `TRIGGER` to `p` bit 4, `RELOAD` unmapped. Cursor source: a per-port cursor stepped by the existing `lr_axis` sum with `LR_STICK_PHASER 4`, trigger RetroPad B or A or mouse left, never off screen, reset at load, `retro_reset` and unload.
- Pause and Reset read the gun's Start and Select when port 0 carries a gun; a gun on a port whose option is not the phaser, and a Sports Pad or Paddle on a gun-device port, read nothing.
- The crosshair is written after the conversion loop into `pixels` through `LR_CONVERT`, at most 17 points per gun, only while a phaser port has a valid aim.
- `tests/libretro/gun.h`: a fake frontend, a Z80 guest assembled in the test and three cases that read the guest's report from a patch of the picture; the crosshair case compares the presented picture with a direct library run.

## Task Commits

1. Tasks 1 and 2 together (the type, options, aim sources, Pause and Reset, and the crosshair, with the descriptor and type counts of the host's controller checks): db8e137
2. Task 3, the gun cases, their registration and the inventory: 8c69daa

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Tasks 1 and 2 share one commit**
- **Found during:** Task 1 commit
- **Issue:** the crosshair was written with the aim sources, and the verify of Task 1 already needs Task 3's two edits to the host's controller and descriptor checks, so the first commit holds Task 1, Task 2 and that host.c edit; Task 2's own verify and greps were then run on it
- **Fix:** one commit with a message naming all of it; Task 3 is a separate commit
- **Files modified:** libretro/libretro.c, libretro/options.h, tests/libretro/host.c
- **Commit:** db8e137

**2. [Rule 1 - Bug] The crop-offset control cannot be a dropped offset**
- **Found during:** Task 3 (the negative control)
- **Issue:** the plan's control drops the crop offset in the screen mapping, but the Light Phaser exists on the Master System only (a Game Gear port takes none, `cogmaster__device_ok`) and the Master System's rectangle starts at 0,0, so the offset is invisible through any machine the gun can run on. The offset itself is proven by `host.aim` on the Game Gear rectangles (plan 09-09)
- **Fix:** the control maps the pointer through the 240-line field instead of the 192-line crop (the same fault the case names, a pointer mapped without the crop); it turned `gun.aim` and `gun.crosshair` red. `gun.aim` carries edge rows of the square so a wrong height is seen
- **Files modified:** none (a one-shot edit of libretro/libretro.c, reverted)
- **Commit:** n/a

**3. [Rule 1 - Bug] The draw control needs a cast**
- **Found during:** Task 3 (the negative control)
- **Issue:** drawing into `native` (a 16-bit buffer) does not compile under -Wconversion without a cast, so the first run of the control was a build failure, not a red test, and its result was discarded
- **Fix:** the control casts the converted value to `uint16_t`; the build succeeded and `gun.crosshair` failed
- **Files modified:** none (one-shot edit, reverted)
- **Commit:** n/a

**Total deviations:** 3 (one commit grouping, two controls adapted). No behaviour change from the plan.

## Phase notes

- The crosshair colours are in the native layout `0000BBBBGGGGRRRR` (white `0x0FFF`, red `0x000F`, green `0x00F0`, yellow `0x00FF`), not the R-high order of the discretion list; a control with red as `0x0F00` (which draws blue) turned `gun.crosshair` and `gun.cursor` red.
- The aim is mapped through the rectangle of the last completed frame; a change of picture size moves it one frame late. Before the first frame the rectangle is `0,0,crop_w,crop_h`, which is the Master System crop, the only machine that takes the gun.
- `libretro.host.gun.*` timeouts are a 60 s floor and provisional (each case ran in well under a second here); the closing plan reads a hosted run.

## Verification evidence

- `cmake --preset ci`, `cmake --build --preset ci`, `ctest --test-dir build/ci -R '^(libretro\.|host\.|abi\.)'`: exit 0 (62 tests before Task 3, 40 libretro tests after).
- `ctest --test-dir build/ci -R '^libretro\.host\.gun\.'`: exit 0 (3 tests); `ctest --test-dir build/ci -N -R '^libretro\.host\.gun\.'` lists the three names.
- `cmake --workflow --preset ci`: exit 0 (584 tests); `cmake --workflow --preset hygiene`: exit 0 (50 tests).
- Container compile: `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -DCOGMASTER_LIBRETRO_VERSION='"x"' [-DCOGMASTER_LIBRETRO_RGB565 | -DCOGMASTER_LIBRETRO_TEST_GG] -I src -I include -I build/ci/generated -I host -I libretro libretro/libretro.c`: exit 0 for all three; the same flags with `-I libretro -I host -I runner -I src -I include -I build/ci/generated -I tests tests/libretro/host.c`: exit 0 (after making the cross helper's array parameters non-const for -Wpedantic).
- Negative controls, each a python edit of libretro/libretro.c confirmed by `git diff --stat`, rebuilt, run, then `git checkout -- libretro/libretro.c` and rebuilt: draw into `native` (gun.crosshair exit 8, gun.cursor exit 8, gun.aim exit 0); the 240-line rectangle (gun.aim exit 8, gun.crosshair exit 8, gun.cursor exit 0); red as 0x0F00 (gun.crosshair exit 8, gun.cursor exit 8); no cursor reset on retro_reset (gun.cursor exit 8); Pause read only from the pad (gun.cursor exit 8); stick scale 5 (gun.cursor exit 8). After the last revert `ctest -R '^libretro\.host\.gun\.'` exit 0 (3 of 3).
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0 (735 check lines, no check code added).
- `git diff --quiet origin/main -- tests/libretro/controls.h tests/retroarch`: exit 0; `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake`: exit 0.
- `LC_ALL=C sort -c tests/inventory/ci.txt`: exit 0; `grep -c 'libretro.host.gun' tests/inventory/wasm32.txt` prints 0.
- No planning ids in added lines of libretro or tests/libretro: count 0. clang-format 21 `--dry-run -Werror` on libretro.c, options.h, host.c, gun.h: exit 0.
- Hosted lanes not dispatched by this plan (no push); the next dispatch carries the gun cases.

## Known Stubs

None.

## Threat Flags

None. The adapter reads only the frontend's input callbacks and draws into its own output buffer; no file, network or core state is added.

## Next Phase Readiness

Plan 09-11 and the closing plan can document the Light Phaser in docs/controllers.md (the option decides what is attached, the frontend's device decides only the aim source, the rectangle's one-frame lag, the crosshair marks the aim and is presentation only). The hosted run of the three gun cases settles their provisional timeouts.

## Self-Check: PASSED

- tests/libretro/gun.h exists; db8e137 and 8c69daa are ancestors of HEAD.
