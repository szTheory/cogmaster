---
phase: 09-light-phaser
plan: "02"
subsystem: vdp-io-hooks
tags:
  - vdp
  - io
  - h-counter
  - refactor
  - gun
status: complete

requires:
  - 09-01
provides:
  - cogmaster__vdp_hcount_at, a pure H counter of an in-line tick, and cogmaster__vdp_latch_h_at
  - th_edge in src/io.c, the single latch site, and a sensor mask on th_pins that is 0 at every caller
  - src/gun.c with the empty cogmaster__gun_sync and cogmaster__gun_wrap hooks
  - ports.gun.latch_at on the ci and wasm32 lanes
affects: [09-03, 09-04, 09-08]

actuals:
  tokens: 1900
  tasks: 2
  commits: 2

plan_head_before: 960a03c72d9d9a457c878cacf5c16c00fda64347
plan_head_after: 3091b2bb465b51daf3516b016fbedd44e2bc32cf
commits: 2

tech-stack:
  added: []
  patterns:
    - "The H counter is a function of the in-line tick; the bus-tick forms are wrappers over it, so a later sensor latches at the tick of its own edge"
    - "One function holds the rising-TH latch condition; a sensor edge reaches it without a ports_th call"

key-files:
  created:
    - src/gun.c
    - tests/ports/test_gun.c
  modified:
    - src/vdp.c
    - src/io.c
    - src/internal.h
    - CMakeLists.txt
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The sensor mask applies only on a non-Game-Gear machine and only to a port whose TH is an input, so a Game Gear and an output TH never see it"
  - "th_edge takes the in-line tick as uint32_t while cogmaster__io_th_changed keeps the absolute uint64_t tick for ports_th, and passes m->bus.t to th_edge"

requirements-completed: []

coverage:
  - id: C1
    description: "vdp_hcount is cogmaster__vdp_hcount_at at the bus tick and cogmaster__vdp_latch_h wraps cogmaster__vdp_latch_h_at, with the origin, jump and skip constants unchanged and no hunk in vdp_next_event or cogmaster__vdp_event"
    requirement: GUN-04
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.latch_at$' exit 0 over all 684 in-line ticks against a copy of the old formula"
      status: pass
    human_judgment: false
  - id: C2
    description: "th_pins takes a sensor mask that is 0 at port_dd and both calls in cogmaster__io_th_changed, and th_edge holds the one latch condition; no other call of cogmaster__vdp_latch_h remains in src/io.c"
    requirement: GUN-04
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(io\\.|bus\\.|audio\\.|core\\.|vdp\\.|ports\\.|abi\\.(globals|undefined|symbols)|fixtures\\.|runner\\.hash)' exit 0 (233 of 233, including the 256 by 256 latch differential on six profiles)"
      status: pass
    human_judgment: false
  - id: C3
    description: "src/gun.c exists as stubs with no caller, constant or table and is in the library source list"
    requirement: GUN-01
    verification:
      kind: other
      ref: "grep -c cogmaster__gun_sync src/io.c src/vdp.c src/frame.c prints 0 for each; grep -c gun.c CMakeLists.txt prints 1; gcc 13 strict -fsyntax-only on src/gun.c src/vdp.c src/io.c exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "The latch_at case cannot pass vacuously: an origin one lower than the copy disagrees with it on some tick, asserted in the case, and moving the core origin by one turns the case red"
    requirement: GUN-04
    verification:
      kind: unit
      ref: "python edit VDP_HCOUNT_ORIGIN 32 to 33, rebuild exit 0, ctest -R '^ports\\.gun\\.latch_at$' exit 8; reverted, rebuild, same ctest exit 0"
      status: pass
    human_judgment: false
  - id: C5
    description: "No behaviour moved: the existing io, vdp, bus, audio, testcard pin and ports test sources equal origin/main, and both workflows and the budget check pass"
    requirement: GUN-04
    verification:
      kind: integration
      ref: "git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c exit 0; cmake --workflow --preset ci exit 0 (546 of 546); cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-11
---

# Phase 9 Plan 2: The hooks the sensor will use Summary

**The H counter is now a pure function of the in-line tick (`cogmaster__vdp_hcount_at`), every rising-TH latch goes through one `th_edge`, a sensor mask on `th_pins` is wired at 0, and `src/gun.c` exists with empty hooks; `ports.gun.latch_at` pins all 684 ticks against a copy of the old formula and no latch position moved.**

## Performance

- **Duration:** about 20 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 9 (2 new)

