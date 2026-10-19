---
phase: 01-a-test-frame-in-retroarch
plan: 02
subsystem: core
tags:
  - c17
  - state-visitor
  - crc32
  - first-fault
  - reset
requires:
  - "01-01: struct cogmaster, cogmaster__check_out_size and cogmaster__write_out, COGMASTER__FIRST_SIZE_* constants, cogmaster-test-core harness"
provides:
  - "Internal state visitor src/state.h, src/state.c: cogmaster__state_size, cogmaster__state_save, cogmaster__state_load, cogmaster__state_record, cogmaster__state_seal; five-mode cogmaster__sio_* primitives"
  - "64-byte little-endian state header (magic 8A 43 47 4D 0D 0A 1A 0A, format version, header size, total size, behaviour revision, API version, packed profile, ROM CRC-32, body CRC-32, frame, ticks, 12 reserved bytes) plus the counter block and the fault record"
  - "cogmaster__crc32 in src/crc32.c (core-only name)"
  - "Public cogmaster_fault (32 bytes), cogmaster_get_fault, cogmaster_reset, COGMASTER_RESET_SOFT 0, COGMASTER_RESET_POWER 1"
  - "Internal struct cogmaster__fault, cogmaster__fault_record (first wins), struct cogmaster__counters, COGMASTER__FIRST_SIZE_FAULT"
  - "cogmaster-test-state binary with nine CTest cases"
affects:
  - runner completion
  - libretro adapter
  - CPU and bus (fault call sites, the trace line)
  - public state API
actuals:
  tokens: 17400
  tasks: 2
  commits: 5
plan_head_before: b9eecb5931e80ad0b14693f79e078990a7565304
plan_head_after: 2a4a53c5f3fde2f3fec486c4ac65e70f8fdaa10b
tech-stack:
  added: []
  patterns:
    - "Header fields are visited by the same field list as the body, so RECORD covers them and the sweep needs no offsets"
    - "A fixed header field is a field whose min and max are its only valid value"
    - "Read-only passes over a const instance copy the pointer through memcpy instead of casting away const"
    - "RED commits declare the new functions with stubs so the target test fails on an assertion, not a build error"
key-files:
  created:
    - src/state.h
    - src/state.c
    - src/crc32.c
    - src/fault.c
    - tests/core/test_state.c
  modified:
    - include/cogmaster.h
    - src/internal.h
    - src/instance.c
    - CMakeLists.txt
    - tests/core/test_core.c
    - tests/core/tests.cmake
    - tests/inventory/ci.txt
    - docs/embedding.md
key-decisions:
  - "The header carries frame, ticks and the packed profile once; the body does not repeat them"
  - "A state loads only into an instance of the same profile: the profile field's only valid value is the instance's machine"
  - "The body CRC-32 covers every byte after the 64-byte header; header fields are checked one by one"
  - "Internal state load reports every refusal as ERR_ARGUMENT, and ERR_NO_CONTENT before a load; state statuses arrive with the public state API"
  - "cogmaster_get_fault is OK with present 0 before a load as well as after it"
  - "reset(POWER) also zeroes the counters, as ARCHITECTURE section 8 says a power cycle does; SOFT keeps them. Both are zero on the card"
  - "Fault bounds: line 0..312, dot 0..341, frame within the state frame cap; with present 0 every fault field must be 0"
patterns-established:
  - "cogmaster-test-state <case>: second test binary for internal visitor and fault tests, labels core;portable"
requirements-completed:
  - FRAME-01
  - FRAME-02
