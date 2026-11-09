---
phase: 04-sound
plan: "03"
subsystem: audio-core
tags: [audio, api-9, fm-profile, state-format, c17]

requires:
  - phase: 04-sound
    provides: API 9, the cadence, the drain and the trace log (plan 04-02)
provides:
  - cogmaster_media.fm (COGMASTER_FM_AUTO, OFF, ON) with reserved2, resolved at load into the instance's fm_unit and fm_mask
  - cogmaster_info.audio_channels and cogmaster_info.fm, never AUTO
  - state header byte at offset 52 is fm_unit, fixed to the instance, with 11 reserved bytes after it
  - synth_io, which saves the cadence phase bounded by the TV modulus, and a size tripwire on struct cogmaster__synth
  - cogmaster__sio_i64, a signed field through the unsigned visitor by a 2^63 bias
  - abi.api_v9 registered in the ci, wasm32 and nofp inventories
  - docs/audio.md, the host audio contract
affects: [04-04, 04-05, 04-06, 04-12, 04-14]

actuals:
  tokens: 11300
  tasks: 3
  commits: 4

tech-stack:
  added: []
  patterns:
    - "A load-time profile field is read with tagged_field and resolved against the resolved machine and region before any instance state changes"
    - "A signed state field is stored as x xor 2^63, so bounds compare as unsigned and no signed shift is needed"
    - "A state loaded into an instance drops that instance's undrained PCM, so the dropped counter does not depend on what it held"

key-files:
  created:
    - docs/audio.md
  modified:
    - include/cogmaster.h
    - src/internal.h
    - src/instance.c
    - src/state.c
    - src/state.h
    - tests/api/v9.c
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/abi/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/nofp.txt
    - docs/embedding.md

key-decisions:
  - "fm resolves at load: AUTO attaches the unit only for resolved region Japan on a non-GG machine, ON attaches on any Master System, OFF never; fm above 2 or ON with a Game Gear is ERR_PROFILE_REJECTED before any state changes"
  - "fm_mask is plain data on the instance (3 with the unit, else 0) so a later test can set 1 for the FM-unit-only mask"
  - "The state header byte at offset 52 is fm_unit written with fixed(), leaving 11 reserved bytes from offset 53"
  - "synth_io saves only the cadence phase (range 0 to modulus minus 1 for the instance's TV); the PCM and trace counters are the host's queue, and a state load clears the undrained PCM"
  - "cogmaster__sio_i64 biases by xor with 2^63 and converts back without an out-of-range cast, so it uses no signed shift"

patterns-established:
  - "Test cases that reuse the shared state buffers (good, bad, before, after) run after the checks that depend on them"

requirements-completed: [SND-02, SND-03, SND-04]

coverage:
  - id: D1
    description: "media.fm resolution at load: AUTO only for Japan on a non-GG machine, OFF never, ON on export SMS; values above 2 and ON with a Game Gear are refused with the instance unchanged; fm_unit and fm_mask set on the instance"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "ctest core.load.profiles and core.load.profile_rejected (tests/core/test_core.c)"
        status: pass
    human_judgment: false
  - id: D2
    description: "cogmaster_info reports audio_channels (2 on the Game Gear in either video mode, else 1) and the resolved fm; an API 8 info tag still succeeds"
    requirement: SND-04
    verification:
      - kind: unit
        ref: "ctest core.info and core.api.first_released_sizes (tests/core/test_core.c)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Public layout: media is 56 bytes with reserved2 at 44 and fm at 48 on 64-bit, 32 bytes with fm at 28 on 32-bit; info is 68 bytes; an API 8 media tag reads fm as AUTO"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "ctest core.api.first_released_sizes, abi.api_v9, abi.api_version, docs.api_names"
        status: pass
    human_judgment: false
  - id: D4
    description: "State header fm_unit byte at offset 52 fixed to the instance; a flipped byte and a state from an instance with the other FM choice are refused with nothing changed; 11 reserved bytes remain"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "ctest core.state.rejects_header (tests/core/test_state.c)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Synth phase in the state: restored on load, bounded 0 to modulus minus 1 for the instance's TV (both edges load, the modulus is refused), every recorded field swept; signed helper round-trips INT64_MIN through INT64_MAX with signed bounds"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest core.state.round_trip and core.state.record_sweep (tests/core/test_state.c)"
        status: pass
    human_judgment: false
  - id: D6
    description: "abi.api_v9 compiles tests/api/v9.c and is registered in the ci, wasm32 and nofp inventories"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest abi.api_v9 (build/ci)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/audio.md states the cadence, channels, written, dropped, the trace log with the tick restart at load and both resets, the fm field and the reset behaviour, and holds no planning identifier"
    requirement: SND-04
    verification:
      - kind: other
        ref: "ctest docs.budgets and docs.api_names pass; grep for planning identifiers in docs/audio.md finds none"
        status: pass
    human_judgment: true
    rationale: "That the prose is accurate and readable against the code is a judgment the agent made by reading the guide beside src/synth.c; no test asserts the wording."
  - id: D8
    description: "The full ci and hygiene workflows pass with the new fields, state layout, test registration and guide"
    requirement: SND-03
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci then cmake --workflow --preset hygiene: both exit 0"
        status: pass
    human_judgment: false

