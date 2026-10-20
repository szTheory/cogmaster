---
phase: 02-the-cpu-matches-the-public-vectors
plan: 07
subsystem: api
status: complete
tags:
  - api-v2
  - profile
  - pal
  - bus
  - state
requires:
  - "02-06: the bus with its frame origin and deadline, API version 2, state format 2"
provides:
  - "COGMASTER_REGION_AUTO/EXPORT/JAPAN and COGMASTER_TV_AUTO/NTSC/PAL; cogmaster_media.region and .tv; cogmaster_info.region and .tv"
  - "m->region and m->tv (resolved, 1 or 2) and cogmaster__frame_ticks / cogmaster__frame_lines in src/internal.h"
  - "A PAL frame of 313 lines, 214,092 ticks; info fps 13,300,856/267,615 on PAL"
  - "ERR_PROFILE_REJECTED for PAL with GG or JAPAN and for region or tv values outside their sets"
  - "State header bytes 24-27: machine, region, tv, a zero byte, each fixed to the instance"
  - "bus.halt_bound.di_pal and core.load.profiles"
affects:
  - "Plan 08 (VDP timing): line events must use cogmaster__frame_lines(m); the frame origin is the same on PAL"
  - "Plan 10 (ports): region and tv readback reads m->region and m->tv"
  - "Plan 11 (runner): --region and --tv map onto media.region and media.tv"
actuals:
  tokens: 14000
  tasks: 2
  commits: 3
plan_head_before: 6280a419b5836962834c3e980cebe7c238bf9e9f
plan_head_after: e6e97bc54ed1da61ba389508e81310073151989e
tech-stack:
  added: []
  patterns:
    - "A media field appended after the first release is read only when the caller's size tag is above the first released size and covers the field, so a field in the old struct's tail padding reads as AUTO"
    - "Frame length and line count come from the instance's tv through two helpers; no other NTSC constant is used at run time"
key-files:
  created: []
  modified:
    - include/cogmaster.h
    - src/internal.h
    - src/instance.c
    - src/bus.c
    - src/frame.c
    - src/state.c
    - tests/api/v2.c
    - tests/bus/test_bus.c
    - tests/bus/tests.cmake
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/core/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/embedding.md
    - .planning/preparation/ARCHITECTURE.md
decisions:
  - "A cogmaster_media size tag at the first released size (4 x P) reads region and tv as AUTO, because on 64-bit targets region sits in that struct's tail padding"
  - "The state header's u32 profile is split into one-byte fields machine, region, tv and profile_reserved, each fixed to the loaded instance, so a state loads only into the same profile; ARCHITECTURE's profile row now says so"
  - "Region or tv values above 2 are ERR_PROFILE_REJECTED, like an unknown machine"
  - "The PAL frame starts at the same frame origin (line 192, tick 607) as NTSC"
  - "requirements-completed stays empty: CPU-02 still needs the EI; HALT case and the hostile VDP-port loop from plan 08"
requirements-completed: []
coverage:
  - id: D1
    description: "DI; HALT under PAL runs ten frames, each stops FRAME with overshoot at most 69 ticks, total ticks are 10 x 214,092 plus the last overshoot, and the beam is back at line 192"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^bus\\.halt_bound\\.di_pal$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every new header name is used by the version 2 program and named in the guide, and the version gates pass"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^(abi\\.api_v2|abi\\.api_version|docs\\.api_names)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "AUTO resolves to export and NTSC; each legal pair reports its region, tv and rate and runs frames of its length; PAL with GG or JAPAN and out-of-set values are refused with the instance byte-identical; a first-size tag reads region and tv as AUTO"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.(load\\.profiles|info)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "200 seeded loads now also draw region and tv, including the refused pairs; every refusal allocates nothing and changes nothing"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.load\\.rejected_unchanged$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D5
    description: "PAL and Japanese states round-trip, a state of another profile is refused, and the machine, region and tv fields are swept and range 1 to 2"
    requirement: CPU-02
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci --no-tests=error -R '^core\\.state\\.' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ci workflow passes and every inventory entry ran once"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
metrics:
  duration: 5min
  completed: 2026-10-19
---

# Phase 2 Plan 7: Region, TV and the PAL frame Summary

`cogmaster_load` now takes `media.region` and `media.tv`. AUTO resolves to an export console on an NTSC set. A
PAL load runs 313-line frames of 214,092 ticks from the same frame origin as NTSC, and `cogmaster_get_info`
reports `region`, `tv` and 13,300,856/267,615 fps for PAL. PAL with a Game Gear or a Japanese console returns
`ERR_PROFILE_REJECTED` and changes nothing. The resolved profile is saved in the state header.

