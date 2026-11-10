---
phase: 01-a-test-frame-in-retroarch
plan: 01
subsystem: core
tags:
  - c17
  - cmake
  - ctest
  - test-card
  - sha256
  - size-tags
requires: []
provides:
  - "Public header include/cogmaster.h: versions, create, destroy, load, run_frame, get_frame, get_info; statuses 0..6; machine AUTO/SMS2/GG; stop FRAME; frame flag FULL_FIELD"
  - "Generated cogmaster_version.h from version.txt (COGMASTER_VERSION_MAJOR/MINOR/PATCH)"
  - "Pure integer SMS and GG test card drawn into the 256x192 RGB444 field, crop and full-field copy"
  - "cogmaster-run --frames N --hash [--machine auto|sms|gg] with exit statuses 0, 1, 2"
  - "Exact-inventory CTest harness (cogmaster_add_test, deferred inventory check), check_junit.cmake, expect_fail.cmake, check_hash.cmake"
  - "Pinned card hashes in tests/testcard_pins.cmake (COGMASTER_TESTCARD_SHA256_SMS, COGMASTER_TESTCARD_SHA256_GG)"
  - "Size-tag helpers cogmaster__check_size (input, 4096 cap), cogmaster__check_out_size and cogmaster__write_out (output), COGMASTER__FIRST_SIZE_* constants"
  - "docs/embedding.md naming every public symbol"
affects:
  - state visitor and first-fault record
  - runner completion
  - libretro adapter
  - packaging and ABI gates
  - CI lanes
actuals:
  tokens: 26500
  tasks: 3
  commits: 5
plan_head_before: 8ff15788160d4ba682d6fbcf795fcbd716b06c57
plan_head_after: c53bae9e229615760dd7a35c4ea851e292b50c65
tech-stack:
  added: []
  patterns:
    - "Header grows only with behaviour: each constant enters in the commit that returns or reads it"
    - "Input structs: lower bound first-released size, upper bound 4096, zero tail; output structs: lower bound only, write min(size, sizeof)"
    - "Frame hash over LE-serialised crop rows, never the in-memory uint16_t array"
    - "Pinned hashes live in one CMake file and are checked through a -P script with a wrong-pin negative control"
key-files:
  created:
    - version.txt
    - CMakeLists.txt
    - CMakePresets.json
    - cmake/CogmasterTests.cmake
    - cmake/version.h.in
    - include/cogmaster.h
    - src/internal.h
    - src/instance.c
    - src/frame.c
    - src/testcard.c
    - runner/main.c
    - runner/sha256.c
    - runner/sha256.h
    - runner/exit_status.h
    - tests/tests.cmake
    - tests/testcard_pins.cmake
    - tests/check.h
    - tests/core/tests.cmake
    - tests/core/test_core.c
    - tests/runner/tests.cmake
    - tests/cmake/check_junit.cmake
    - tests/cmake/expect_fail.cmake
    - tests/cmake/check_hash.cmake
    - tests/inventory/ci.txt
    - docs/embedding.md
  modified: []
key-decisions:
  - "Status values settled: OK 0, ERR_ARGUMENT 1, ERR_STRUCT_SIZE 2, ERR_NO_MEMORY 3, ERR_BUFFER_TOO_SMALL 4, ERR_NO_CONTENT 5, ERR_PROFILE_REJECTED 6"
  - "get_info before a successful load returns ERR_NO_CONTENT, like run_frame and get_frame"
  - "get_frame refuses pitch_bytes above 65536 with ERR_ARGUMENT so row offsets fit a 32-bit size_t"
  - "Wrong-hash negative control runs through expect_fail.cmake (exit 1 and 'hash mismatch'), not a bare WILL_FAIL"
  - "Both pinned digests equal the planning oracle values; neither was re-pinned to the C output"
patterns-established:
  - "cogmaster-test-core <case>: one test binary, one CTest per case, labels core;portable"
  - "Runner checks pass the build's CMAKE_CROSSCOMPILING_EMULATOR through a $<SEMICOLON>-escaped -DCMD list"
requirements-completed:
  - FRAME-01
  - FRAME-02