duration: 7min
completed: 2026-11-09
status: complete
plan_head_before: ecba7166693358024a3b0d7bd6d6cbd67950ca96
plan_head_after: 19552bc4374326782d507238cac282e8e8819ab2
commits: 4
---

# Phase 4 Plan 03: FM profile field, info fields, state header byte and host audio guide Summary

**API 9 completed: media.fm chooses the YM2413 at load, info reports audio channels and the resolved FM unit, the state carries the fm_unit header byte and a bounded synth phase with a signed visitor helper, abi.api_v9 runs in every abi lane, and docs/audio.md states the host contract.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-11-09T19:48:24Z
- **Completed:** 2026-11-09T19:55:28Z (approximate)
- **Tasks:** 3
- **Files modified:** 14 (13 modified, 1 created)

## Accomplishments

- `cogmaster_media` gains `reserved2` (64-bit only) and `fm`; `cogmaster_info` gains `audio_channels` and `fm`; every offset and size is asserted for both pointer sizes in `src/internal.h` and named in `tests/api/v9.c` and `docs/embedding.md`.
- `cogmaster_load` reads `fm` through `tagged_field`, so an API 8 size tag reads AUTO, refuses a value above 2 or ON with a Game Gear before anything changes, and resolves `fm_unit` and `fm_mask` onto the instance.
- State header offset 52 is `fm_unit` (fixed per instance), 11 reserved bytes follow; `synth_io` saves the cadence phase bounded by the TV modulus; a size tripwire on `struct cogmaster__synth` fails the build when a field is added without a visitor decision; `cogmaster__sio_i64` is ready for plan 04-04.
- `abi.api_v9` builds `tests/api/v9.c` and is registered in `ci`, `wasm32` and `nofp`.
- `docs/audio.md` (new) holds the host contract; `docs/embedding.md` points to it and stays at 40,385 bytes of 40,960.

## Task Commits

1. **Task 1 (tracer): media.fm resolves the FM unit at load and info reports it with the audio channels** - `c5ab3c8` (feat)
2. **Task 2 (tdd): the fm_unit state header byte, the signed visitor helper and the synth visitor** - RED `b2f1e90` (test), GREEN `3c7281a` (feat)
3. **Task 3: abi.api_v9 in every abi lane and the host-facing audio guide** - `19552bc` (feat)

**Plan metadata:** recorded in the following docs commit.

## Files Created/Modified

- `include/cogmaster.h` - FM constants, `media.reserved2` and `media.fm`, `info.audio_channels` and `info.fm`
- `src/internal.h` - media and info layout asserts, instance `fm_unit` and `fm_mask`
- `src/instance.c` - fm validation and resolution at load, info fields
- `src/state.c` - header comment corrected (video mode at 27, FM unit at 52), `fm_unit` byte, `synth_io`, synth tripwire
- `src/state.h` - `cogmaster__sio_i64` with its bias helpers
- `tests/core/test_core.c`, `tests/core/test_state.c` - extended cases (no new test names)
- `tests/api/v9.c` - names every new header name
- `tests/abi/tests.cmake`, `tests/inventory/{ci,wasm32,nofp}.txt` - `abi.api_v9`
- `docs/audio.md` (new), `docs/embedding.md` - the guide and the names, sizes and pointer

## Decisions Made

See `key-decisions`. The state load drops undrained PCM because the library counts a frame's untaken samples into the saved `audio_dropped` at the next frame end; two instances that loaded the same state would otherwise differ by what the earlier of them had pending.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] A state load discards undrained PCM**
- **Found during:** Task 2 (synth visitor)
- **Issue:** The synth phase is the only saved audio field, but the next frame end adds the untaken PCM of the loading instance to the saved `audio_dropped` counter, so the same state loaded into two instances could diverge by their pending output. The audio design for the state says undrained output is cleared on load.
- **Fix:** `synth_io` sets `pcm_frames` and `pcm_read` to 0 on APPLY only; the trace counters stay (the trace log is cleared by load and reset, not by a state load, a choice plan 04-04 may revisit).
- **Files modified:** src/state.c
- **Verification:** core.state.round_trip and the full ctest (348 of 348) pass
- **Committed in:** `3c7281a`

