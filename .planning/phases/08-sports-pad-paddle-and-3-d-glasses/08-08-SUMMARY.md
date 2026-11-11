---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "08"
subsystem: runner
tags:
  - runner
  - controllers
  - exit-77
  - probe
status: complete

requires:
  - 08-07
provides:
  - "--port1 and --port2 take auto, pad, none, paddle or sports, with auto the default"
  - "one cogmaster_set_input after load and before frame 1 that exits 77 naming the port, the device and the fix when the profile rejects a device"
  - "the execution line prints device names (port1=sports) after the unchanged mapper=N port1= prefix"
  - "--probe appends port1_default and port2_default after unsupported_reason"
  - "the exit 77 row widened to name a controller in exit_status.h and docs/runner.md"
affects: [08-09, 08-13, 08-17]

actuals:
  tokens: 2400
  tasks: 2
  commits: 2

plan_head_before: 390423374c3d3a028da14f2c017ee16d56b58f29
plan_head_after: 70f5b59b4e42c8d47d4c7f5fba8cbf36d9e656d3
commits: 2

tech-stack:
  added: []
  patterns:
    - "Options record whether a port was given (port1_given, port2_given) next to the device value, so auto is the zero value and no sentinel can collide with a device constant"
    - "A refused set_input changes nothing, so the refusal asks about port 1 alone to name the offending port without parsing a status"

key-files:
  created: []
  modified:
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - runner/probe.c
    - runner/exit_status.h
    - tests/runner/tests.cmake
    - docs/runner.md
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "auto resolves from cogmaster_info after load, which already degrades a table default the profile cannot take to a pad, so a hash default never refuses; only an explicit choice can exit 77"
  - "The probe prints the raw table intent from cogmaster_cart_info (pad for any code but paddle and sports), the same words the option takes, so a script can pass a default back to --port1"
  - "The device words live in one table in runner/args.c that serves both the parser and the printed names, exposed as cogmaster__device_name"

requirements-completed: []

coverage:
  - id: R1
    description: "--port1 and --port2 accept auto, pad, none, paddle and sports; any other word, including AUTO and Paddle, is a usage error with exit 2, and an option with no value stays a usage error"
    requirement: PAD-06
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#device_usage via ctest runner.device.usage"
        status: pass
    human_judgment: false
  - id: R2
    description: "A run with no port option, or auto spelled out, on a pad-only image prints port1=pad port2=pad, a chosen device is printed by name, the mapper=N port1= prefix is unchanged, and the probe of a pad-only image prints port1_default=pad and port2_default=pad after unsupported_reason"
    requirement: PAD-06
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#device_names and device_auto via ctest runner.device.names and runner.device.auto"
        status: pass
    human_judgment: false
  - id: R3
    description: "An explicit Paddle without --region ntsc-j, or a Paddle or Sports Pad on the Game Gear, exits 77 with a message naming the port, the device and the fix; the same choices on a profile that takes them exit 0; a bad second port is named as port 2"
    requirement: PAD-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#device_refused via ctest runner.device.refused"
        status: pass
    human_judgment: false
  - id: R4
    description: "The widened 77 row is the same text in exit_status.h and docs/runner.md, the probe shape case carries the two new keys, and every earlier runner, fixture and library case passes with the same hashes"
    requirement: PAD-06
    verification:
      - kind: integration
        ref: "ctest -R runner.exit.table and ctest -R '^(runner|fixtures|library)[.]' (122 of 122); cmake --workflow --preset ci exit 0 (502 tests)"
        status: pass
    human_judgment: false

duration: 40min
completed: 2026-11-11
---

# Phase 8 Plan 8: Runner device grammar, refusal and default keys Summary

**The runner takes auto, pad, none, paddle or sports for each port, resolves auto from the library's per-dump default, makes one set_input call after load that exits 77 with the fix when an explicit choice is impossible (naming the port and `--region ntsc-j` for a Paddle), prints device names in the execution line and appends `port1_default` and `port2_default` to `--probe`.**

## Performance

- **Duration:** about 40 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 9 (none new)

## Accomplishments

