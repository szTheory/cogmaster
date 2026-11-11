---
phase: 09-light-phaser
plan: "03"
subsystem: ports-gun-device
tags:
  - ports
  - state
  - gun
  - fuzz
status: complete

requires:
  - 09-02
provides:
  - the Light Phaser as a device on either port of any Master System profile, refused on the Game Gear
  - an absolute last-wins aim, canonicalised to 0 0 when it is not a field dot, that persists across frames
  - the 24-byte port block with six gun fields and their saved-state bounds and cross-checks
  - ports.gun.aim, ports.gun.pins and ports.gun.hostile_aim on the ci and wasm32 lanes
affects: [09-04, 09-05, 09-08]

actuals:
  tokens: 12500
  tasks: 3
  commits: 3

plan_head_before: 308a455e4795a363be537bc9b664776d10a6d17e
plan_head_after: def1c887a6720ceacb0c1dc1dc05d96e74b25af1
commits: 3

tech-stack:
  added: []
  patterns:
    - "A phaser's aim is a position held in pending_input and m->input; ports_frame consumes the axes of every other device and keeps the phaser's"
    - "The two independent state models carry their own written-again aim and gun-field rules and are not imported from src/state.c"

key-files:
  created: []
  modified:
    - src/internal.h
    - src/ports.c
    - src/io.c
    - src/state.c
    - tests/ports/test_state.c
    - tests/fuzz/fuzz_pad.c
    - tests/ports/test_gun.c
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "A power reset keeps the pending aim like a soft reset; only a load zeroes it, because cogmaster_reset never touches pending_input and the existing Sports Pad motion rule already says it is not touched"
  - "The gun fields are watched in tests/ports/test_state.c together with vdp_height, so the independent model can apply the scan-row bound without importing the loader"

requirements-completed: []

coverage:
  - id: C1
    description: "A phaser is accepted on either port of SMS1, SMS2, Japan and PAL profiles and refused whole with ERR_PROFILE_REJECTED on the Game Gear, and COGMASTER__DEVICE_LAST is the phaser in the same commit"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.pins$' exit 0 (four profiles and the three Game Gear refusals with pending_input byte-identical)"
      status: pass
    human_judgment: false
  - id: C2
    description: "The aim is absolute and last-wins, a pair that is not a field dot is stored as 0 0, it survives frames, a device change restarts it, a soft reset keeps it and a load parks it"
    requirement: GUN-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.aim$' exit 0; negative control: add_motion summing a phaser pair turns it red (exit 8)"
      status: pass
    human_judgment: false
  - id: C3
    description: "The phaser's pins are 0x2F | (~held & 0x10) for every held value and every direction write, and a phaser at rest reads as a pad with nothing pressed on $DC and $DD"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.pins$' exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "Hostile aims are bounded: ten axis values by both ports by four profiles by three calls a frame equal an independent canonicalisation, and 32768, -32768 and the int32 extremes are ERR_ARGUMENT with pending_input, the in-force record and the blocks byte-identical"
    requirement: GUN-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.hostile_aim$' exit 0; clamping 257 to 256 turns it red (exit 8); a macOS build of the case with -fsanitize=address,undefined exits 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "The saved state holds six bounded gun fields per port with the cross-checks (a) to (f), the active axis bounds are 0..256 and 0..240 with the device rule, and the two independent checkers agree with the loader on mutation, sweep and the outside rows"
    requirement: GUN-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^(ports\\.state\\.|fuzz\\.regress\\.|ports\\.devices\\.|core\\.state\\.)' exit 0 (26 of 26); restoring the ceiling to SPORTS_PAD turns ports.state.outside red (exit 8)"
      status: pass
    human_judgment: false
  - id: C6
    description: "Nothing else moved: the existing io, vdp, bus, audio, testcard pin and ports test sources equal origin/main, and both workflows and the budget check pass"
    requirement: GUN-01
    verification:
      kind: integration
      ref: "git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c exit 0; cmake --workflow --preset ci exit 0 (549 of 549); cmake --workflow --preset hygiene exit 0 (50 of 50); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 45min
completed: 2026-11-11
---

# Phase 9 Plan 3: The Light Phaser as a device Summary

**A Light Phaser attaches on either port of any Master System profile with an absolute, last-wins aim that is parked (0 0) whenever it is not a field dot, a persistent position, a trigger on TL and a 24-byte port block whose six gun fields the state visitor bounds; the two independent state models in `tests/ports/test_state.c` and `tests/fuzz/fuzz_pad.c` moved with the ceiling in the same commit as the core.**

## Performance