**2. [Rule 1 - Test bug] The new fm_unit state case clobbered the shared buffers**
- **Found during:** Task 2 (GREEN run)
- **Issue:** `reject_other_fm` was first called in the middle of `test_rejects_header`, overwriting `before` and `after` that later `expect_refused` calls rely on.
- **Fix:** the call moved to after `reject_image_states`, the last user of the shared buffers.
- **Files modified:** tests/core/test_state.c
- **Verification:** core.state.rejects_header passes
- **Committed in:** `3c7281a`

---

**Total deviations:** 2 auto-fixed (1 missing critical, 1 test bug)
**Impact on plan:** none on scope. Plan 04-04 extends `synth_io` and the tripwire as the plan states.

## TDD Gate Compliance

The plan is type `execute` and `workflow.tdd_mode` is off; the one `tdd="true"` task (Task 2) ran RED then GREEN.

- **RED `b2f1e90`.** Command `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^core\.state\.'`, non-zero exit: `core.state.round_trip` failed on `b->synth.phase == a->synth.phase` (for example 1233 against 2466), and `core.state.record_sweep`, `core.state.rejects_header` and `core.state.mapper` reported `no state field named fm_unit` and `no state field named synth_phase`. The binary built and the named tests ran, so the failures are the planned assertions, not a build or discovery fault. The format is the project's plain `CHECK` output, which the red-evidence classifier does not parse, so the semantic assessment is the one above.
- **GREEN `3c7281a`.** `core.state.` five of five, then the full ctest 348 of 348.
- The `cogmaster__sio_i64` round-trip case (`i64_round_trip`, run from `core.state.record_sweep`) landed with GREEN rather than RED, because a test naming a function that does not exist yet would fail to compile, which is not a valid RED.
- **Task 1 (tracer):** committed, then its verify command re-run end to end (exit 0) and the full ctest passed (348 of 348); expansion proceeded.

## Issues Encountered

None. `clang-format` 21 is at `/opt/homebrew/opt/llvm@21/bin/clang-format`; the hygiene workflow finds it by itself.

## Verification

Run from the repository root; exit statuses only, no logs committed.

- Task 1 verify (`cmake --build --preset ci && ctest ... -R '^(abi\.api_version|abi\.symbols|core\.|docs\.api_names|docs\.budgets)'`): exit 0.
- Task 2 verify (`ctest ... -R '^core\.state\.'`): exit 0, 5 of 5; `grep -Eq 'H_RESERVED 53u' src/state.c`: exit 0.
- Task 3 verify (`cmake --preset ci && cmake --build --preset ci && ctest ... -R '^(abi\.|docs\.)'`): exit 0, 26 of 26; `ctest -N -R '^abi\.api_v9$'` lists one test; `grep -c '^abi.api_v9' tests/inventory/nofp.txt` prints 1.
- `cmake --workflow --preset ci`: exit 0 (345 of 345 tests, then 4 of 4).
- `cmake --workflow --preset hygiene`: exit 0 (38 of 38 tests).
- The CI nofp lane (GCC with `-mgeneral-regs-only`) was not run here; the inventory row follows the other abi rows and the test is the same executable the other lanes build.

## Known Stubs

None added. The silent samples in `src/synth.c` remain the stub plan 04-02 recorded in `.planning/WINDOWS.md`; the PSG and FM plans replace them.

## Threat Flags

None. The two boundaries in the plan's threat model are mitigated and tested: `media.fm` (values above 2 and ON with a Game Gear refused before any change, `core.load.profile_rejected`) and the state header `fm_unit` byte and synth phase (fixed and range-checked, refused states change nothing, `core.state.rejects_header` and `core.state.record_sweep`).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for 04-04, which adds the synthesis, extends `synth_io` and the tripwire through `cogmaster__sio_i64`, and extends `docs/audio.md` with the synthesis section. SND-02, SND-03 and SND-04 stay open in the traceability table until the PSG audio, the FM core and the hash lines exist; this plan lands the profile choice, the info fields, the state layout and the guide they depend on.

## Self-Check: PASSED

Created files found: docs/audio.md. Commits found on the branch: c5ab3c8, b2f1e90, 3c7281a, 19552bc.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
