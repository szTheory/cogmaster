---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "03"
subsystem: input
tags:
  - set-input
  - struct-size-tag
  - abi
  - asan
status: complete

requires:
  - 08-02
provides:
  - abi.api_v12 registered on the ci, wasm32 and nofp inventories
  - cogmaster_set_input that reads only the whole fields the caller's size tag covers, validates the assembled copy and range-checks every axis
  - tests/ports/test_input.c, six cases that prove a short caller is never read past its end
affects: [08-04, 08-05, 08-17]

actuals:
  tokens: 4900
  tasks: 2
  commits: 2

plan_head_before: f7ad7fc4f699c4516e89419f1f86958a4216ad1d
plan_head_after: 570d5a5c80c69267092f47b5548a7b8cded15f31
commits: 2

tech-stack:
  added: []
  patterns:
    - "A tagged caller struct is copied byte by byte up to min(tag, ours) rounded down to a whole word, into a zeroed local that is validated before it is stored"

key-files:
  created:
    - tests/ports/test_input.c
  modified:
    - src/io.c
    - tests/abi/tests.cmake
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/nofp.txt

key-decisions:
  - "The copy is a byte loop, not memcpy: a variable-length memcpy into a local of known size compiles to __memcpy_chk on Apple clang, which abi.undefined rejects, so the loop keeps the library free of libc fortify symbols"
  - "Axes are range-checked to +-32767 and then dropped (pending axes stay 0) until plan 08-05 makes them persist"
  - "The highest accepted device stays NONE until plan 08-05 introduces the one shared constant"

requirements-completed: []

coverage:
  - id: C1
    description: "abi.api_v12 pins the version 12 versions, constants, sizes and offsets and runs on the ci, wasm32 and nofp inventories"
    requirement: PAD-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^abi\\.api_v1[0-2]$' exit 0 (3 of 3); a changed offset literal made abi.api_v12 fail with ctest exit 8, and the reverted file passed with exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "cogmaster_set_input never reads a caller past its size tag: a 48-byte object with tag 32 and 0x7F in bytes 32 to 47 is accepted with the pending axes 0, the same object with tag 48 is refused"
    requirement: PAD-05
    verification:
      - kind: unit
        ref: "tests/ports/test_input.c#poison via ctest ports.input.poison; red (exit 1) against the unchanged io.c, green after the change"
        status: pass
    human_judgment: false
  - id: C3
    description: "An exact 32-byte heap block holding the old struct passes through set_input with no read past it"
    requirement: PAD-05
    verification:
      - kind: unit
        ref: "tests/ports/test_input.c#exact_malloc under cc -fsanitize=address,undefined exit 0; with the whole-struct copy restored the same binary aborts with heap-buffer-overflow (exit 134)"
        status: pass
    human_judgment: false
  - id: C4
    description: "Tags 31, 4097, 0 and 0xFFFFFFFF are ERR_STRUCT_SIZE, tag 4096 with a zero tail is accepted, a non-zero unknown tail is refused, and tags 33 to 47 copy whole fields only"
    requirement: PAD-05
    verification:
      - kind: unit
        ref: "tests/ports/test_input.c#tags via ctest ports.input.tags"
        status: pass
    human_judgment: false
  - id: C5
    description: "Every refusal path leaves the saved state bytes and the pending input unchanged, and INT32_MIN, INT32_MIN+1, INT32_MAX and +-32768 on each axis are refused with no signed overflow under UBSan"
    requirement: PAD-05
    verification:
      - kind: unit
        ref: "tests/ports/test_input.c#refusals and #range via ctest ports.input.refusals and ports.input.range; the same binary under -fsanitize=address,undefined exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "run_result and info appended words are not written past a short caller, with sizes 12 to 24 and 60 to 76 into exact blocks and into blocks with a poisoned tail"
    requirement: PAD-05
    verification:
      - kind: unit
        ref: "tests/ports/test_input.c#outputs via ctest ports.input.outputs"
        status: pass
    human_judgment: false

duration: 35min
completed: 2026-11-11
---

# Phase 8 Plan 3: Register abi.api_v12 and bound the set_input copy Summary

**`cogmaster_set_input` now copies only the whole fields a caller's size tag covers into a zeroed local, validates that copy (axes within +-32767), then commits, so a caller built against the 32-byte input struct is never read past its end; `abi.api_v12` is registered on every lane and six `ports.input.*` cases prove the rule.**

## Performance

- **Duration:** about 35 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 7 (1 new)

## Accomplishments

- `abi.api_v12` runs `tests/api/v12.c` beside `abi.api_v11` (labels `abi portable`) and is in the ci, wasm32 and nofp inventories in sorted position.
- `src/io.c` carries `FIRST_SIZE_INPUT` (32, asserted equal to `offsetof(cogmaster_input, x1)`) and the new copy: `n = min(tag, sizeof) & ~3`, a zeroed local, the existing range checks plus each axis in -32767..32767, then a commit with the axes dropped and `size` 0. A refusal changes nothing. `m->pending_input = *in` is gone.
- `tests/ports/test_input.c` (`cogmaster-test-ports-input`, six cases registered as `ports.input.poison`, `exact_malloc`, `tags`, `refusals`, `range`, `outputs`) with labels `ports portable`, TIMEOUT 20, and inventory lines on the ci and wasm32 lanes.