## Accomplishments

- `src/vdp.c` gains `cogmaster__vdp_hcount_at` and `cogmaster__vdp_latch_h_at`; `vdp_hcount` and `cogmaster__vdp_latch_h` are wrappers at the bus tick. The per-line event path has no hunk.
- `src/io.c` routes the console's own `$3F` latch through `th_edge`; `th_pins` takes a low mask applied only to input TH pins on a non-Game-Gear machine, and every caller passes 0.
- `src/gun.c` carries the provenance comment and the two stub hooks, declared in `src/internal.h` and listed in the library sources.
- `tests/ports/test_gun.c` holds `latch_at`: the exhaustive tick sweep, the bus-tick latch, sixteen pairs proving the latch uses its own tick, and the off-by-one control.

## Task Commits

1. **Task 1: One latch through hcount_at, th_edge and an empty gun module** - `0c1a4fe` (refactor)
2. **Task 2: ports.gun.latch_at with its control, registered on ci and wasm32** - `3091b2b` (test)

**Plan metadata:** the commit that carries this file.

## Evidence

All from the repository root, no `cd`:

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(io\.\|bus\.\|audio\.\|core\.\|vdp\.\|ports\.\|abi\.(globals\|undefined\|symbols)\|fixtures\.\|runner\.hash)'` (task 1, 233 tests) | 0 |
| `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I src -I include -I build/ci/generated src/gun.c src/vdp.c src/io.c` | 0 |
| same container command on `tests/ports/test_gun.c` with `-I tests` | 0 |
| `clang-format --dry-run -Werror` on the five changed C files and headers | 0 |
| `ctest --test-dir build/ci -R '^(ports\.\|io\.\|vdp\.)'` (task 2, 106 tests) | 0 |
| negative control: python edit `VDP_HCOUNT_ORIGIN` 32 to 33 (`git diff --stat HEAD` showed 1 line in src/vdp.c), rebuild, `ctest -R '^(ports\.gun\.latch_at\|ports\.latch)'` | build 0, ctest 8 (latch_at red) |
| same control reverted with `git checkout -- src/vdp.c`, rebuild, same ctest | 0 |
| `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c` | 0 |
| `cmake --workflow --preset ci` | 0 (546 of 546) |
| `cmake --workflow --preset hygiene` | 0 (50 of 50) |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (723 of 8851 lines, the counted check code is unchanged) |

Acceptance counts: `cogmaster__vdp_hcount_at` appears 3 times in src/vdp.c and once in src/internal.h; `th_edge` 2 in src/io.c; `cogmaster__vdp_latch_h(` 0 in src/io.c; `gun.c` once in CMakeLists.txt; `cogmaster__gun_sync` 0 in io.c, vdp.c and frame.c; the vdp diff has 0 hunks headed `vdp_next_event` or `cogmaster__vdp_event`; no planning ID in an added src line; `ports.gun.latch_at` appears once in each of ci.txt and wasm32.txt and in `ctest -N`. No local perf reading exists (no Cachegrind here); the perf delta is the hosted reading of plan 04. No run URL: nothing was pushed or dispatched.

## Decisions Made

See key-decisions.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The container compile needed the generated include directory**
- **Found during:** Task 1 acceptance
- **Issue:** the plan's command with only `-I src -I include` fails on the missing generated version header.
- **Fix:** added `-I build/ci/generated`, as the phase notes say; same flags otherwise.
- **Files modified:** none
- **Commit:** none

**2. [Observation] The existing ports.latch cases do not fail under the origin control**
- **Found during:** Task 2 negative control
- **Issue:** the plan expected `ports.latch` to go red as well; it stayed green because those cases assert whether a latch fires, not the counter value it captures. Only `ports.gun.latch_at` fails, which is the position pin this plan adds.
- **Fix:** none needed; recorded so the claim is not repeated.
- **Files modified:** none
- **Commit:** none

The plan's "origin off by one" control was run as 33 (one above), and the in-test control uses 31 (one below); both are off by one.

## Known Stubs

`cogmaster__gun_sync` and `cogmaster__gun_wrap` in `src/gun.c` are intentional empty hooks, filled by plan 04. They affect no output.

## Threat Flags

None. No input reaches the new mask yet and no new endpoint or file access is added.

## Self-Check: PASSED

- `src/gun.c` and `tests/ports/test_gun.c` exist; commits `0c1a4fe` and `3091b2b` are ancestors of HEAD.
- Every acceptance criterion of both tasks was re-run and passes.
