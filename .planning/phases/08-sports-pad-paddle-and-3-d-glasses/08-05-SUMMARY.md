---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "05"
subsystem: ports
tags:
  - sports-pad
  - paddle
  - set-input
  - state-bounds
  - replay
status: complete

requires:
  - 08-04
provides:
  - the Sports Pad read sequence on the TH wire with the idle-tick rule, and the stateless Paddle with its TR clock
  - cogmaster__device_ok availability by profile and a set_input that refuses an unavailable device with ERR_PROFILE_REJECTED before any commit
  - motion summed per axis within the call range, a device change restarting the port, accumulation into the device blocks at frame start
  - pending axes, active axes and both port blocks saved with bounds and five cross-checks
  - eleven ports.devices cases on the ci and wasm32 inventories
affects: [08-06, 08-07, 08-09, 08-12, 08-16, 08-17]

actuals:
  tokens: 17600
  tasks: 3
  commits: 4

plan_head_before: fe53cb29329dc1c63a9368a2229806761e981b3e
plan_head_after: 7936dbfa104b547d28c031a076edfb72b7788cbd
commits: 4

tech-stack:
  added: []
  patterns:
    - "A device sees the TH wire, not the echoed readback, so Japan and export give the same bytes; cogmaster__io_th_changed tells a port only when its own wire changed, so its edges alternate"
    - "Per-port device blocks are zeroed by io_reset (the stored edge too, because the tick count restarts at 0) and by a device change at the next frame; pending motion survives a soft reset and a load zeroes it"
    - "The pad is a pure function of the block, the held byte and the tick, so a read of the ports can never move a device"

key-files:
  created:
    - tests/ports/test_devices.c
  modified:
    - src/ports.c
    - src/io.c
    - src/frame.c
    - src/internal.h
    - src/state.c
    - src/state.h
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "Both MAME (sms_ctrl/paddle.cpp) and ares (ms/controller/paddle) put the high nibble on the paddle's TR high, so the nibble follows the device's own clock even when the console drives TR as an output and tr_pin replaces only the TR pin"
  - "The tick constants live only in src/internal.h (SPORTS_IDLE_TICKS, PADDLE_HALF_TICKS); src/ports.c, io.c, frame.c and the new test use the names, and the paddle clock uses a 64-bit divide by the constant exactly as fm_in does"
  - "The state cross-checks read the VALUE each sio call returns; the new fields are pending_x1 .. active_y2 and port_edge .. port_pos, none beginning input_, so the tests/io byte-mutation cases are untouched"
  - "The signed 8, 16 and 32 bit state helpers share one bias function at the field's width, so zero encodes as the top bit set at every width"

requirements-completed: []