## Task Commits

1. **Task 1: Register abi.api_v12** - `6d523c6` (test)
2. **Task 2: Bounded copy in set_input with its six cases** - `570d5a5` (feat)

**Plan metadata:** the commit that carries this file.

## Evidence

All from the repository root, no `cd`. No run URL: nothing was pushed or dispatched; the hosted asan and nofp jobs at the next dispatch (plan 08-04, dispatch 1) carry those lanes.

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^abi\.api_v1[0-2]$'` (3 tests) | 0 |
| Negative control: `tests/api/v12.c` offset of `x1` changed 32 to 36, rebuilt, `ctest --test-dir build/ci -R '^abi\.api_v12$'` | 8 (fails) |
| Same file reverted with `git checkout -- tests/api/v12.c`, rebuilt, same ctest | 0 |
| `ctest --test-dir build/ci -R '^ports\.input\.'` against the unchanged io.c (5 of 6 fail; `ports.input.poison` prints the axes not 0 and the tag-48 call returning OK) | 8 (red) |
| `build/ci/tests/cogmaster-test-ports-input poison` against the unchanged io.c | 1 |
| `ctest --test-dir build/ci -R '^(ports\.input\.\|abi\.api_v)'` after the change (20 tests) | 0 |
| `cc -std=c17 -fsanitize=address,undefined -fno-sanitize-recover=undefined` over `src/*.c` and `tests/ports/test_input.c`, all six cases | 0 each |
| ASan control: the same build with `local = *in` restored, `exact_malloc` | 134 (heap-buffer-overflow in `cogmaster_set_input`) |
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 |
| `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` | 0 |
| `git diff --quiet HEAD -- tests/api/v12.c` | 0 |
| `clang-format --dry-run` on `src/io.c` and `tests/ports/test_input.c` | 0 |

The poison red run is the control that runs on every machine; Apple clang's ASan also runs on this host, so the exact-allocation control was demonstrated locally as well (exit 134 with the whole-struct copy restored, 0 with the rule). The hosted asan job at plan 08-04's first dispatch is the lane that carries `ports.input.exact_malloc` green in CI; its run URL is not yet available.

## Decisions Made

See key-decisions.

## Deviations from Plan

**1. [Rule 3 - Blocking] `cmake --workflow --preset nofp` cannot run on this host**
- **Found during:** Task 1
- **Issue:** the nofp preset needs GCC (`-mgeneral-regs-only`); this macOS host has only Apple clang, and `cmake` stops with "nofp: -mgeneral-regs-only needs GCC".
- **Fix:** the lane is evidenced by the hosted nofp job. Locally the inventory was checked by `diff` of the `abi.` lines of `tests/inventory/ci.txt` against `tests/inventory/nofp.txt`: the only differences are the header comment lines and `abi.nofp_fixture`, so `abi.api_v12` is in the same sorted position in both.
- **Files modified:** none.

**2. [Rule 1 - Bug] A variable-length `memcpy` into the local broke `abi.undefined`**
- **Found during:** Task 2
- **Issue:** `memcpy(&local, in, n)` compiles to `__memcpy_chk` on Apple clang, an unexpected undefined symbol for the library (`ctest -R '^abi\.undefined$'` exit 8 in the ci workflow).
- **Fix:** a bounded byte loop over `n` (at most 48) copies the same bytes with no libc call; the poison, exact-allocation and tag cases pass unchanged.
- **Files modified:** `src/io.c`.
- **Verification:** `cmake --workflow --preset ci` exit 0.
- **Committed in:** `570d5a5`.

**Total deviations:** 2 (one lane that only hosted CI can run, one self-caused build fault fixed before commit). **Impact:** none on scope.

## Issues Encountered

None beyond the deviations.

## Phase notes

- The highest accepted device stays NONE until plan 08-05 introduces the one shared constant.
- The over-read window of plan 08-02 (a 32-byte caller read past its end by the old whole-struct copy) is closed here: both plans can now be pushed together.
- `tests/api/v12.c` pins versions with `==`; the next phase that bumps them moves those pins to `>=`.

## Known Stubs

None. The axis words are validated and dropped by design until plan 08-05 makes them persist.

## Threat Flags

None. The new size-tag handling narrows what the core reads from a caller; no new endpoint or file access.

## Self-Check: PASSED

- `tests/ports/test_input.c`, `src/io.c` and the three inventories exist; `git rev-list --count f7ad7fc..HEAD` was 2 before this file; commits `6d523c6` and `570d5a5` are ancestors of HEAD.
- All acceptance criteria of both tasks were re-run and pass (3 and 6 registered tests, grep counts 1, 6, 6, 1, 3 and 0, `cmake --workflow --preset ci` exit 0); the ASan lane is evidenced locally with Apple clang and hosted at the next dispatch.