- **Duration:** about 45 min
- **Completed:** 2026-11-11
- **Tasks:** 3 (Tasks 1 and 2 are one commit by the plan's own rule)
- **Files:** 10

## Accomplishments

- `COGMASTER__DEVICE_LAST` is `COGMASTER_DEVICE_LIGHT_PHASER` and `cogmaster__device_ok` accepts the phaser on any machine except the Game Gear, so `set_input`, the saved-state bound and both models agree from one commit.
- `add_motion` stores a phaser pair as the call's `(x, y)` when `1 <= x <= 256` and `1 <= y <= 240` and `(0, 0)` otherwise (a state, never an error or a clamp), so a stored aim has `|dx|, |dy| <= 256` and nothing overflows later. `cogmaster__ports_frame` zeroes a port's axes only when its device is not the phaser, so the aim persists; a device change clears the whole 24-byte block with `memset`.
- `cogmaster__ports_pins` returns `0x2F | (~held & 0x10)` for the phaser; the hardware comment cites SMS Power and the peripherals note.
- `struct cogmaster__port` is 24 bytes (`g_ax`, `g_dot`, `g_ay`, `g_line`, `g_sense`, `g_scan`, no padding); `ports_io` visits `port_gun_ax`, `port_gun_dot`, `port_gun_ay`, `port_gun_line`, `port_gun_sense`, `port_gun_scan` with bounds 0..256, 0..256, 0..240, 0..239, 0..3, 0..1 and the cross-checks (a) to (f); the `active_*` bounds are 0..256 and 0..240 with the device rule. No new `input_` name.
- `tests/ports/test_state.c` (available, aim and gun-field model, `device 5` outside rows, plus five new outside rows) and `tests/fuzz/fuzz_pad.c` (range, device_table, attachable, active-pair rule, gun-field rule, pending-axis rule) follow the ceiling. `tests/fuzz/seeds.c` is byte-equal.
- Three new cases: `ports.gun.aim`, `ports.gun.pins`, `ports.gun.hostile_aim`.

## Task Commits

1. **Tasks 1 and 2: the device, the aim, the 24-byte block, the state bounds and the two checkers (one commit, so no commit leaves the ci workflow red)** - `f6e3726` (feat)
2. **Fixup: the pin comment split a section number across a line break and hygiene.no_float read it as a decimal** - `2e544d3` (fix)
3. **Task 3: ports.gun.aim, ports.gun.pins and ports.gun.hostile_aim, registered on ci and wasm32** - `def1c88` (test)

**Plan metadata:** the commit that carries this file.

## Measurements the plan asked for

- **Recorded fields.** `cogmaster__state_record` returns 170,899 fields on an SMS2 profile and 171,420 on the Japan profile (the Paddle's), up from 170,887 and 171,408 (the 12 new gun fields). The cap is 240,000, so the worst-case headroom is 68,580, far above 64: `MAX_FIELDS` is not raised in either `tests/ports/test_state.c` or `tests/core/test_state.c`, so the plan stays at ten files and was not split.
- **`ports.state.mutation`.** 4.99 s and 5.10 s before, 6.37 s and 6.29 s after, on the ci lane here (same machine, run alone, a detached worktree of the previous commit for the before reading). The increase is the 16 added bytes of watched fields times six values per profile. At the be32 factor seen in run 42973268415 (38.44 s for 4.7 s native) that is about 52 s, inside the 80 s timeout.
- **Sanitiser.** `cmake --workflow --preset asan` needs clang-21, which is not on this Mac. The four gun cases and `ports.state` were built with Apple clang `-fsanitize=address,undefined -fno-sanitize-recover=undefined` over `src/*.c` and the test source and the four gun cases exited 0; the hosted asan job of plan 04 is the Linux reading.

## Evidence

All from the repository root, no `cd`. No push and no dispatch, so no run URL.

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci` | 0, no warning |
| `ctest --test-dir build/ci -R '^(io\.\|bus\.\|vdp\.\|ports\.(latch\|pins\|input\|devices\|glasses\|gun)\|abi\.\|core\.)'` (159 tests) | 0 |
| `ctest --test-dir build/ci -R '^(ports\.state\.\|fuzz\.regress\.\|ports\.devices\.\|core\.state\.)'` (26 tests) | 0 |
| `ctest --test-dir build/ci -R '^ports\.gun\.'` (4 tests) | 0 |
| `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I src -I include -I build/ci/generated src/ports.c src/io.c src/state.c` | 0 |
| same container command with `-I tests` on `tests/ports/test_state.c tests/fuzz/fuzz_pad.c` and on `tests/ports/test_gun.c` | 0 |
| `clang-format --dry-run -Werror` on every changed C file | 0 |
| negative control, task 2: python edit of `COGMASTER__DEVICE_LAST` back to `COGMASTER_DEVICE_SPORTS_PAD` (`git diff --stat HEAD` showed the line), rebuild 0, `ctest -R '^ports\.state\.outside$'` | 8 |
| the same control reverted, rebuild, `ctest -R '^(ports\.state\.\|fuzz\.regress\.\|ports\.devices\.\|core\.state\.)'` | 0 |
| negative control, task 3: `add_motion` sums a phaser pair (`git diff --stat` 2 lines in src/io.c), rebuild 0, `ctest -R '^ports\.gun\.(aim\|hostile_aim)$'` | 8, both red |
| negative control, task 3: `add_motion` clamps 257 to 256 (3 lines), rebuild 0, same ctest | 8, only `ports.gun.hostile_aim` red |
| both controls reverted by restoring `src/io.c`, rebuild, `ctest -R '^ports\.gun\.'` | 0 |
| `cc -fsanitize=address,undefined` build of `src/*.c` and `tests/ports/test_gun.c`, the four cases run | 0 each |
| `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c` | 0 |
| `git diff --quiet origin/main -- tests/fuzz/seeds.c tests/fuzz/tests.cmake` | 0 |
| `cmake --workflow --preset ci` | 0 (549 of 549) |
| `cmake --workflow --preset hygiene` | 0 (50 of 50, after the fixup) |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (723 check lines / 8928 src lines; no counted check file edited) |

Acceptance counts: `COGMASTER__DEVICE_LAST COGMASTER_DEVICE_LIGHT_PHASER` 1; `sizeof(struct cogmaster__port) == 24u` 1; `port_gun_` in src/state.c 6; `"input_..."` names 8 in both this tree and `origin/main`; no planning ID in an added `src` line; `device 5 pending` 1 and `device 4 pending` 0; `LIGHT_PHASER` in fuzz_pad.c 7; `ctest -N -R '^ports\.state\.'` 5 and `core\.state\.` 5; `ports.gun.*` 4 in ctest, and 4 rows each in ci.txt and wasm32.txt.

## Decisions Made

See key-decisions.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] A power reset keeps the pending aim; the plan said it zeroes it**
- **Found during:** Task 3 (reading `cogmaster_reset` for the aim case, as Task 1's read_first asked)
- **Issue:** the plan and CONTEXT decision 1 say "load and power zero it (`instance.c:239`)". Line 239 is `cogmaster_load` only. `cogmaster_reset` for both kinds never touches `pending_input` or `m->input`, and `cogmaster__io_reset` states "the pending motion is not touched" for the device blocks. A soft reset keeps the aim as decided; a power reset keeps it the same way, and zeroing it would change the existing Sports Pad motion and button behaviour on a power reset, which the plan forbids.
- **Fix:** none in `src/`; `ports.gun.aim` asserts the soft-reset keep and the load park and does not pin the power case. The device blocks (and so any scan in progress) are zeroed by both resets as before. The docs plan should say "a load parks the aim", not "a power reset does".
- **Files modified:** none
- **Commit:** none

**2. [Rule 3 - Blocking] The hygiene no-float scan flagged the phaser pin comment**
- **Found during:** Task 3 verification (`cmake --workflow --preset hygiene` exit 8)
- **Issue:** a section number "3.3" that began a continuation line of a comment is read as a decimal literal; the notes warn only about the word `section`.
- **Fix:** reworded so `section 3.3` sits on one line.
- **Files modified:** src/ports.c
- **Commit:** `2e544d3`

**3. [Rule 3 - Blocking] The container compile needs the generated include directory**
- **Found during:** Task 1 acceptance
- **Issue:** the plan's `-I src -I include` fails on the generated version header.
- **Fix:** added `-I build/ci/generated`, as the phase notes say.
- **Files modified:** none
- **Commit:** none

### Scope additions inside the plan's files

- `tests/ports/test_state.c` gained five outside rows (a phaser in force on a Game Gear, a gun field on a pad, a phaser pending with an aim accepted, with half an aim refused, and off the field refused). They exercise the new rules without adding the `ports.state.gun` case, which stays with plan 05.
- `tests/fuzz/fuzz_pad.c` checks the six gun fields as well as the active and pending pair, the same independent rules the state model uses.
- `tests/ports/tests.cmake` sets the gun timeouts through a variable, the form the devices block already uses.

**Total deviations:** 3 auto-fixed (1 bug in the plan's premise, 2 blocking). **Impact:** none on behaviour; the power-reset finding is a wording fix for the docs plan.

## Known Stubs

None. The gun's scan fields exist and are bounded but nothing sets `g_scan`, `g_dot`, `g_line` or `g_sense` yet; that is the sensor of plan 04, and a zero block means parked.

## Threat Flags

None. Axes are range-checked by the unchanged `axis_ok` before the canonical form is taken, a stored aim has `|dx|` and `|dy|` of at most 256, and the new saved fields are bounded and cross-checked by the loader and by two independent models.

## Self-Check: PASSED

- `src/internal.h`, `src/ports.c`, `src/io.c`, `src/state.c`, `tests/ports/test_gun.c`, `tests/ports/test_state.c` and `tests/fuzz/fuzz_pad.c` exist with the described edits; commits `f6e3726`, `2e544d3` and `def1c88` are ancestors of HEAD.
- Every acceptance criterion of the three tasks was re-run and passes.