coverage:
  - id: C1
    description: "A Sports Pad answers $F idle, then the high and low nibble of X and of Y with TL and TR as buttons 1 and 2 active low, latched at the first fall; a second fall inside the idle limit keeps the read and one at the limit starts over, written with the constant's name"
    requirement: PAD-02
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#sports_ticks and #sports_walk via ctest ports.devices.sports_ticks and ports.devices.sports_walk on SMS1 and SMS2, export and Japan, both ports, every held-byte pattern"
        status: pass
    human_judgment: false
  - id: C2
    description: "The pad byte is the negated host motion saturated at 127 either way with the remainder carried across reads and frames, and no more than 1023 per axis is kept"
    requirement: PAD-02
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#clamp via ctest ports.devices.clamp; expected bytes come from a running total of what was sent"
        status: pass
    human_judgment: false
  - id: C3
    description: "A Paddle position is clamp(position + x, 0, 255) with +x larger, y ignored, centre 128 after a load, the button on TL, and TR alternating each half period on both ports with no state, a console TR output replacing the pin"
    requirement: PAD-03
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#paddle via ctest ports.devices.paddle"
        status: pass
    human_judgment: false
  - id: C4
    description: "set_input sums motion per axis with a clamp to the call range (two calls equal one summed call, two 20000 calls give 32767, 100000 calls at the ends never leave the range and the result still loads), restarts a port's axes on a device change and drops axes a device does not take"
    requirement: PAD-03
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#pending and #device_switch via ctest ports.devices.pending and ports.devices.device_switch; the Apple clang address and undefined sanitiser build of all eleven cases exited 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The TH wire, not the echoed readback, drives the pad: a 400-write random $3F script gives the bytes and block of an independent reference model on SMS1 and SMS2 in both regions, with equal blocks across the four profiles; a Paddle on an export instance or a Game Gear and a Sports Pad on a Game Gear are refused whole with the instance byte-identical"
    requirement: PAD-01
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#profiles via ctest ports.devices.profiles"
        status: pass
    human_judgment: false
  - id: C6
    description: "A soft or power reset and a load zero the blocks and the stored edge with no underflow on the first sequence; a soft reset keeps pending motion and a load zeroes it"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#resets via ctest ports.devices.resets"
        status: pass
    human_judgment: false
  - id: C7
    description: "A million reads and unchanged $3F writes at one tick leave the block unchanged, and a million toggles of both TH lines at one tick finish inside the model's bounds with a state that saves and loads"
    requirement: PAD-02
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#hammer via ctest ports.devices.hammer"
        status: pass
    human_judgment: false
  - id: C8
    description: "The new fields are saved with their bounds and cross-checks: states at step 1 right after the first fall of a reset instance (edge 0), at step 2 with motion left over, a latched byte and pending motion, and a Paddle off centre load into a second instance and save byte-equal and then continue identically; state.record_sweep and the io input cases still pass"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#roundtrip via ctest ports.devices.roundtrip; ctest core.state.record_sweep records 170887 fields against 240000"
        status: pass
    human_judgment: false
  - id: C9
    description: "Replaying the recorded per-call deltas of a 120-frame run through a handler that reads the pad each frame gives the same frame hashes and state bytes, also after a save and restore at frame 60; the motion reaches the picture and the same total cut into other per-frame chunks gives a different hash"
    requirement: PAD-03
    verification:
      - kind: unit
        ref: "tests/ports/test_devices.c#replay via ctest ports.devices.replay; the run hash differs from the no-motion run and from the re-chunked run"
        status: pass
    human_judgment: false
  - id: C10
    description: "Nothing guest-visible for a pad-only run changed: every existing io, audio, bus, vdp, fixture and core test passes, tests/io, tests/audio and tests/testcard_pins.cmake are byte-equal to origin/main, and the counted check code is within budget"
    requirement: PAD-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (488 tests); git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
  - id: C11
    description: "The new core code compiles with general registers only (no floating point) and passes the nofp, wasm32, be32, asan and perf lanes"
    requirement: PAD-01
    verification:
      - kind: integration
        ref: "local stand-in: cc -mgeneral-regs-only over src/ports.c, io.c, state.c and frame.c exit 0; hosted lanes run with the next dispatch (the plan names none)"
        status: pending
    human_judgment: false

duration: 85min
completed: 2026-11-11
---

# Phase 8 Plan 5: The Sports Pad, the Paddle and their saved state Summary

**The Sports Pad and the Paddle now work through the internal and public API: a Sports Pad answers its four-nibble read from the TH wire with the idle-tick rule, a Paddle shows its position on a stateless TR clock, set_input sums motion and refuses unavailable devices, and the device blocks and pending axes are saved with bounds and cross-checks; eleven `ports.devices` cases prove it against a reference model, a round trip and a recorded-motion replay.**

## Performance

- **Duration:** about 85 min
- **Completed:** 2026-11-11
- **Tasks:** 3
- **Files:** 10 (1 new)

## Accomplishments

