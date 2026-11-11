---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "02"
subsystem: public-header
tags:
  - header
  - abi
  - api-version-12
  - layout-asserts
status: complete

requires:
  - 08-01
provides:
  - the whole header delta of the phase, with inert behaviour: input axes, the four device constants, the three eye constants, run_result.glasses, info and cart_info default ports, versions 12, 14 and 14
  - tests/api/v12.c, the exact pins of the version 12 additions (registered as abi.api_v12 by the next plan)
affects: [08-03, 08-05, 08-17]

actuals:
  tokens: 3400
  tasks: 2
  commits: 2

plan_head_before: 4cfbcf260b777c4cc74f5ca5364ae498a2e688b0
plan_head_after: 0202d706cf8c7e4d82d2afcd64ae10972294dbe9
commits: 2

tech-stack:
  added: []
  patterns:
    - "A contract-shape pin that a later struct growth would move is relaxed to >= in a commit before the growth, so no commit has a failing existing test"

key-files:
  created:
    - tests/api/v12.c
  modified:
    - include/cogmaster.h
    - src/internal.h
    - docs/embedding.md
    - tests/api/v3.c
    - tests/api/v8.c
    - tests/api/v9.c
    - tests/api/v10.c
    - tests/api/v11.c
    - tests/core/test_core.c

key-decisions:
  - "The shape-only pin relaxations were committed first and run green against the unchanged header; the header growth and tests/api/v12.c landed together in the second commit because abi.api_version demands v12.c the moment the version is 12"
  - "The embedding paragraph is 613 bytes and docs/embedding.md is 34,624 bytes, 2,240 under the 36,864 phase limit; behaviour prose is left to the controllers guide"
  - "tests/api/v12.c keeps == pins for sizes, offsets and versions as the newest file; the next phase moves the versions to >="

requirements-completed: []

coverage:
  - id: C1
    description: "The six existing tests carry their >= and 99 and 31 forms and every api and core test is green against the unchanged header"
    requirement: PAD-05
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(abi\\.api_v|core\\.)' exit 0 (42 of 42) before the header changed"
      status: pass
    human_judgment: false
  - id: C2
    description: "The header carries the whole delta: cogmaster_input is 48 bytes with x1, y1, x2, y2 at 32, 36, 40, 44; run_result is 24 with glasses at 20; info is 76 and cart_info 536 with default_port1 and default_port2 at 68, 72 and 528, 532; the seven constants and versions 12, 14, 14; every offset is a static assert in src/internal.h"
    requirement: PAD-05
    verification:
      kind: unit
      ref: "tests/api/v12.c compiled and run against the library sources, exit 0; cmake --build --preset ci exit 0 with the asserts"
      status: pass
    human_judgment: false
  - id: C3
    description: "abi.api_version finds tests/api/v12.c and every new header identifier in some tests/api/vK.c, and docs.api_names finds every appended name in embedding.md"
    requirement: PAD-05
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(abi\\.api_version|docs\\.api_names)$' exit 0 (2 of 2)"
      status: pass
    human_judgment: false
  - id: C4
    description: "No behaviour changes and no other test changes: the whole ci workflow is green, nothing under tests/io, tests/audio or tests/testcard_pins.cmake differs from origin/main, no line is removed from the three pin files, src/io.c and src/instance.c are untouched, and the check-code budget holds"
    requirement: PAD-05
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci exit 0 (464 of 464); cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-11
---

# Phase 8 Plan 2: The header delta and the shape-only test edits Summary

**The whole header growth of the phase lands once with inert behaviour: `cogmaster_input` 48 bytes with four axis words, the Paddle and Sports Pad devices, the eye constants, `glasses` in the run result and default ports in both info structs, API 12 / behaviour 14 / state 14, with every offset asserted and pinned.**

## Performance

- **Duration:** about 20 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 10 (1 new)

## Accomplishments