coverage:
  - id: D1
    description: "save, load into a fresh instance, save again is byte-equal for SMS and GG after 0, 1 and 60 frames; a buffer one byte short is ERR_BUFFER_TOO_SMALL"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.round_trip"
        status: pass
    human_judgment: false
  - id: D2
    description: "RECORD emits 50 fields; every field set one step outside its bounds (47 values per machine) is refused and the target's saved bytes are unchanged; the resealed unchanged state loads"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.record_sweep"
        status: pass
    human_judgment: false
  - id: D3
    description: "Bad magic, total size off by one in the field or the buffer length, wrong header size, flipped body CRC, flipped body byte, non-zero reserved byte, ticks off the frame count, and a state of the other machine are each refused with the target unchanged"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.rejects_header"
        status: pass
    human_judgment: false
  - id: D4
    description: "After 60 frames the header frame field reads 60 and ticks reads 10,752,480"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#state.frame_counter"
        status: pass
    human_judgment: false
  - id: D5
    description: "get_fault reports present 0 after create and after load; below 32 bytes it is ERR_STRUCT_SIZE with the struct untouched; at sizeof+8 and 8192 only 32 bytes are written"
    requirement: FRAME-01
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#fault.empty_after_create"
        status: pass
      - kind: unit
        ref: "tests/core/test_core.c#api.first_released_sizes"
        status: pass
      - kind: unit
        ref: "tests/core/test_core.c#api.size_tags"
        status: pass
    human_judgment: false
  - id: D6
    description: "The first recorded fault wins; reset SOFT and POWER clear it and zero frame and ticks; a fault survives a state round trip and a fault-free state clears it"
    requirement: FRAME-01
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#fault.first_wins"
        status: pass
      - kind: unit
        ref: "tests/core/test_state.c#fault.reset_clears"
        status: pass
      - kind: unit
        ref: "tests/core/test_state.c#fault.state_round_trip"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every error status of create, load, reset, run_frame, get_frame, get_info, get_fault and get_versions leaves the visitor bytes and the field unchanged; before a load the whole instance is unchanged"
    requirement: FRAME-01
    verification:
      - kind: unit
        ref: "tests/core/test_state.c#api.error_changes_nothing"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/embedding.md documents reset, faults, the fault struct size and every new public name"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "grep -c 'cogmaster_reset|cogmaster_get_fault|COGMASTER_RESET_SOFT|COGMASTER_RESET_POWER' docs/embedding.md (prints 9, exit 0)"
        status: pass
    human_judgment: false
duration: 13min
completed: 2026-10-19
status: complete
---

# Phase 1 Plan 02: State visitor, first fault and reset Summary

**The test-card instance now has an internal little-endian state visitor. Its 64-byte header, counters and first-fault record round-trip byte for byte, and a RECORD-driven sweep refuses every out-of-range field without changing the instance. The public API gains `cogmaster_reset`, `cogmaster_get_fault` and `cogmaster_fault`, and a test shows that no error path of any public call changes the instance's state.**

## Performance

- **Duration:** about 13 min
- **Started:** 2026-10-19T21:07Z
- **Completed:** 2026-10-19T21:20Z
- **Tasks:** 2
- **Files created:** 5, modified: 8

## Accomplishments

- **Visitor.** One field list drives SIZE, WRITE, VALIDATE, APPLY and RECORD. The list covers the header too, so a fixed header value is a field with min = max, and RECORD can report header offsets as well as body offsets. VALIDATE checks every field and the rule ticks = frame × 179,208 before APPLY runs, so APPLY cannot fail.
- **State size.** The state is 132 bytes: 64 of header, 40 of counters and 28 of fault record. The field buffer is not saved, because the card redraws it every frame.
- **Sweep.** RECORD emits 50 fields. The sweep writes 47 out-of-range values per machine, reseals each state, and confirms every one is refused with the target's saved bytes unchanged. A resealed copy of the unchanged state loads, as the control.
- **First fault and reset.** The first recorded fault wins. Both reset kinds clear the fault and zero frame and ticks. `get_fault` applies the same output-struct rule as info, versions and run_result. No code in `src/` calls the recorder yet.
- **Errors change nothing.** `core.api.error_changes_nothing` triggers 35 error statuses across all eight status-returning public calls. After each one it compares the visitor bytes and the field buffer, or the whole instance if nothing is loaded.

## Task Commits

1. **Task 1: state visitor, round trip, RECORD sweep**: `4667f3c` (test, RED), `f825d28` (feat, GREEN)
2. **Task 2: first fault, reset, error paths**: `61be86e` (test, RED), `7340b05` (feat, GREEN)
3. **Citation fix after review of the summary**: `2a4a53c` (docs, comment only)

## Evidence (command, exit status)

- `cmake --workflow --preset ci`: exit 0, 23 tests.
- `ctest --test-dir build/ci --no-tests=error -R '^core\.state\.' --output-on-failure && ctest --test-dir build/ci -R '^core\.state\.' --show-only=json-v1 | jq -e '.tests | length == 4'`: exit 0.
- `ctest --test-dir build/ci --no-tests=error -R '^core\.(fault|api)\.' --output-on-failure && ctest --test-dir build/ci -R '^core\.(fault|api)\.' --show-only=json-v1 | jq -e '.tests | length == 8'`: exit 0.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `build/ci/tests/cogmaster-test-state state.record_sweep` printed `state fields: 50 recorded, 47 out-of-range values refused` for each machine: exit 0. At the end of Task 1, before the fault record existed, it printed 40 recorded and 43 refused.
- `grep -Ec 'cogmaster_(state_size|save_state|load_state)' include/cogmaster.h` printed 0.
- `grep -Ec 'cogmaster_status cogmaster_(reset|get_fault)\(' include/cogmaster.h` printed 2.
- `grep -c 'cogmaster__fault_record' src/frame.c src/instance.c` printed 0 for each file.
- `wc -c include/cogmaster.h` printed 3,588, below 24,576.
- `grep -rEn '\b(D-[0-9]{2}|FRAME-[0-9]{2}|DEC\.[0-9]+|01-0[0-9])\b' src include runner tests docs` printed nothing (exit 1).
- `sh scripts/hygiene.sh --tree`: exit 0.
- A one-off build outside the tree with `-fsanitize=address,undefined -fno-sanitize-recover=all` (Apple clang) passed all 23 tests: `ctest`, exit 0.

