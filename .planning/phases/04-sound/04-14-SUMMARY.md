---
phase: 04-sound
plan: "14"
subsystem: libretro-audio
tags: [libretro, audio-sessions, batch-callback, single-sample-callback, cogmaster_fm, core-options]

requires:
  - phase: 04-sound
    provides: the adapter drain and cadence (plan 04-02), psg_sweep and boot_to_play audio hashes (plans 04-07 to 04-09), the FM unit and fm_sweep (plans 04-12 and 04-13)
provides:
  - host mode audio, three callback registrations per image, judged against the runner's own --hash-audio hash in the same test
  - libretro.host.audio.psg_sweep.sms, libretro.host.audio.psg_sweep.gg, libretro.host.audio.boot_to_play.sms and libretro.host.audio.boot_to_play.gg
  - cogmaster_fm (auto, off, on) in the v2, v1 and v0 option tables with a compile-time count check
  - libretro.host.fm_option, the option's three values against the runner on the Japanese, export and Game Gear profiles
  - the .info description and README say the core has sound
affects: [04-15]

actuals:
  tokens: 5400
  tasks: 2
  commits: 3
plan_head_before: 144cd352abebfb10e63cc4690997be866cc11b25
plan_head_after: 4236ca314d3e1b06f19bbb37cde7a2bdc5f8cd5f

tech-stack:
  added: []
  patterns:
    - "A libretro audio proof registers the frontend callbacks per session, copies each batch at call time and hashes little-endian bytes built byte by byte, so one digest means the same on every byte order"
    - "A host mode that prints a hash and a CMake case that compares it with the runner's, so no expected hash is committed for a core test"

key-files:
  created:
    - tests/libretro/audio.h
  modified:
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/inventory/ci.txt
    - libretro/libretro.c
    - libretro/options.h
    - libretro/cogmaster_libretro.info.in
    - README.md
    - docs/audio.md
    - docs/testing.md

key-decisions:
  - "The adapter was already correct, so libretro/libretro.c changed only to read cogmaster_fm; the audio sessions found no defect and the mutation runs show they would"
  - "The option table size is one macro, LR_OPTION_COUNT, asserted against the v2 array and the v0 variable array and used as the v1 array size and loop bound, so a seventh option cannot miss a table (the v1 array is 7 and the bound 6, as planned)"
  - "On with Game Gear content leaves media.fm at automatic instead of asking for a unit the console does not have"
  - "The host prints the delivered PCM hash for fm_option and the CMake case judges it, so the equal and differ checks live beside the runner runs"

patterns-established:
  - "The audio and fm_option CASEs of tests/fixtures/tests.cmake run the runner with explicit --machine, --video-mode and --region as the hash matrix does, then call the host"

requirements-completed: [SND-04]

coverage:
  - id: D1
    description: "With only the batch callback, only the single-sample callback and both registered, the module delivers the runner's PCM for psg_sweep and boot_to_play on the Master System (mono as equal left and right) and the Game Gear (interleaved), every frame holding 801 or 802 stereo frames with running totals equal to floor(frames x 3,504,512 / 4,375), per-channel hashes equal across the three sessions, and the single-sample callback never called in the both session"
    requirement: SND-04
    verification:
      - kind: integration
        ref: "ctest libretro.host.audio.psg_sweep.sms, libretro.host.audio.psg_sweep.gg, libretro.host.audio.boot_to_play.sms, libretro.host.audio.boot_to_play.gg"
        status: pass
    human_judgment: false
  - id: D2
    description: "With no audio callback registered the module still drains the library, so the next frame holds a normal count and not a backlog"
    requirement: SND-04
    verification:
      - kind: integration
        ref: "ctest libretro.host.audio.* (audio_neither phase)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The cogmaster_fm option: automatic on a Japanese console equals the runner's Japanese hash, off differs from it, automatic on an export console equals the export hash, on there differs from it, and on with Game Gear content loads and equals the runner's Game Gear hash"
    requirement: SND-04
    verification:
      - kind: integration
        ref: "ctest libretro.host.fm_option"
        status: pass
    human_judgment: false
  - id: D4
    description: "The v2, v1 and v0 option tables hold the same number of options, checked when the module compiles"
    requirement: SND-04
    verification:
      - kind: other
        ref: "_Static_assert in libretro/options.h, compiled by cmake --build --preset ci: exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The .info description and README say the core has sound; savestate stays false and is_experimental stays true, and every .info claim still maps to a test"
    requirement: SND-04
    verification:
      - kind: integration
        ref: "ctest libretro.info_claims and docs.budgets"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-11-09
status: complete
---

# Phase 04 Plan 14: The libretro Audio Sessions and the FM Option Summary

**An in-process host that plays psg_sweep and boot_to_play on SMS and GG through the libretro module with batch-only, single-only and both callbacks and requires the runner's exact PCM each time, plus the cogmaster_fm option (auto, off, on) in all three option tables.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-11-09T22:19:03Z
- **Completed:** 2026-11-09T22:28:00Z
- **Tasks:** 2
- **Files modified:** 11 (one created, ten changed)

## Accomplishments