- Six existing tests (`tests/api/v3.c`, `v8.c`, `v9.c`, `v10.c`, `v11.c`, `tests/core/test_core.c`) changed shape only: sizes and versions are `>=`, every `offsetof` pin stays `==`, an unknown device is 99 and the undersized input tag is 31. They were green against the unchanged header before it grew.
- `include/cogmaster.h` appends the axes, the four `COGMASTER_DEVICE_*` and three `COGMASTER_GLASSES_*` constants, `glasses`, and `default_port1`/`default_port2` in `cogmaster_info` and `cogmaster_cart_info`. A zeroed result or info means no eye and a pad. The header is 9,708 bytes of its 24,576.
- `src/internal.h` replaces the three size asserts and adds an `offsetof` assert for every appended field of all four structs, plus a cart_info size assert of 536.
- `tests/api/v12.c` pins versions, constants, sizes and offsets and names every appended identifier; `docs/embedding.md` names every appended symbol in one 613-byte paragraph.

## Task Commits

1. **Task 1: Shape-only edits to existing tests** - `e294108` (test)
2. **Task 2: Header delta, layout asserts, embedding paragraph and v12.c** - `0202d70` (feat)

**Plan metadata:** the commit that carries this file.

## Evidence

All from the repository root, no `cd`:

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(abi\.api_v\|core\.)'` (task 1, header unchanged; 42 tests) | 0 |
| `git diff --quiet HEAD -- include src` before the task 1 commit | 0 |
| `ctest --test-dir build/ci -R '^(abi\.api_version\|docs\.api_names)$'` (2 tests) | 0 |
| `cc -std=c17 -fsyntax-only -I include -I build/ci/generated tests/api/v12.c` | 0 |
| v12.c built with `src/*.c` by `cc` and run | 0 |
| `cmake --workflow --preset ci` | 0 (464 of 464) |
| `cmake --workflow --preset hygiene` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (723 check lines of 8,331 src lines) |
| `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` | 0 |
| `git diff origin/main -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt` then `grep -c '^-[^-]'` | 0 and prints 0 |
| `git diff --quiet HEAD -- src/io.c src/instance.c` before the task 2 commit | 0 |

Final sizes and bytes: `sizeof` input 48, run_result 24, info 76, cart_info 536; `include/cogmaster.h` 9,708 bytes; `docs/embedding.md` 34,624 bytes. No run URL: nothing was pushed or dispatched.

## Decisions Made

See key-decisions. `clang-format` realigned the trailing comments of `cogmaster_info.audio_channels` and `fm` and of `cogmaster_cart_info.unsupported_id` and `unsupported_reason` when the new members joined the same comment block; the values and layout are unchanged.

## Deviations from Plan

**1. [Rule 3 - Blocking] The v12.c syntax-check command needs the generated include directory**
- **Found during:** Task 2
- **Issue:** `cc -std=c17 -fsyntax-only -I include tests/api/v12.c` fails because `include/cogmaster.h` includes the generated `cogmaster_version.h`, which only exists under the build directory.
- **Fix:** the same command with `-I build/ci/generated` added exits 0; v12.c was also compiled with the library sources and run, exit 0.
- **Files modified:** none.

**Total deviations:** 1 (acceptance command adjusted, no file changed). **Impact:** none.

## Phase notes

From this plan until plan 08-03 lands, `cogmaster_set_input` still copies the whole struct while the struct is 48 bytes, so a caller that passes tag 32 would be over-read; no test does, nothing may be pushed or dispatched in that window, and plan 08-03 closes it. `tests/api/v12.c` is not yet registered; plan 08-03 registers it as `abi.api_v12`.

## Known Stubs

None.

## Threat Flags

None. The new fields are not read by any code yet; the copy rule and range validation that guard them are plan 08-03.

## Self-Check: PASSED

- `tests/api/v12.c` exists; `git rev-list --count 4cfbcf2..HEAD` was 2 before this file; commits `e294108` and `0202d70` are ancestors of HEAD.
- All acceptance criteria of both tasks were re-run and pass (grep counts 1, 0 and at least 1 for the v3.c edits, 3 and 5 for the header, 42 and 2 registered tests, byte counts below the limits).
