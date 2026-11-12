---
phase: 09-light-phaser
plan: "06"
subsystem: runner-gun
tags:
  - runner
  - gun
  - tests
  - docs
status: complete

requires:
  - 09-03
  - 09-05
provides:
  - lphaser as a runner device word on either port, in the usage text, the help text and the probe defaults
  - a per-device aim check in the input validator, exit 2 before frame 1 with a message that names the port and the device
  - runner.gun.args and runner.gun.gg_refused, each with a control
affects: [09-07, 09-12]

actuals:
  tokens: 4764
  tasks: 2
  commits: 2

plan_head_before: afa1f39db1b5384941f36c74ce0c4bc3f693d299
plan_head_after: e846f67e1b6c2e53e3479bb7c79688dd5cba620c
commits: 2

tech-stack:
  added: []
  patterns:
    - "A per-device motion rule in one function (input_motion_ok): the phaser's pair rule sits beside the pad, paddle and sports rules and the validator keeps one bad_port output, so main.c only chooses the sentence"

key-files:
  created: []
  modified:
    - runner/args.c
    - runner/args.h
    - runner/input.c
    - runner/input.h
    - runner/main.c
    - runner/probe.c
    - docs/runner.md
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The two new cases need no fixture image (the test card runs on both machines), so they register beside runner.device.{usage,names,refused,auto} outside the fixtures guard"
  - "The usage lines list the words with commas (sports, lphaser) so the line stays inside the 100-column limit; the argument error text keeps the prose form"
  - "input.h's comment on the validator is widened to say a phaser's out-of-field aim also returns 2 with bad_port set, the only header change"

requirements-completed: []

coverage:
  - id: C1
    description: "cogmaster-run names lphaser in its device table, usage, help and probe defaults; lphaser is accepted on either port and phaser, Lphaser and lightgun stay exit 2 usage"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.device\\.usage$' exit 0 and ctest --test-dir build/ci -R '^runner\\.probe' exit 0 (the probe form accepts lphaser in both default alternations)"
        status: pass
    human_judgment: false
  - id: C2
    description: "A phaser's script aim is strict: 0 0, the 35-byte form or x 1 to 256 with y 1 to 240 runs, and a negative axis, x above 256, y above 240 or exactly one of the pair zero is exit 2 before frame 1 naming the port and lphaser, on both ports and on the last record; the same valid aim on a pad port is exit 2 takes no motion"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.gun\\.args$' exit 0; dropping the y above 240 refusal in runner/input.c turned it red (build exit 0, ctest exit 8) and restoring it turned it green"
        status: pass
    human_judgment: false
  - id: C3
    description: "A phaser on a Game Gear is exit 77 with the shipped message shape and no region hint, for either port, with a pad on the Game Gear as the exit 0 control"
    requirement: GUN-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.gun\\.gg_refused$' exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "Nothing else moved: both workflows pass, the counted check code is unchanged at 723, the exit-status text and the existing pins are untouched, and the runner sources compile clean under GCC 13 with the strict flags"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; git diff --quiet origin/main -- runner/exit_status.h tests/io tests/audio tests/testcard_pins.cmake exit 0; docker run gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only over the four runner sources exit 0"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-11
---

# Phase 9 Plan 6: The runner speaks the Light Phaser Summary

**`cogmaster-run` takes `--port1 lphaser` and `--port2 lphaser`, reads the gun's aim from the existing `x1 y1` or `x2 y2` script columns with a strict per-device check (0 0 or x 1 to 256 and y 1 to 240, else exit 2 before frame 1 naming the port), takes the trigger from bit 4 of `p1` or `p2`, prints `lphaser` in `--probe`, and refuses the gun on a Game Gear with the shipped exit 77 message; it draws no crosshair and calls no mapping.**

## Performance

- **Duration:** about 25 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 10

## Accomplishments

- `runner/args.c` gains `{"lphaser", COGMASTER_DEVICE_LIGHT_PHASER}` in `devices[]`, so `cogmaster__device_name` and `--probe` print the word, and the two usage lines and the argument error list it.
- `input_motion_ok` in `runner/input.c` carries the phaser's rule: `0 0`, or x in 1..256 and y in 1..240. The core would park a pair with one zero; the runner refuses it so a typo is loud. A pad and an empty port keep taking no motion and the sports pad and paddle keep their rules. `cogmaster__input_validate` is otherwise unchanged and still reports the port in `bad_port`.
- `runner/main.c` chooses the sentence: for a phaser `input file error: port N is lphaser, which takes an aim of x 1 to 256 and y 1 to 240, or 0 0`, for the other devices the existing `which takes no motion`. `set_devices` needed no edit: the Game Gear refusal already read `unsupported: lphaser on port 1 is not available here; the Game Gear has no such controller` with no region hint, because only the paddle gets one.
- `runner/probe.c`: `default_name` passes a phaser through `cogmaster__device_name`.
- `docs/runner.md` (10 added lines, well under the 20 allowed): the device word in the option row, the script paragraph gains the aim rule, the trigger bit, the stricter checks and the absence of a crosshair or mapping, and the exit 77 and probe rows name the phaser.
- `runner.gun.args` covers six strict refusals on each port, a refusal on the last record, the accepted aims (1,1), (256,240) and (0,0) on each port, the 35-byte line with a held trigger, and the pad control. `runner.gun.gg_refused` covers both ports on a Game Gear and the pad control. The probe-form regex and the usage case follow the new word set.