- `runner/args.c`, `runner/args.h`: one small table of device words drives the parser and `cogmaster__device_name`; `port1_given` and `port2_given` are 0 for auto, which keeps every existing option struct valid (zero-initialised) and every existing usage message and test case as it was. The help text names the five words.
- `runner/main.c`: after a successful load the runner reads `cogmaster_get_info`, takes an explicit option as given and `default_port1/2` otherwise, and makes the single refusing call before frame 1. A profile-rejected status prints one line on standard error and exits 77; any other non-OK status exits 1. The per-frame loop keeps passing the resolved devices, so a frame can never change them. The execution line prints names through the table.
- `runner/probe.c`: two keys appended after `unsupported_reason`, from the raw table intent; no other key moved.
- `runner/exit_status.h` and `docs/runner.md`: the 77 row says "the cartridge, its size, the machine and region, or a controller is not supported, so nothing ran" in both; the doc also lists the refusal, the option words, the `--region ntsc-j` rule for a Paddle and the two probe keys (docs/runner.md grew by about 0.3 KB, well inside its room; `docs.budgets` passes).
- Four registered cases, `runner.device.usage`, `names`, `refused` and `auto`, on the ci and wasm32 inventories with `portable,runner`, under one "Fault prevented" comment. They assert on exit status and tolerant regexes.

## Negative control

The edit made `set_devices` return success for a profile-rejected status. It was applied with a script, `git diff --stat runner/main.c` showed it changed, and the build ran before the test.

| Control | Command | Exit | Result |
|---|---|---|---|
| rejected status ignored | `ctest --test-dir build/ci -R '^runner\.device\.'` | 8 | `runner.device.refused` fails; the other three pass |
| reverted | `ctest --test-dir build/ci -R '^runner\.device\.'` | 0 | 4 of 4 pass |

## Verification run

- Task 1 verify: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(runner\.|fixtures\.|library\.)'` exit 0, 118 of 118 pass before the new cases and `-N` shows 122 after them; `grep -c port1_default runner/probe.c` is at least 1; `grep -c 'a controller is not supported' runner/exit_status.h` prints 1; `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0; the probe form line carries `port1_default=(pad|paddle|sports)`.
- Task 2 verify: `ctest --test-dir build/ci -R '^(runner\.|docs\.)'` exit 0 (87 of 87 with the four new cases); `-N -R '^runner\.device\.'` lists the four names; four matching lines in each of tests/inventory/ci.txt and wasm32.txt; `grep -c 'a controller is not supported' docs/runner.md` prints 1; no line was removed from tests/runner/tests.cmake against origin/main; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8793 src lines).
- Plan: `cmake --workflow --preset ci` exit 0 (502 tests plus 4 release tests); `cmake --workflow --preset hygiene` exit 0 (48 tests); clang-format 21 dry run on runner/*.c and runner/*.h exit 0.
- Nothing was pushed and no hosted run was dispatched; the plan names none. Run URLs: none for this plan.

## Task Commits

1. **Task 1: Option grammar, resolved devices, exit 77 and the probe keys** - `d98815e` (feat)
2. **Task 2: Runner device cases and the docs** - `70f5b59` (test; also carries the refusal-naming fix to `runner/main.c` found by the new cases)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The refusal named the wrong port and the wrong fix in two cases**
- **Found during:** Task 2, writing `runner.device.refused`
- **Issue:** the first form of the message chose the offending port by looking at whether port 1 held a Paddle or Sports Pad, so `--port1 sports --port2 paddle` blamed port 1; and a Paddle on the Game Gear said only to pass `--region ntsc-j`, which cannot help there.
- **Fix:** a refused call changes nothing, so the runner asks about port 1 alone with a pad on port 2 to find the culprit; the Paddle line says both the region and the Game Gear facts. A case pins `paddle on port 2`.
- **Files modified:** runner/main.c, tests/runner/tests.cmake
- **Committed in:** 70f5b59

### Notes that are not deviations

- The usage-error cases `--port1 other` and `--port2` with no value and the `--port1 none --port2 none --hash` case are unchanged. The plan's check that at most three old lines of tests/runner/tests.cmake change holds with none removed.
- The table-default cases that need an image with a table row wait for the owned fixture rows (a later plan), as the plan says; the resolution code path is covered here for the pad-only and no-content cases and by the library cases from the previous plan.
- The execution line is printed only with `--expect`, so the names case drives it through a one-line expectation file.

**Total deviations:** 1 auto-fixed. **Impact:** none on scope.

## Known Stubs

None.

## Threat Flags

None. The change reads an option word and a library result and adds no endpoint, file access or trust boundary.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-09. The branch has the 08-05 to 08-08 commits locally (pushed through `b05c7b4`).

## Self-Check: PASSED

- All nine modified files exist; commits `d98815e` and `70f5b59` are ancestors of HEAD.
