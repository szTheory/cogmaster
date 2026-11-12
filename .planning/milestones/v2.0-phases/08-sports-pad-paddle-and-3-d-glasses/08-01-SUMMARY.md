---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "01"
subsystem: io-ports
tags:
  - ports
  - th-latch
  - refactor
  - differential-test
status: complete

requires: []
provides:
  - cogmaster__ports_pins in src/ports.c, the pure pin function with the pad and none arms that the later device arms extend
  - cogmaster__io_th_changed in src/io.c, the one function every $3F write goes through
  - tests/ports with a 65536 pair latch sweep per profile, a falling-edge control and a pin sweep, registered on the ci and wasm32 lanes
affects: [08-02, 08-03, 08-04]

actuals:
  tokens: 3200
  tasks: 2
  commits: 2

plan_head_before: b3e5da3efebbe8b778335e984bdd04bc470fbdf3
plan_head_after: ecea8efadded52d665ba4b047d5f5fdd97cb4087
commits: 2

tech-stack:
  added: []
  patterns:
    - "A refactor is pinned by a differential against a literal copy of the old expression held in the test, plus a control whose wrong oracle must disagree"

key-files:
  created:
    - src/ports.c
    - tests/ports/test_ports.c
    - tests/ports/tests.cmake
  modified:
    - src/io.c
    - src/internal.h
    - CMakeLists.txt
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "port_dc and port_dd take their six pin bits per port from cogmaster__ports_pins; the reset button bit and tr_pin mixing stay in io.c exactly as shipped"
  - "io_control reads the old control byte before the assignment and passes both bytes to cogmaster__io_th_changed, which computes th_pins for each, so the latch value is the same as the inline code gave"
  - "The new test lives in tests/ports so the tests/io, tests/audio and testcard_pins diff against origin/main stays empty"

requirements-completed: []

coverage:
  - id: C1
    description: "The six device-driven pins of both ports come from one pure function in src/ports.c, and $DC and $DD return the same bytes as the old inline formula for every p1 and p2 value, for a device or none on each port, on SMS1, SMS2 and Game Gear, export and Japan, with the reset button and the console TR output clear and set"
    requirement: PAD-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.pins$' exit 0"
      status: pass
    human_judgment: false
  - id: C2
    description: "Every $3F write goes through cogmaster__io_th_changed, and the H-counter latch fires on exactly the pairs the copied pre-refactor formula says it does, across all 65536 pairs on six profiles"
    requirement: PAD-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.latch$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "The latch sweep can fail: an oracle that latches on the falling edge disagrees with the core on some pair, and moving the core's condition to the falling edge makes ports.latch fail"
    requirement: PAD-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.latch_control$' exit 0; local negative control, ports.latch exit 8 then exit 0 after the revert"
      status: pass
    human_judgment: false
  - id: C4
    description: "No behaviour changes: every existing io, bus, audio, core, vdp, abi, fixture and runner test is green, and nothing under tests/io, tests/audio or tests/testcard_pins.cmake differs from origin/main"
    requirement: PAD-01
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci exit 0 (464 of 464); git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake exit 0"
      status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-10
---

# Phase 8 Plan 1: The port layer and the single TH write Summary

**The two controller ports' pad pins now come from one pure function in `src/ports.c` and every `$3F` write goes through `cogmaster__io_th_changed`, proven byte-equal to the old inline code by a 65536-pair latch sweep and a full pin sweep.**

## Performance

- **Duration:** about 25 min
- **Completed:** 2026-11-10
- **Tasks:** 2
- **Files:** 9 (3 new)

## Accomplishments

- `src/ports.c`: `cogmaster__ports_pins(m, port, tick)` returns a port's six device pins (`~p & 0x3F` for a pad, `0x3F` for none and for port 2 on a Game Gear). It is pure, has no function-pointer table and takes a `tick` the pad arm ignores so a later arm needs no signature change.
- `src/io.c`: `port_dc` and `port_dd` take their pin bits from the port layer, with port 2's pins landing on `$DC` b7-b6 and `$DD` b3-b0 as before; the reset bit and `tr_pin` stay put. The latch condition moved byte for byte into `cogmaster__io_th_changed`, declared in `src/internal.h`.
- `tests/ports`: `ports.latch` (all 256 x 256 pairs on SMS1, SMS2 and Game Gear, export and Japan, against a copy of the old expression, counting the pairs it ran and requiring that some but not all latch), `ports.latch_control` (a falling-edge oracle must disagree somewhere) and `ports.pins` (the old `$DC`, `$DD`, `th_pins` and `tr_pin` copied as the oracle; 2.4 million reads).

## Task Commits

1. **Task 1: One TH write and one pad read through src/ports.c** - `7cbecb5` (refactor)
2. **Task 2: The latch and pin differentials with negative controls** - `ecea8ef` (test)

**Plan metadata:** the commit that carries this file.

## Evidence

All from the repository root, no `cd`:

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(io\.\|bus\.\|audio\.\|core\.\|vdp\.\|abi\.(globals\|undefined\|symbols)\|fixtures\.\|runner\.)'` (after task 1) | 0 (243 of 243) |
| `cmake --build --preset ci && ctest --test-dir build/ci -R '^(ports\.\|io\.)'` (after task 2) | 0 (14 of 14) |
| Negative control: the condition in `cogmaster__io_th_changed` changed to the falling edge, rebuilt, `ctest --test-dir build/ci -R '^ports\.latch$'` | 8 (failed, as intended) |
| The same after `git checkout -- src/io.c`, rebuilt, `ctest --test-dir build/ci -R '^ports\.'` | 0 |
| `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 |
| `cmake --workflow --preset ci` | 0 (464 of 464) |
| `cmake --workflow --preset hygiene` | 0 |

No run URL: nothing was pushed or dispatched in this plan. The `ports.*` timeouts are provisional at 20 s (the three cases run in under 20 ms each locally).

## Decisions Made

See key-decisions. `th_pins` stays the readback and latch function; the plan's argument that evaluating the old control byte before or after the assignment is the same (it depends on the control byte and the region only) holds, and the sweep covers it.

## Deviations from Plan

None - plan executed exactly as written. One note: the negative-control edit was first attempted with a BSD `sed` pattern that did not apply, so the first rebuild tested nothing; it was redone with the edit applied (exit 8 above) and the tree restored with `git checkout -- src/io.c` before the commit.

## Known Stubs

None.

## Threat Flags

None. No network, auth, file or schema surface is touched.

## Self-Check: PASSED

- `src/ports.c`, `tests/ports/test_ports.c` and `tests/ports/tests.cmake` exist.
- Commits `7cbecb5` and `ecea8ef` are ancestors of HEAD; `git rev-list --count b3e5da3..HEAD` was 2 before this file.
- All acceptance criteria of both tasks were re-run and pass: the grep counts (`cogmaster__io_th_changed` 2 in `src/io.c` and 1 in `src/internal.h`, `cogmaster__ports_pins` 3 in `src/io.c`, 0 planning IDs in `src/ports.c`), 11 io tests present, 3 ports tests registered, 3 rows in each of `ci.txt` and `wasm32.txt`, and the fault comment present once.