coverage:
  - id: D1
    description: "cmake --workflow --preset ci configures, builds and runs exactly the 14 tests listed in tests/inventory/ci.txt, each once"
    requirement: FRAME-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0); cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Configure fails naming the fault for an unlisted test, an unsorted inventory, and check_junit fails naming a listed test absent from the JUnit file"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "scratch copy with an extra cogmaster_add_test: cmake -S <copy> --preset ci exit 1, 'registered but not listed'; reordered inventory: exit 1, 'is not sorted'; JUnit without runner.hash.sms: check_junit exit 1, 'in the JUnit file 0 times'"
        status: pass
    human_judgment: false
  - id: D3
    description: "cogmaster-run --frames 60 --hash prints the SMS card hash e14d77ce...57ae7 and --machine gg prints the GG card hash 7404ed4a...5e779, both equal to the independent planning oracle"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash.sms"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash.gg"
        status: pass
    human_judgment: false
  - id: D4
    description: "A wrong pinned hash makes check_hash.cmake exit 1 with 'hash mismatch'"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash.self_test"
        status: pass
    human_judgment: false
  - id: D5
    description: "Every card rule is checked at its boundary for both machines, the SMS chart holds exactly the 64 SMS colours, the GG chart all 4,096 RGB444 values as 2x2 cells, and the GG field is magenta outside the window"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/test_core.c#testcard.static, testcard.boundaries, testcard.all_colours, testcard.full_field"
        status: pass
    human_judgment: false
  - id: D6
    description: "Per-machine info (crop, field, max, fps 1640625/27379, PAR 8/7 and 6/5) and the rejected profile leaving the loaded machine in place"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/test_core.c#info, load.profile_rejected"
        status: pass
    human_judgment: false
  - id: D7
    description: "All seven statuses have a tested trigger; input and output size-tag rules including the 4096 cap and the first-released sizes; allocator contract; wide pitch leaves padding untouched"
    requirement: FRAME-01
    verification:
      - kind: unit
        ref: "tests/core/test_core.c#api.size_tags, api.statuses, api.first_released_sizes, create.allocator, frame.pitch_wide"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/embedding.md states the size-tag rules, the 4096-byte cap and its reason, the allocator contract, the frame format and every function and status, and names every public symbol"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "grep -q 4096 docs/embedding.md (exit 0); every COGMASTER_STATUS_ name of the header in the doc; grep -Ec '[0-9]+\\.[0-9]+\\.[0-9]+' docs/embedding.md prints 0"
        status: pass
    human_judgment: false
duration: 12min
completed: 2026-10-19
status: complete
---

# Phase 1 Plan 01: Test card tracer Summary

**The library draws the SMS and Game Gear test cards into the real 256x192 RGB444 field, and `cogmaster-run --frames 60 --hash` prints the SHA-256 of each crop. Both digests equal the independent planning oracle, and a 14-test exact-inventory `ci` workflow pins them.**

## Performance

- **Duration:** about 12 min
- **Started:** 2026-10-19T20:55:41Z
- **Completed:** 2026-10-19T21:07:39Z
- **Tasks:** 3
- **Files created:** 25

## Accomplishments

- The tracer runs end to end: `testcard.c` draws into the field, `get_frame` crops it, the runner serialises the rows low byte first and hashes them, and a pinned CTest checks the result inside `cmake --workflow --preset ci`. The SMS hash matched the planning oracle on the first build.
- Game Gear card: the 160x144 window at (48, 24) holds all 4,096 RGB444 values, and the rest of the field is magenta. `COGMASTER_FRAME_FULL_FIELD` returns the whole field. The GG hash matches the oracle.
- The phase-1 status set is complete (0..6). Each status entered the header in the commit that returns it, and each has a tested trigger.
- Size tags follow one rule for input structs (first-released lower bound, a 4096 upper bound, a zero tail) and one for output structs (first-released lower bound, min(size, sizeof) written). Internal constants pinned by `_Static_assert` hold each struct's first-released size.
- `docs/embedding.md` names every public symbol and gives the condition behind each status.

## Task Commits

1. **Task 1: end-to-end SMS card (tracer)**: `a5ab3e9` (feat)
2. **Task 2: Game Gear card, crop window, full field**: `9e7beb2` (test, RED), `bc412f4` (feat, GREEN)
3. **Task 3: remaining statuses, first-released sizes, embedding guide**: `ad1eff6` (test, RED), `c53bae9` (feat, GREEN)

## Evidence (command, exit status)

- `cmake --workflow --preset ci` from a removed `build/ci`: exit 0, 14 tests.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `build/ci/cogmaster-run --frames 60 --hash | grep -Eq '^frame 60 sha256 e14d77ce564f0b1498f703134e82e6c1fc718a27c94e49b5a35b95a981857ae7$'`: exit 0.
- `build/ci/cogmaster-run --frames 60 --hash --machine gg | grep -Eq '^frame 60 sha256 7404ed4adc74d6ccbe99f0e157fe0db873e78895834799123cbb04495db5e779$'`: exit 0.
- `build/ci/cogmaster-run --frames 0 --hash`: exit 2. `build/ci/cogmaster-run --probe`: exit 2. `--machine xyz`: exit 2.
- The Task 2 selection of 9 tests and the Task 3 selection of 5, run with `ctest ... --show-only=json-v1 | jq -e`: exit 0 for each.
- In a scratch copy of the tree, adding an unlisted `cogmaster_add_test` made `cmake -S <copy> --preset ci` exit 1 with `registered but not listed`. A reordered inventory exited 1 with `is not sorted`. A JUnit file without `runner.hash.sms` made `check_junit.cmake` exit 1, naming that test. All three were run once and the copy was discarded.
- `grep -rEn '\b(D-[0-9]{2}|FRAME-[0-9]{2}|01-0[0-9])\b' src include runner tests docs` printed nothing (exit 1).
- `sh scripts/hygiene.sh --tree`: exit 0.
- `grep -Ec '^#define COGMASTER_STATUS_[A-Z_]+ [0-6]u' include/cogmaster.h` printed 7. The header is 3,045 bytes and `docs/embedding.md` 11,523 bytes.
- A one-off build with `-fsanitize=address,undefined -fno-sanitize-recover=all` (Apple clang, outside the tree) passed all 14 tests (`ctest`, exit 0).

