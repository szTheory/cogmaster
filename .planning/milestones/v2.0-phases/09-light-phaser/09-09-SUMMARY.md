---
phase: 09-light-phaser
plan: "09"
subsystem: host-aim
tags:
  - host
  - gun
  - tests
status: complete

requires:
  - 09-08
provides:
  - host/aim.c and host/aim.h, a pure integer helper in the cogmaster-host library that maps a pointer through the crop rectangle to a 1-based field aim, steps a clamped cursor and lists up to 17 crosshair dots
  - host.aim, the exhaustive mapping, off-screen, step and crosshair cases with a crop-blind oracle control, registered in the ci and wasm32 inventories
affects: [09-10, 09-12]

actuals:
  tokens: 4500
  tasks: 2
  commits: 2

plan_head_before: 29caa4ed7261be29002116a9d1a11b5bcf346ecd
plan_head_after: f3653c6868445376bbaf23efbf9a59fb9baab18d
commits: 2

tech-stack:
  added: []
  patterns:
    - "A host helper with its own rectangle and point types so a host without the core can use it; cursors and aims are 1-based, crosshair points are 0-based field dots"
    - "An oracle written in the test from the definition in 64 bits, plus a deliberately crop-blind oracle that must disagree on the Game Gear rectangles"

key-files:
  created:
    - host/aim.c
    - host/aim.h
    - tests/host/test_aim.c
  modified:
    - CMakeLists.txt
    - tests/host/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The cursor is a 1-based aim, so the adapter passes it unchanged to the core and to cogmaster__aim_cross; first step from UINT32_MAX centres at x + w / 2 + 1 (the same dot a zero pointer maps to) and then applies the delta"
  - "A pointer outside -32767..32767, an empty rectangle or any field above 65535 parks (0 0) rather than wraps; a NULL rectangle parks, a NULL output pointer does nothing"
  - "cogmaster__aim_cross returns 0 points for an aim outside the rectangle, so a stale aim after a smaller picture draws nothing"

requirements-completed: []

coverage:
  - id: C1
    description: "A pointer maps through the picture rectangle to a 1-based field aim in 32-bit arithmetic (u = s + 32768, col = (u * w) >> 16, ax = x + col + 1): zero is column w / 2, -32767 is column 0, +32767 is column w - 1, the map is monotone and reaches every column, exhaustively over -32767..32767 for widths 160, 224, 240 and 256 and for all five Master System and Game Gear rectangles"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^host\\.aim$' exit 0; the test compares against a 64-bit oracle written in the test"
        status: pass
    human_judgment: false
  - id: C2
    description: "An off-screen flag, either coordinate -32768 (or any value outside the range) and an empty or missing rectangle give 0 0 and never an edge dot"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^host\\.aim$' exit 0 (the offscreen case runs on all five rectangles)"
        status: pass
    human_judgment: false
  - id: C3
    description: "The crop offset is proven to matter: an oracle that ignores x and y disagrees with the helper at the corners and centre on both Game Gear rectangles and agrees on the Master System ones, and dropping rect->x from the helper turned host.aim red (85195 mapping failures and ax 1 against 49 at a Game Gear corner)"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "python edit of host/aim.c call site (aim_axis(0u, ...)), confirmed by git diff --stat HEAD, rebuilt, ctest -R '^host\\.aim$' failed; git checkout -- host/aim.c, rebuilt, exit 0. A first control that dropped the parameter use failed to compile (unused parameter, -Werror), so its green was discarded and the call-site control used instead"
        status: pass
    human_judgment: false
  - id: C4
    description: "The cursor step clamps any int32 delta into the rectangle with no overflow (checked for eleven deltas including INT32_MIN and INT32_MAX from three starting cursors per axis on all five rectangles), centres when unset and pulls a stale cursor back inside; the crosshair writes at most 17 dots, all inside, exactly the shape's count at every edge, corner and centre, nothing past the count (0xA5 poison), and nothing for 0 0, an aim outside the rectangle or a NULL buffer"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^host\\.aim$' exit 0; host.aim also built with Apple clang -fsanitize=address,undefined,unsigned-integer-overflow,implicit-conversion and run, exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The helper is independent of the core header, builds without warnings under gcc 13 with -Wpedantic -Wconversion -Wtype-limits, is in the cogmaster-host library, and every other ci and hygiene test is unchanged"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "docker run gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I host host/aim.c tests/host/test_aim.c exit 0; cmake --workflow --preset ci exit 0 (581 tests); cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (735 check lines, limit unchanged by this plan)"
        status: pass
    human_judgment: false
