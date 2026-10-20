---
phase: 02-the-cpu-matches-the-public-vectors
plan: 06
subsystem: bus
status: complete
tags:
  - bus
  - loader
  - run-loop
  - state
  - api-v2
requires:
  - "02-05: the ED page with its ed_undefined seam, interrupt acceptance that waits for a pending prefix"
provides:
  - "src/bus.c: the library's seam definitions, incremental line/tick counters, the frame origin (BUS_ORIGIN_LINE 192, BUS_ORIGIN_TICK 607), the frame deadline and the no-mapper decode"
  - "src/cart.c: the 1 KiB to 1 MiB + 512 size rule, copier-header strip, $FF padding to 16 KiB pages, CRC-32 of the body"
  - "src/boot.c: the fast-boot hand-off (SP $DFF0, AF $FFFF, $C000 = $AB on the Master System, PC 0)"
  - "COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE (7), COGMASTER_API_VERSION 2, COGMASTER_STATE_FORMAT_VERSION 2"
  - "Fault kind and device enums in src/internal.h; the first ed_undefined fault is recorded"
  - "cogmaster-test-bus with bus.halt_bound.di, bus.frame_origin, fault.ed_undefined; core.load.sizes, core.load.rejected_unchanged, core.boot.handoff; abi.api_v2"
affects:
  - "Plan 07 (region, TV, PAL): the frame length and line count are NTSC constants in bus.c and state.c"
  - "Plan 08 (VDP timing): adds events to cogmaster__bus_sync and drives z80.int_line / nmi_pending; the interrupt tick stays separate from BUS_ORIGIN_*"
  - "Plan 09 (mappers): the no-mapper mem_read becomes a switch on the board"
  - "Plan 10 (faults): the other kinds and devices are already in the enums"
actuals:
  tokens: 20700
  tasks: 3
  commits: 4
plan_head_before: 5eb0916d69af517d8e2974ddef888a7bb1884453
plan_head_after: f1835ae40c8818cdc8afb58fef9ad1d5bd8a63e5
tech-stack:
  added: []
  patterns:
    - "The bus keeps line and t with the tick they describe (bus.at) and catches up by subtraction before each access; no 64-bit division or modulo"
    - "run_frame steps the Z80 while ticks < deadline, then sets deadline += one frame, so overshoot is charged to the next frame"
    - "load validates size, then machine, then allocates the new image before freeing the old one, so every refusal changes nothing"
key-files:
  created:
    - src/bus.c
    - src/cart.c
    - src/boot.c
    - tests/api/v2.c
    - tests/bus/tests.cmake
    - tests/bus/test_bus.c
  modified:
    - CMakeLists.txt
    - include/cogmaster.h
    - src/internal.h
    - src/instance.c
    - src/frame.c
    - src/state.c
    - tests/api/v1.c
    - tests/abi/tests.cmake
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/core/tests.cmake
    - tests/header/header.c
    - tests/header/header.cpp
    - tests/install/tests.cmake
    - tests/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/nofp.txt
    - docs/embedding.md
    - docs/testing.md
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/SMS-SYSTEM.md
decisions:
  - "The beam starts at the frame origin (line 192, tick 607, ticks 0) after load and both resets, so every NTSC frame is exactly 179,208 ticks; the origin is two private constants in bus.c, separate from any interrupt tick"
  - "The bus stores the tick its line/t describe (bus.at) in the state, so a state taken between bus calls resumes exactly; VALIDATE requires at <= ticks <= at + 69"
  - "A state's deadline must equal (frame + 1) x 179,208 and ticks must lie in [frame x 179,208, deadline); the test card keeps ticks == frame x 179,208"
  - "An ed_undefined fault reports pc at the $ED byte, addr at the second byte, width 8 (bits), dot = t / 2"
  - "A non-NULL image of size 0 is ERR_UNSUPPORTED_SIZE; a NULL image with a size is ERR_ARGUMENT"
  - "The Game Gear hand-off leaves $C000 alone (SMS-SYSTEM C7: unverified whether its BIOS writes it)"
  - "API version checks in v1.c, the header tests and the consumer tests accept any version from 1 up, so later bumps do not break frozen programs"
  - "requirements-completed stays empty: CPU-02 also needs the EI; HALT case, PAL frame lengths and the hostile VDP-port loop (plans 07 and 08)"