- `src/ports.c`: `cogmaster__ports_pins` gains a Paddle arm and a Sports Pad arm; `cogmaster__ports_th` steps the read sequence (a fall at step 2 less than the idle limit after the last rise goes to step 3, else it latches and shows the X high nibble; a rise moves 1 to 2 and 3 to 4 and stamps the edge); `cogmaster__ports_frame` zeroes a port whose pending device differs from the active one, adds the pending motion (Sports Pad: acc clamped to 1023; Paddle: position clamped to 0 to 255, stored XOR $80) and zeroes the axes of both input records; `cogmaster__device_ok` is the availability rule.
- `src/io.c`: `th_wire` (the level written, or high when the pin is an input) feeds `cogmaster__ports_th` once per port whose wire changed; the H-latch line is untouched. `cogmaster__io_reset` zeroes both blocks. `cogmaster_set_input` now bounds devices by `COGMASTER__DEVICE_LAST`, refuses an unavailable device with `ERR_PROFILE_REJECTED` before anything is stored, sums axes with a per-axis clamp, restarts a port's axes on a device change and drops axes a device does not take.
- `src/internal.h`: `struct cogmaster__port` (16 bytes, size and field-sum asserts), `SPORTS_IDLE_TICKS`, `PADDLE_HALF_TICKS` and their derivations in the one place the numbers live, `ports[2]` in the instance.
- `src/state.h`, `src/state.c`: `cogmaster__sio_i32`, `_i16`, `_i8`; the pending and active axes, the per-port edge, acc, val, step and pos, the device bound widened to `COGMASTER__DEVICE_LAST`, and the five cross-checks. No new field name begins `input_`.
- `tests/ports/test_devices.c`: eleven cases, listed in the coverage block. A reference model of the pad sequence is written apart from the core, a hand-assembled Z80 handler reads the pad each frame and writes the result to the backdrop colour so the replay's frame hashes depend on delivered motion.

## State record count and headroom

`ctest --test-dir build/ci -R '^core\.state\.record_sweep$' -V` prints `state fields: 170887 recorded`, against 170863 at the phase's start (the glasses pair added 2 and this plan 22). The test buffer holds 240000, so the headroom is 69113 and the margin above the plan's raise threshold (239936) is 69049; `tests/core/test_state.c` is not edited (`git diff --quiet fe53cb2 -- tests/core/test_state.c` exit 0).

## Negative controls

Each was run once locally with the edit confirmed by `git diff --stat` (1 file changed) before the run, then reverted with `git checkout -- <file>` and rebuilt.

| Control | Command | Exit | Result |
|---|---|---|---|
| idle comparison `<` changed to `<=` in src/ports.c | `ctest --test-dir build/ci -R '^ports\.devices\.sports_ticks$'` | 8 | sports_ticks fails |
| per-axis clamp of pending removed in src/io.c | `ctest --test-dir build/ci -R '^ports\.devices\.pending$'` | 8 | pending fails |
| ports_th driven by the echoed readback in src/io.c | `ctest --test-dir build/ci -R '^ports\.devices\.profiles$'` | 8 | profiles fails |
| all three reverted | `ctest --test-dir build/ci -R '^ports\.devices\.'` | 0 | 11 of 11 pass |

The first attempt at the third control did not compile (`th_wire` was left unused and the build runs with `-Werror`), so ctest ran the old binary and passed; that run was discarded and the edit redone with a `(void)th_wire(...)` so the build succeeded before the red result above was taken.

## Verification run