---

# Phase 9 Plan 9: Light Phaser host aim helper Summary

A pure host helper, `host/aim.{c,h}` in `cogmaster-host`, maps a libretro pointer through the last frame's rectangle to a 1-based field aim in 32-bit integer arithmetic, steps a clamped adapter cursor and lists the crosshair's dots; `host.aim` proves it exhaustively against independent oracles, with a crop-blind control that must fail.

## Accomplishments

- `cogmaster__aim_from_screen`: `ax = x + ((s + 32768) * w >> 16) + 1`; off screen (flag, -32768, out of range, empty or missing rectangle) is `0 0`.
- `cogmaster__aim_step`: a 1-based cursor, centred when unset, deltas clamped to the rectangle size before the addition.
- `cogmaster__aim_cross`: up to 17 0-based field dots inside the rectangle; the adapter subtracts `x` and `y` to index its buffer.
- `host.aim` (label `host portable`, timeout 10 s floor, provisional) on the ci and wasm32 inventories beside `host.eyes`.

## Task Commits

1. Task 1, the three helpers and the target change: be37df3
2. Task 2, `host.aim`, the registration and the header's NULL-rectangle sentence: f3653c6

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] NULL rectangle behaviour stated in the header**
- **Found during:** Task 2 (the NULL case of the test)
- **Issue:** the header said "A NULL pointer does nothing", but a NULL rectangle with valid outputs parks (writes `0 0`), which is the safer state for an adapter
- **Fix:** the header now says a NULL rectangle parks and a NULL output pointer does nothing; the test asserts both
- **Files modified:** host/aim.h, tests/host/test_aim.c
- **Commit:** f3653c6

**Total deviations:** 1 auto-fixed (documentation of an edge case, no behaviour change).

The first negative-control edit (removing the use of `origin`) did not compile, so its red-looking/green-looking result was discarded and the control repeated at the call site; recorded under C3.

## Verification evidence

- `cmake --preset ci`, `cmake --build --preset ci`, `ctest --test-dir build/ci -R '^(host\.|abi\.(globals|undefined|symbols)|libretro\.)'`: exit 0 (44 tests).
- `ctest --test-dir build/ci -R '^host\.'`: exit 0 (host.aim, host.eyes, host.palette).
- `cmake --workflow --preset ci`: exit 0 (581 tests); `cmake --workflow --preset hygiene`: exit 0 (50 tests).
- Container compile: `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I host host/aim.c tests/host/test_aim.c`: exit 0.
- Sanitised run (local Apple clang, not the hosted asan job): `cc -std=c17 -fsanitize=address,undefined -fsanitize=unsigned-integer-overflow,implicit-conversion -fno-sanitize-recover=undefined,integer -I host host/aim.c tests/host/test_aim.c`, then the binary with `aim`: exit 0. The hosted asan lane of the next dispatch is the lane evidence.
- `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake`: exit 0.
- `LC_ALL=C sort -c` on tests/inventory/ci.txt and wasm32.txt: exit 0.
- No planning IDs in added lines of host, CMakeLists.txt or tests/host: count 0.

## Known Stubs

None.

## Threat Flags

None. The helper touches no file, network or core state.

## Next Phase Readiness

Plan 09-10 (the libretro adapter) can include `aim.h`; it keeps `x y w h` from the last frame, passes a zero-initialised cursor set to `COGMASTER_AIM_CURSOR_UNSET`, and draws the points from `cogmaster__aim_cross` into its output buffer only. The `host.aim` timeout is provisional until the closing plan reads a hosted run.

## Self-Check: PASSED

- host/aim.c, host/aim.h, tests/host/test_aim.c exist; be37df3 and f3653c6 are ancestors of HEAD.