## What was built

- **Task 1 (tracer), `25f3922`:** the header gains the six `COGMASTER_REGION_*` and `COGMASTER_TV_*` constants
  and the `region` and `tv` fields of `cogmaster_media` and `cogmaster_info`. `src/internal.h` pins their offsets
  and keeps the first released sizes. `cogmaster__frame_ticks` and `cogmaster__frame_lines` feed the bus origin,
  the line wrap, the frame deadline and the test card's frame step. `tests/api/v2.c` uses every new name, and the
  guide names each one, the PAL clock and the PAL rate. `bus.halt_bound.di_pal` was added. The tracer gate re-ran
  the four target tests green before Task 2.
- **Task 2 (TDD):** RED `f4771e2` added `core.load.profiles` and extended `core.info`,
  `core.load.rejected_unchanged`, `core.state.round_trip` and `core.state.record_sweep`. GREEN `e6e97bc` refuses
  the two impossible pairs. It splits the state header's profile word into machine, region and tv bytes and makes
  the state's frame length, deadline and line bounds follow the loaded tv.

## Verification

- `cmake --workflow --preset ci`: exit 0, 115 + 4 tests.
- `check_junit.cmake` against `tests/inventory/ci.txt`: exit 0 (115 ran once each).
- Both plan verify commands exit 0, including the `jq -e '.tests | length == 1'` registration check.
- Acceptance: the six names appear in `tests/api/v2.c` and `docs/embedding.md` (0 missing). The header is 3,942
  bytes. The planning-ID grep finds nothing.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `core.api.size_tags` assumed the library's size equals the first released size**
- **Found during:** Task 1
- **Issue:** `check_input` and `check_output` refused `ours - 1`. Now that media and info have grown, a size
  between the first released size and ours is valid.
- **Fix:** both helpers take the first released size and refuse `first - 1`. The first sizes are written as
  literals from the documented layout.
- **Files modified:** tests/core/test_core.c
- **Commit:** 25f3922

**2. [Rule 2 - Correctness] A first-size media tag on 64-bit covers `region`**
- **Issue:** `cogmaster_media` had 4 bytes of tail padding on 64-bit, and `region` now sits there. A version 1
  caller's padding is not guaranteed to be zero.
- **Fix:** load reads region and tv only when the size tag is above the first released size and covers the field.
  `core.load.profiles` checks this with JAPAN and PAL in those bytes. The guide's size rules say so.
- **Commit:** 25f3922, e6e97bc

**3. [Rule 3 - Blocking] `src/frame.c` stepped the test card by the NTSC frame**
- This file was not in the plan's file list. A PAL test card would have broken the state frame checks, so it now
  uses `cogmaster__frame_ticks`.
- **Commit:** 25f3922

**4. [Scope] State profile layout**
- ARCHITECTURE described a u32 "packed profile" of eight fields. The plan asks for region and tv state fields
  with VALIDATE ranges, so the word is now four named bytes. ARCHITECTURE's header row was updated to match.
  State format 2 has not been released, so the format version stays 2.
- **Commit:** e6e97bc

## TDD Gate Compliance

- RED: `f4771e2` test(02-07). The classifier verdict was `RED_EVIDENCE_OK` on a CTest JUnit report (target
  `core.load.profiles`, exit 8). The semantic assessment: four tests failed on their planned assertions.
  `core.load.profiles` got status 0 where 6 was expected for the four PAL+GG and PAL+JAPAN rows.
  `core.load.rejected_unchanged` got status 0 where 6 was expected for the drawn pairs. `core.state.round_trip`
  refused a PAL state into a PAL instance and accepted a state of another profile. `core.state.record_sweep`
  found no machine, region or tv field.
- GREEN: `e6e97bc` feat(02-07). All pass.
- REFACTOR: none.

## Known Stubs

None added. The stubs from plan 06 remain: ports read `$FF`, every image maps without a mapper, and a loaded
image's field stays black.

## Commits

- `25f3922` feat(02-07): load takes region and tv; a PAL frame is 313 lines
- `f4771e2` test(02-07): failing profile pair, seeded region and tv, and profile state cases
- `e6e97bc` feat(02-07): refuse PAL on a Game Gear or Japanese console; save the profile

## Self-Check: PASSED