## TDD Gate Compliance

- Task 2: RED `9e7beb2` (test) came before GREEN `bc412f4` (feat). RED was recorded as `ctest --preset ci -R '^core\.testcard\.boundaries$'` with exit 8; the JUnit report showed the target failed, and `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK` (target_test_failed). By inspection, five core cases failed on assertions about the planned behaviour (a full-field `get_frame` returned ERR_ARGUMENT, GG load was refused, the profile status was 1 instead of 6) and `runner.hash.gg` exited 2 on `--machine gg`. No case failed on a build or fixture error.
- Task 3: RED `ad1eff6` came before GREEN `c53bae9`. RED was recorded as `ctest --preset ci -R '^core\.api\.statuses$'`, and the classifier returned `RED_EVIDENCE_OK`. By inspection, `core.api.statuses` failed because run_frame, get_frame and get_info before a load returned 1 where 5 was expected, and a short pitch returned 1 where 4 was expected. `core.create.allocator` failed because a NULL allocation returned 1 where 3 was expected. `core.api.first_released_sizes` and `core.frame.pitch_wide` already passed at RED. That was investigated and is expected: the sizeof lower bound and arbitrary pitches already worked from Task 1, and these cases pin that behaviour. The new part of Task 3 is the named constants and the statuses.
- The RED commits use test-local value macros so that the header gains each constant only in the commit that gives it behaviour. GREEN replaced them with the header names.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Wrong expected pixel in the boundary test**
- **Found during:** Task 2 GREEN
- **Issue:** The test expected GG (u=15, v=40) to be black. The checker rule `((u^v)&1)` gives white there (15^40 = 39, which is odd).
- **Fix:** The test point is now (15, 41), which is black under the same rule. The card code was not changed, and the GG hash still matches the oracle.
- **Files modified:** tests/core/test_core.c
- **Commit:** bc412f4

**2. [Rule 1 - Bug] The pitch_wide test overran its buffer**
- **Found during:** Task 3 RED
- **Issue:** 192 rows at a 672-byte pitch need 129,024 bytes. The test passed its 122,880-byte field buffer.
- **Fix:** The test now has a dedicated 336x240 buffer.
- **Files modified:** tests/core/test_core.c
- **Commit:** ad1eff6

### Interpretations and additions

- **"No magenta pixel" in the GG crop:** the GG chart holds every RGB444 value, so it contains one 2x2 magenta cell (0x0F0F). `core.testcard.full_field` checks instead that the crop equals the field window pixel for pixel and holds exactly those 4 magenta dots. That proves no surround pixel is in the crop.
- **get_info before load** returns ERR_NO_CONTENT. The plan names only run_frame and get_frame, but the info describes the loaded machine. Plan 02's error-path list already covers get_info. This is tested in `core.api.statuses`.
- **Pitch cap:** `pitch_bytes` above 65536 returns ERR_ARGUMENT. This comes from the seed, keeps row offsets inside a 32-bit `size_t`, and is documented and tested.
- **Interim statuses in Task 1:** before ERR_NO_MEMORY, ERR_NO_CONTENT, ERR_BUFFER_TOO_SMALL and ERR_PROFILE_REJECTED existed, those paths returned ERR_ARGUMENT. Tasks 2 and 3 replaced them, as the plan's rule that a constant enters with its behaviour requires. The interim state appears only in intermediate commits on the branch.
- **Generated version header:** it carries only the three numeric macros. The seed's version-string macro was dropped because the header carries no declaration without a reader.
- **Runner exit table:** `exit_status.h` holds the enum only, named ok, failure and usage. The printable table arrives with `--help`.
- **Embedding guide scope:** it describes `add_subdirectory` and `cmake --install` only. Release archives and the CMake package config arrive with packaging, and the guide does not describe them before they exist.

## Requirement status

`requirements-completed` lists FRAME-01 and FRAME-02, the plan's requirements, because this plan delivers its share of each. Neither box in `.planning/REQUIREMENTS.md` is ticked yet, because the parts that remain belong to later plans in this phase: packaging, the libretro core, and the same hash on all six platforms plus `be32` and `wasm32`. Plans 02–04, 08, 09, 11 and 12 also list these requirements.

## Known Stubs

None. Loading only the test card is the documented behaviour of this release, not a stub.

## Issues Encountered

None beyond the two test bugs above.

## Next Phase Readiness

The instance layout (`struct cogmaster` in `src/internal.h`) and the size-tag helpers are ready for the state visitor and the first-fault record. The output-struct helper `cogmaster__check_out_size` takes a first-released constant, so the fault struct can reuse it.

## Self-Check: PASSED

- All 25 created files exist on disk.
- Commits a5ab3e9, 9e7beb2, bc412f4, ad1eff6 and c53bae9 are ancestors of HEAD.