requirements-completed: []
coverage:
  - id: D1
    description: "DI; HALT runs ten frames, each stops FRAME with overshoot at most 69 ticks, total ticks are 10 x 179,208 plus the last overshoot, the CPU stays halted and the beam is back at line 192"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.halt_bound\\.di$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "After load, a soft reset and a power reset the beam is at line 192, tick 607 with ticks 0, and the next frame is exactly one frame long"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.frame_origin$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "Images of 1,023 and 1 MiB + 513 bytes are refused with ERR_UNSUPPORTED_SIZE; 1 KiB, 16 KiB + 512, 48 KiB, 20 KiB, 1 MiB and 1 MiB + 512 load with the copier header stripped and $FF padding to a page"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.load\\.sizes$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "200 seeded loads around both size bounds with any machine value: every refusal returns the rule's status, allocates nothing and leaves the instance and its image byte-identical"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.load\\.rejected_unchanged$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "Fast boot leaves SP $DFF0, $C000 = $AB, PC 0 and no other RAM written"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.boot\\.handoff$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "ED 00 records the first fault with kind ed_undefined, device cpu, opcode $00, pc 0, width 8, frame 0"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fault\\.ed_undefined$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D7
    description: "API version 2's first name has its program, its docs line and the version gates"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^(abi\\.api_v2|abi\\.api_version|docs\\.api_names)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D8
    description: "CPU, beam, deadline and RAM state round-trips, including mid-frame from a running image, and every new field is range-checked"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.(state|fault)\\.' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D9
    description: "The library imports no 64-bit division helper, no mutable global and no unprefixed symbol, and the test-card hashes are unchanged"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^(abi\\.undefined|abi\\.globals|abi\\.symbols|runner\\.hash.*|core\\.testcard\\..*)$'"
        status: pass
    human_judgment: false
  - id: D10
    description: "The ci workflow passes and every inventory entry ran once"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
metrics:
  duration: 9min
  completed: 2026-10-19
---

# Phase 2 Plan 6: Images load and run to a bounded frame deadline Summary

The Z80 now runs inside the library. `cogmaster_load` takes a cartridge image of 1 KiB to 1 MiB + 512 bytes. It
strips a copier header, pads the image with `$FF` to 16 KiB pages and applies the fast-boot hand-off.
`cogmaster_run_frame` steps the CPU to the frame deadline through `src/bus.c`. Overshoot is at most 69 ticks and is
charged to the next frame. The beam starts at the frame origin (line 192, tick 607), so every NTSC frame is exactly
179,208 ticks. API version 2 adds `COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE`. State format 2 saves the CPU, the beam,
the deadline and the work RAM.

## What was built

- **Task 1 (tracer), `8d25e3a`:** `src/bus.c` (129 lines) defines the six seam functions. Each one first brings
  `line` and `t` up to `ticks` by subtraction, wrapping at 684 ticks and 262 lines. It then decodes the no-mapper
  layout: ROM below `$C000`, `$FF` past the image, 8 KiB of RAM mirrored to `$FFFF`. Ports read `$FF`.
  `src/cart.c` and `src/boot.c` hold the copy and the hand-off. `bus.halt_bound.di` and `bus.frame_origin` passed,
  and the tracer gate re-ran green before Task 2.
- **Task 2 (TDD):** RED `92b7eda` added the header name, `tests/api/v2.c`, the guide's loading, status and fault text,
  and five tests. GREEN `39c6bcb` makes the size rule return the new status.
- **Task 3, `f1835ae`:** the state visitor adds 20 register bytes, PC, SP, WZ, I, R and the hidden flags. It also
  adds the deadline, `bus.at`, line, t and 8,192 RAM bytes, all range-checked, with tripwire size asserts. The
  header's ROM CRC-32 is now the image body's CRC. ARCHITECTURE (power-on and boot Beam rows), SMS-SYSTEM (C7 and
  power-on Beam rows) and the guide's reset paragraph now place the beam at the frame origin.