- `cmake --workflow --preset ci` exit 0 (488 tests passed, plus the 4 release tests); `cmake --workflow --preset hygiene` exit 0 (48 tests); `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8763 src lines); `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0.
- Task 1 regex `^(io\.|bus\.|audio\.|core\.|vdp\.|fixtures\.|ports\.|abi\.(globals|undefined|symbols))` registers 187 tests (floor 170) and `ctest ... -R '^(io\.|bus\.|audio\.|core\.|vdp\.|fixtures\.|ports\.|abi\.(globals|undefined|symbols)|eeprom\.state\.)'` passed 190 of 190.
- Acceptance greps: `#define (SPORTS_IDLE_TICKS|PADDLE_HALF_TICKS)` appears twice in src/internal.h; 1536 and 299 appear in none of src/ports.c, io.c, frame.c or the new test (and 598 not in the test); `cogmaster__ports_th` is in src/io.c and `cogmaster__ports_frame` once in src/frame.c; `cogmaster__sio_(i32|i16|i8)` 3 times in src/state.h; 15 field-name matches in src/state.c (floor 9) and 8 `"input_` names (unchanged); 11 `ports.devices.*` names in `ctest -N` and in each inventory; `Fault prevented: a device that desyncs` once in tests/ports/tests.cmake.
- nofp: `cmake --workflow --preset nofp` exits 1 on this Mac with `nofp: -mgeneral-regs-only needs GCC, not AppleClang`; as a stand-in `cc -mgeneral-regs-only -std=c11 -O2` over src/ports.c, io.c, state.c and frame.c exits 0 on arm64. The hosted nofp, wasm32, be32, asan and perf jobs are the lane evidence and run with the next dispatch; the plan names none and no push was made.
- Sanitisers: all eleven cases, the six `ports.input` cases and the six `ports.glasses` cases built with `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` over src/*.c exit 0.

## Task Commits

1. **Task 1: The device model, availability, accumulation and the frame-start step** - `09c9549` (feat)
2. **Task 2: Saved fields, bounds and cross-checks for the new state** - `ab660c0` (feat)
3. **Task 3: Closed-form device, accumulation, reset, round-trip and replay cases** - `e137799` (test)
4. **Hygiene fix, see deviations** - `7936dbf` (fix)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] A decimal number in a source comment broke the no_float hygiene check**
- **Found during:** final `cmake --workflow --preset hygiene`
- **Issue:** the header comment of src/ports.c cited "sections 3.4 to 3.6"; the check exempts `section N.N` but not the plural, so it reported one violating line.
- **Fix:** worded as "section 3.4 and the two after it" in src/ports.c and the test header.
- **Files modified:** src/ports.c, tests/ports/test_devices.c
- **Commit:** 7936dbf

### Notes that are not deviations

- The plan puts the implementation tasks ahead of the tests; the three negative controls show the new cases go red when the rule they guard is broken.
- The Paddle nibble order came from the cited sources as the plan directs (MAME `in_r`: TR high selects bits 4 to 7; ares `read`: the same), so no order swap is left for the fixture plan to discover.
- Test helpers call `cogmaster__bus_sync` after moving the tick, as the bus does before a port access, so the states the tests save are ones a run could have saved (a bare tick change would break the saved beam cross-check).
- The pad's saturation is plus or minus 127 on both sides (the pad byte is `-clamp(acc, -127, 127)`); the plan's "+127/-128" wording describes the byte range, which never holds -128, and the state bound refuses it.

**Total deviations:** 1 (a comment wording fix). **Impact:** none on behaviour.

## Known Stubs

None.

## Threat Flags

None. The change adds no endpoint, file access or trust boundary; set_input bounds every device and axis before use, and a loaded state is refused unless the device blocks and pending axes are ones the core could have produced.

## Issues Encountered

Local nofp (GCC only), libFuzzer and the Homebrew clang-21 asan preset cannot run on this Mac; the sanitiser build above and the general-registers compile are the local stand-ins, and the hosted lanes are the evidence.

## Next Phase Readiness

Ready for the plans that build on the device model. Nothing is pushed since `b05c7b4`; the next hosted dispatch should run on the branch head so nofp, wasm32, be32, asan and the perf gate see the pad arms, the frame-start step and the new state fields.

## Self-Check: PASSED

- `tests/ports/test_devices.c` exists; commits `09c9549`, `ab660c0`, `e137799` and `7936dbf` are ancestors of HEAD.