## Task Commits

1. **Task 1: the device word, the aim check, the probe word and the docs** - `64c8928` (feat)
2. **Task 2: runner.gun.args and runner.gun.gg_refused, and the two edited cases** - `e846f67` (test)

**Plan metadata:** the commit that carries this file.

## Files Created/Modified

- `runner/args.c`, `runner/args.h` - the device table row, the usage and error text
- `runner/input.c`, `runner/input.h` - the per-device aim rule and the validator's comment
- `runner/main.c` - the aim refusal sentence for a phaser
- `runner/probe.c` - the probe default name for a phaser
- `docs/runner.md` - option row, script paragraph, exit 77 and probe rows
- `tests/runner/tests.cmake` - the two cases, the probe form and the usage case
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the two names, sorted

## Decisions Made

- The new cases need no fixture, so they register unguarded next to the other device cases, under the plan's comment and a provisional 30 s timeout (plan 07 settles it from its hosted dispatch).
- The two usage lines use `sports, lphaser` with a comma to keep inside 100 columns; the exit 2 error text keeps `sports or lphaser`.

## Evidence

All run from the repository root.

- `cmake --preset ci` then `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -N -R '^runner\.gun\.'` lists exactly `runner.gun.args` and `runner.gun.gg_refused`.
- `ctest --test-dir build/ci -R '^(runner\.|docs\.)' --output-on-failure` exit 0 (97 of 97).
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0 (50 of 50, first try); `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines / 9221 src lines).
- Negative control: a python edit that dropped `y <= 240` in `runner/input.c` (build exit 0, so the new binary ran) made `ctest -R '^runner\.gun\.args'` fail; restoring it and rebuilding made it pass.
- `clang-format --dry-run -Werror` over the six runner sources exit 0.
- `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -D_POSIX_C_SOURCE=200809L -I src -I include -I runner -I host -I build/ci/generated runner/args.c runner/input.c runner/main.c runner/probe.c` exit 0.
- `git diff --quiet origin/main -- runner/exit_status.h` exit 0; the same for `tests/io tests/audio tests/testcard_pins.cmake`; `grep -c` for planning IDs over the added runner, docs and test lines prints 0.
- No hosted dispatch in this plan (plan 07 dispatches the runner cases).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] A header comment and two usage lines were over the 100-column limit**
- **Found during:** Task 1 (`clang-format --dry-run -Werror`)
- **Issue:** the longer device list pushed the two usage strings and a new comment line in `runner/input.h` past 100 columns.
- **Fix:** commas in the usage strings (`paddle, sports, lphaser`) and a re-wrapped comment.
- **Files modified:** `runner/args.c`, `runner/input.h`
- **Verification:** `clang-format --dry-run -Werror` exit 0
- **Committed in:** `64c8928`

**2. [Rule 3 - Blocking] The container compile needed the generated version header on the include path**
- **Found during:** Task 1 acceptance (`docker run gcc:13 ... -I src -I include -I runner -I host`)
- **Issue:** `include/cogmaster.h` includes `cogmaster_version.h`, which only the build tree has.
- **Fix:** added `-I build/ci/generated`, as the phase notes prescribe for this check.
- **Verification:** exit 0

**3. [Plan wording] Task 1's verify command and the two tests that name the old word set**
- The probe form and the usage case are edited in Task 2, so Task 1's commit alone leaves them red; both tasks were verified together before either was committed, as the acceptance criteria allow.

---

**Total deviations:** 2 auto-fixed (blocking), 1 plan wording note
**Impact on plan:** none; no scope change.

## Known Stubs

None.

## Issues Encountered

None.

## Threat Flags

None. The only new input surface is a bounded integer pair already parsed by the existing fixed-width reader.

## Next Phase Readiness

Plan 07 can dispatch the runner cases on the hosted lanes and settle the provisional `TIMEOUT 30` of `runner.gun.args` and `runner.gun.gg_refused`. The runner now accepts a script aim for the gun fixture work in later plans.

## Self-Check: PASSED

- `runner/input.c` contains `COGMASTER_DEVICE_LIGHT_PHASER`; `runner/args.c` holds `lphaser` four times.
- Commits `64c8928` and `e846f67` are ancestors of HEAD.
- `ctest -N -R '^runner\.gun\.'` lists both cases; both names are in `tests/inventory/ci.txt` and `wasm32.txt` with labels `portable,runner`.