## Verification

- `cmake --workflow --preset ci`: exit 0, 113 + 4 tests.
- `check_junit.cmake` against `tests/inventory/ci.txt`: exit 0 (113 ran once each).
- A local Apple clang build with `-fsanitize=address,undefined` ran 40 tests (core, bus, fault, api, z80): all
  passed. The `asan` preset needs clang-21, which this machine does not have.
- Mutation check: with the RAM visitor removed, `core.state.round_trip` fails on the RAM and Z80 comparisons. With
  it restored, the test passes.
- All acceptance greps exit as specified: no `% 684` or `/ 684`, the origin defines, the stale Beam rows, no planning
  IDs. The header is 3,637 bytes.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Frozen v1 and header checks pinned API version 1**
- **Found during:** Task 2 GREEN
- **Issue:** these checks required version 1 exactly: `tests/api/v1.c`, `tests/header/header.c(pp)`, the consumer
  expectations in `tests/install/tests.cmake` (`api 1`) and the `abi.api_version.self_test` regex (`K <= 1`). The
  bump to 2 broke all of them.
- **Fix:** v1.c and the header checks now accept any version from 1 up, and v1.c compares `v.api` with
  `COGMASTER_API_VERSION`. The consumer and self-test regexes now accept any number.
- **Commits:** 92b7eda, 39c6bcb

**2. [Rule 3 - Blocking] `abi.api_v2` also registers in the nofp lane**
- `tests/abi/tests.cmake` is the nofp lane's only test file, so `tests/inventory/nofp.txt` gained `abi.api_v2`.
- **Commit:** 92b7eda

**3. [Rule 1 - Test bug] Pad buffer for 1 MiB + 496 bytes**
- An image of up to 1 MiB + 511 bytes with no copier header pads to 65 pages. The first draft of the seeded test
  snapshotted into a 1 MiB buffer and tripped the fortify check. The buffer is now 1 MiB + 16 KiB.

**4. [Rule 2 - Docs] Fault codes and width**
- The guide called the kind and device codes opaque and gave width in bytes. It now lists the codes and gives width
  in bits (8), as recorded. `docs/testing.md` gains the `bus` label.

**5. [Scope] Size refusal in the tracer**
- Task 1 already refused out-of-range sizes, with ERR_ARGUMENT, so that `core.api.statuses` kept passing and no
  image above 1 MiB + 512 could be copied. Task 2 changed the status.

## TDD Gate Compliance

- RED: `92b7eda` test(02-06). The classifier verdict was `RED_EVIDENCE_OK` on the CTest JUnit report (target
  `core.load.sizes`). Five tests failed, each on the planned assertion: status 1 (ERR_ARGUMENT) where 7 was
  expected. These were `core.load.sizes`, `core.load.rejected_unchanged`, `abi.api_v2`, `core.api.statuses` and
  `core.api.error_changes_nothing`. `core.boot.handoff` and `fault.ed_undefined` already passed because the
  tracer had implemented them.
- GREEN: `39c6bcb` feat(02-06). All pass.
- REFACTOR: none.

## Known Stubs

These are intentional, and later plans name them:
- `src/bus.c`: `cogmaster__bus_in` returns `$FF` and `cogmaster__bus_out` does nothing until plan 08 (VDP ports)
  and the I/O plan.
- `src/bus.c`: every image maps as the no-mapper layout until plan 09 adds the board probe and mappers.
- `src/frame.c`: a loaded image's field stays black until the phase 3 renderer.

## Commits

- `8d25e3a` feat(02-06): z80 runs images through the bus to a bounded frame deadline
- `92b7eda` test(02-06): failing image size, loader, boot hand-off and ed fault cases
- `39c6bcb` feat(02-06): refuse images outside 1 KiB to 1 MiB + 512 with ERR_UNSUPPORTED_SIZE
- `f1835ae` feat(02-06): save the cpu, beam, deadline and work ram; beam starts at the frame origin

## Self-Check: PASSED