## TDD Gate Compliance

- **Task 1:** RED `4667f3c` came before GREEN `f825d28`. RED was recorded as `ctest --preset ci -R '^core\.state\.round_trip$' --output-junit red-junit.xml` with exit 8, and `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK` (target_test_failed). Semantic assessment: the RED commit has the visitor interface with stub bodies, so all four state cases built cleanly and failed on assertions about the planned behaviour (state size 0, RECORD count 0, swept 0). None failed on a build or fixture error.
- **Task 2:** RED `61be86e` came before GREEN `7340b05`. RED was recorded as `ctest --preset ci -R '^core\.fault\.first_wins$' --output-junit red-junit.xml` with exit 8, and the classifier returned `RED_EVIDENCE_OK`. Semantic assessment: `get_fault` and `reset` were stubs returning ERR_ARGUMENT. Seven cases failed on assertions: wrong statuses (1 where 0, 2 or 5 was expected) and present 0 where 1 was expected. The reset kinds were test-local macros in RED and entered the header in GREEN, following plan 01's rule that a constant enters with its behaviour.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `-Wcast-qual` refused the seed's const cast**
- **Found during:** Task 1 GREEN
- **Issue:** The seed passed a const instance to the walk with `(T *)m`, and the project's warning set makes that cast an error.
- **Fix:** A small `walkable()` helper copies the pointer with `memcpy`, with a comment stating why this is defined: SIZE, WRITE and RECORD only read, and an instance is never a const object.
- **Files modified:** src/state.c
- **Commit:** f825d28

### Interpretations and additions

- **No duplicated fields.** The plan lists the profile, frame and ticks in the header and again as body content. They are stored once, in the header. The body holds the counters and the fault record.
- **Profile as identity.** The profile field accepts only the target instance's machine, so a GG state is refused by an SMS instance. A test checks this.
- **CRC scope.** The body CRC-32 covers the bytes after the header, as its name says. Header fields are each range-checked, so a header change outside the CRC is still refused.
- **Bounds.** Ticks are capped at 2^62 (ARCHITECTURE section 2) and frames at 2^62 / 179,208. The counters have no bound below the u32 maximum, so the sweep counts them but cannot push them out of range. The fault's line and dot are bounded at 312 and 341, from the PAL line count and the 342 dots per line. If no fault is present, every fault field must be zero; `fault.state_round_trip` checks that cross-field rule.
- **`get_fault` before load** returns OK with present 0, as D-02 requires ("present = 0 after create"). It does not return ERR_NO_CONTENT like `get_info`.
- **POWER zeroes the counters** and SOFT keeps them, following ARCHITECTURE section 8. On the card both kinds have the same effect, as the plan says.
- **Additions beyond the listed cases:** `api.size_tags` also runs the output-struct rule on `cogmaster_fault`. `state.rejects_header` also covers ticks off the frame count, a flipped body byte, the other machine's state, NULL arguments, and save or load before content.

## Requirement status

`requirements-completed` lists FRAME-01 and FRAME-02 because this plan delivers its share of each. Neither box in `.planning/REQUIREMENTS.md` is ticked, because later plans in this phase still carry them: the runner, the libretro core, packaging, and the same hashes and state round trip on all six platforms plus `be32` and `wasm32`.

## Known Stubs

None. Nothing calls `cogmaster__fault_record` in this release. That is intended: phase 2 adds the call sites and `--trace first-fault`. The device and kind codes stay opaque bytes until then.

## Issues Encountered

None beyond the warning fix above.

## Next Phase Readiness

The cross-platform legs (`be32`, `wasm32`, the six-platform matrix) will run the `core.state.*`, `core.fault.*` and `core.api.error_changes_nothing` cases unchanged, because they carry the `portable` label. Plan 03's runner CRC must be named `cogmaster__runner_crc32` so it does not collide with the core's `cogmaster__crc32`.

## Self-Check: PASSED

- src/state.h, src/state.c, src/crc32.c, src/fault.c and tests/core/test_state.c exist on disk.
- Commits 4667f3c, f825d28, 61be86e, 7340b05 and 2a4a53c are ancestors of HEAD.
