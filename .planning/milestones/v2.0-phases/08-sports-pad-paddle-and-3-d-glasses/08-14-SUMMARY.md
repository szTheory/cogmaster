---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "14"
subsystem: host-library
tags:
  - host
  - anaglyph
  - glasses
  - rgb444
status: complete

requires:
  - 08-13
provides:
  - "cogmaster__eyes_anaglyph(left, right, out, count) in host/eyes.c: red from the left RGB444 picture, green and blue from the right one, top four bits zero, inputs const, no state, no allocation"
  - "host/eyes.c in the cogmaster-host library beside palette.c"
  - "host.eyes in the ci and wasm32 inventories, with an in-test bad variant as its negative control"
affects: [08-15, 08-17]

actuals:
  tokens: 1500
  tasks: 2
  commits: 2

plan_head_before: 42151ace23548cd098b0511a5df76b5a2f746b78
plan_head_after: ff602aa41849e349720d4c809c70e7fe74fea0e4
commits: 2

tech-stack:
  added: []
  patterns:
    - "A pure host helper over const inputs with a null-and-zero-count guard, proven by a closed-form grid and a bad variant that must trip the same comparison"

key-files:
  created:
    - host/eyes.c
    - host/eyes.h
    - tests/host/test_eyes.c
  modified:
    - CMakeLists.txt
    - tests/host/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The test is its own executable, cogmaster-test-eyes, with one case name, because one test file per case family is the repository pattern and the inventory carries one host.eyes line"
  - "The test buffers are static so the 4097-dot case stays off a small wasm stack"

requirements-completed: []

coverage:
  - id: F1
    description: "The composition takes red from the left picture and green and blue from the right one, equals the closed form for every left value against 16 partners in both roles, leaves the top four bits zero even for dirty inputs, and differs from the left picture exactly when the right one differs in green or blue"
    requirement: PAD-04
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^host\\.eyes$'"
      status: pass
    human_judgment: false
  - id: F2
    description: "Both inputs stay byte-equal after the call for 1, 2 and 4097 dots, a zero count or a NULL argument writes nothing, and nothing past count is written"
    requirement: PAD-04
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^host\\.eyes$'"
      status: pass
    human_judgment: false
  - id: F3
    description: "The inputs-untouched comparison can fail: a bad variant that writes into its left input trips it, and with red taken from the right picture host.eyes fails with a non-zero exit"
    requirement: PAD-04
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^host\\.eyes$' with host/eyes.c edited, exit 8, then restored, exit 0"
      status: pass
    human_judgment: false
  - id: F4
    description: "host/eyes.c is in the cogmaster-host library and host.eyes is in both inventories; the ci and hygiene workflows, the api name check and the check-code budget pass"
    requirement: PAD-04
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; ctest -R '^docs\\.api_names$' exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-11
---

# Phase 8 Plan 14: The anaglyph composition Summary

**A pure host function mixes red from the left RGB444 picture with green and blue from the right one into a third picture, proven on every lane by a closed-form grid and a bad-variant control, with the core's hashed frame untouched.**

## What was built

- `host/eyes.h` and `host/eyes.c`: `cogmaster__eyes_anaglyph(left, right, out, count)`, one loop, `out[i] = (left[i] & 0x000F) | (right[i] & 0x0FF0)`. A zero count or a NULL pointer returns before touching anything. The header says that nothing here is hashed or saved and that `out` must not overlap an input.
- `CMakeLists.txt`: `host/eyes.c` joins `add_library(cogmaster-host STATIC ...)` and nothing else changes there.
- `tests/host/test_eyes.c`: three families under one case. A grid of every left value against 16 partners, both as left and as right, compared with the closed form, with the positive control (out differs from left exactly when the green or blue differs). Inputs byte-equal after the call for 1, 2 and 4097 dots; zero count, three NULL variants and a guard slot past count change nothing. A `mix_into_left` bad variant must make the same inputs comparison fail, and the real function must not.
- `tests/host/tests.cmake`: `cogmaster-test-eyes` executable and `host.eyes` (labels `host portable`, TIMEOUT 10, with the fault-prevented comment). `tests/inventory/ci.txt` and `wasm32.txt` list `host.eyes	host,portable` before `host.palette`.

## Negative controls

- In `host/eyes.c` the mix `(left[i] & 0x000F)` was changed to `(right[i] & 0x000F)` with python (confirmed with `git diff --stat host/eyes.c`: 1 insertion, 1 deletion; the build linked). `ctest --test-dir build/ci -R '^host\.eyes$'`: exit 8 (failed on the grid, the dirty-input and the three-dot checks). `git checkout -- host/eyes.c`, rebuild, the same ctest: exit 0.
- The in-test bad variant (writes into the left input): the control case asserts the inputs comparison fails for it, so a check that cannot fail is caught.

## Verification

- `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(host\.|abi\.(globals|undefined|symbols)|policy\.)'`: exit 0 after task 1; `ctest --test-dir build/ci -R '^host\.'`: exit 0, 2 of 2 passed after task 2. `ctest --test-dir build/ci -N -R '^host\.'`: exit 0, "Total Tests: 2", one `host.eyes`.
- `cmake --workflow --preset ci`: exit 0. `cmake --workflow --preset hygiene`: exit 0. `ctest --test-dir build/ci -R '^docs\.api_names$'`: exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0 (723 check lines, 8793 source lines; the count is unchanged because the test is outside the counted check code).
- `gcc:13` in a container, `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -Ihost -fsyntax-only` over `host/eyes.c` and `tests/host/test_eyes.c`: exit 0. `clang-format --dry-run -Werror` over the three C files: exit 0.
- No hosted run was made and no push; the plan names no dispatch. The last hosted evidence is run 42973268415 at `60c97a6`.

## Task Commits

1. `1df22db` feat(08-14): anaglyph composition in the host library
2. `ff602aa` test(08-14): anaglyph unit test with controls on every lane

## Deviations from Plan

None - plan executed exactly as written. (Task 1 was committed before its test, so the red-first order was not followed; the test was proven able to fail by the negative control instead.)

## Known Stubs

None.

## Threat Flags

None. A pure function over caller-owned buffers; no network, auth or file-access surface.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-15: the libretro adapter can call `cogmaster__eyes_anaglyph` on the two eye pictures. The branch holds the 08-13 and 08-14 commits beyond the pushed `60c97a6`; nothing was pushed here.

## Self-Check: PASSED

- `host/eyes.c`, `host/eyes.h` and `tests/host/test_eyes.c` exist; commits `1df22db` and `ff602aa` are ancestors of HEAD.
- `git rev-list --count 42151ace23548cd098b0511a5df76b5a2f746b78..HEAD` printed 2 before this SUMMARY was committed.