- `tests/libretro/audio.h` adds the host mode `audio <image> <frames> sms|gg <sha256>`. It loads the image three times and registers the callbacks per session. Each batch is copied at call time. Every sample is hashed as little-endian bytes built byte by byte, mono on the Master System and interleaved on the Game Gear, with left and right also hashed apart. A session requires 801 or 802 frames each, a running total equal to `floor(f x 3,504,512 / 4,375)` after every frame, exactly one non-empty batch call a frame when the batch callback is registered, no single-sample call in the both session, left equal to right on the Master System, the expected hash and equal per-channel hashes across the sessions. A fourth phase registers neither callback for ten frames and then the batch one, and requires a normal frame count.
- The four `libretro.host.audio.*` tests take their expected hash from `cogmaster-run --hash-audio` in the same test (`CASE=audio` in `tests/fixtures/tests.cmake`, a tolerant regex on the `audio-pcm` line); no hash is committed. They pass on the first run, so `libretro/libretro.c` needed no fix.
- `cogmaster_fm` joins the three option tables. `LR_OPTION_COUNT` sizes the v1 array (7) and its loop bound (6), and `_Static_assert` checks the v2 and v0 tables against it. `retro_load_game` sets `media.fm` to off or on, and leaves automatic when on meets a Game Gear.
- `libretro.host.fm_option` compares the delivered PCM under each value with the runner's hashes: automatic on `ntsc-j` equals the Japanese run, off differs from it, automatic on `ntsc-u` equals the export run, on there differs, and on with `psg_sweep.gg` loads and equals the runner's Game Gear run.
- The `.info` description names the sound, the README line about silent audio is replaced, `docs/audio.md` states the option and its mapping, and `docs/testing.md` (40,718 of 40,960 bytes) describes the audio sessions and the option test.

## Task Commits

1. **Task 1: Batch, single and both-registered sessions deliver the runner's PCM (tracer)** - `4873809` (test)
2. **Task 2, RED: a failing test for the cogmaster_fm option** - `3f5c8e0` (test)
3. **Task 2, GREEN: the cogmaster_fm option, the .info description and the README status** - `4236ca3` (feat)

**Plan metadata:** the commit that adds this summary and the state and roadmap updates.

## TDD Gate Compliance

Task 2 carries `tdd="true"` inside a plan of `type: execute`, so the gate is advisory. The sequence is present: RED `3f5c8e0` precedes GREEN `4236ca3`; no refactor was needed.

- **RED:** `libretro.host.fm_option` ran against the unchanged adapter with `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^libretro\.host\.fm_option'`, exit 8, and printed `fm off on ntsc-j still gives the Japanese run's hash 695a51c6...`. Semantic assessment: the target test ran, its two earlier checks (runner hashes differ, automatic equals the Japanese run) passed, and it failed on the planned assertion that off must change the hash, because the adapter ignored the option; not a load, parser or fixture fault.
- **GREEN:** the same command exits 0 after the adapter reads the option. A scratch change that dropped the Game Gear mapping failed the test with `the module refused the content with fm on`; the restored source passes.

## Measured values and commands

Every command ran from the repository root.

| Check | Command | Result |
|---|---|---|
| Task 1 verify | `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^libretro\.'` | exit 0 (24 of 24; the audio tests take 0.1 to 0.3 s each) |
| Tracer gate | the Task 1 verify re-run before Task 2 (automated-only verify, end-of-phase mode) | all passed, expanded |
| Mutation, audio | the adapter made to swap left and right on the single path and to call both callbacks | the four audio tests failed, naming the hash and the single-sample call in the both session; restored, they pass |
| Task 2 verify, tests | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(libretro\.\|docs\.budgets)'` | exit 0 (31 of 31) |
| Task 2 verify, workflow | `cmake --workflow --preset ci` | exit 0 (399 of 399 plus 4 of 4) |
| Hygiene | `cmake --workflow --preset hygiene` | exit 0 (38 of 38) |

The be32 and wasm32 legs were not run here (no `qemu-ppc` and no pinned wasmtime); the libretro tests are not in the wasm32 inventory. The four audio tests and `libretro.host.fm_option` take the 10 second floor (0.1 to 0.9 s here, 7.3 times that at worst under `qemu-ppc`); the hosted lanes will show whether it holds.

## Files Created/Modified

- `tests/libretro/audio.h` - the audio sessions, the neither-callback phase and the `fm_option` host mode
- `tests/libretro/host.c` - the `cogmaster_fm` value in the environment stub and the two mode dispatches
- `tests/libretro/tests.cmake`, `tests/fixtures/tests.cmake`, `tests/inventory/ci.txt` - the five tests and the `audio` and `fm_option` cases
- `libretro/libretro.c`, `libretro/options.h` - the option, its three tables and the count check
- `libretro/cogmaster_libretro.info.in`, `README.md`, `docs/audio.md`, `docs/testing.md` - the user-facing and maintainer text

## Decisions Made

See key-decisions. The audio proof needed no adapter change; only the option touched `libretro/libretro.c`.

## Deviations from Plan

None - plan executed exactly as written. Two choices inside the plan's latitude: the option count is a macro shared by the three tables (the plan's `definitions[7]` and bound 6 follow from it), and the plan's "stale silence-test wording" in `docs/testing.md` did not exist in its libretro section, so none was removed (the test card's silence is still asserted by `libretro.host.xrgb8888`, which stays true).

**Total deviations:** 0
**Impact on plan:** none.

## Issues Encountered

None. `docs/testing.md` had 898 bytes of headroom, not the 1,650 the brief expected; the two paragraphs added take 656 bytes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The in-process half of SND-04 is proven; plan 04-15 can run the released RetroArch against the same sound fixtures.
- The v1 and v0 option tables are only compile-checked (the host exercises v2), as the research notes.
- No stubs, no skipped tests and no new ledger entries.

## Self-Check: PASSED

`tests/libretro/audio.h` exists; commits 4873809, 3f5c8e0 and 4236ca3 are ancestors of HEAD; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
