---
phase: 03-games-play-in-retroarch
plan: "08"
status: complete
subsystem: game-gear-video
tags:
  - game-gear
  - rgb444
  - lcd
  - input
  - state
requires:
  - phase: 03-07
    provides: Shared completed-field renderer with latched heights and bounded deadlines
provides:
  - Explicit native GG and SMS video modes independent of machine identity
  - Native palette latch, crop and independently held LCD image
  - Runner overrides and owned GG Start-driven playing fixture
affects:
  - 03-09
  - 03-10
  - 03-11
tech-stack:
  added: []
  patterns:
    - Preserve previous media tail padding before appending a size-tagged field
    - Hold native LCD output separately from the shared live diagnostic field
    - Share assembly and independent pixel arithmetic across SMS and GG fixtures
key-files:
  created:
    - tests/vdp/test_gg.c
    - tests/api/v5.c
    - tests/roms/src/boot_to_play_gg.asm
    - tests/roms/src/boot_to_play_gg.link
  modified:
    - include/cogmaster.h
    - src/internal.h
    - src/instance.c
    - src/vdp.c
    - src/vdp_render.c
    - src/frame.c
    - src/io.c
    - src/state.c
    - runner/args.c
    - runner/main.c
    - tests/fixtures/play.c
    - tests/fixtures/tests.cmake
    - tests/roms/src/boot_to_play.asm
    - docs/embedding.md
    - docs/hardware/video.md
    - docs/runner.md
    - docs/conformance.md
key-decisions:
  - Append video_mode beyond the previous media size including 64-bit tail padding; expose the resolved mode in info.
  - Retain the last native LCD pixels and descriptor until POWER reset while full-field rendering continues.
  - Keep GAME-03 pending until plan 03-09 demonstrates the required hosted GG screenshot equality.
requirements-completed: []
coverage:
  - id: native-gg-palette
    description: All 4096 native colours, even-byte latch and odd-byte commit, address changes and wrap, immediate SMS colours and immutable raster intervals
    requirement: GAME-03
    verification:
      - kind: unit
        ref: vdp.gg.palette
        status: pass
      - kind: unit
        ref: vdp.gg.colours
        status: pass
      - kind: unit
        ref: vdp.gg.intervals
        status: pass
    human_judgment: false
  - id: native-lcd-and-full-field
    description: Exact 192 and 224 crop coordinates, shared pixel identity, frozen native image versus live full field, SOFT retention and POWER recovery
    requirement: GAME-03
    verification:
      - kind: unit
        ref: vdp.gg.coordinates
        status: pass
      - kind: unit
        ref: vdp.gg.freeze
        status: pass
      - kind: unit
        ref: vdp.gg.state
        status: pass
      - kind: unit
        ref: core.state.record_sweep
        status: pass
    human_judgment: false
  - id: explicit-mode-and-controller-decode
    description: Previous media sizes select AUTO, impossible modes reject, resolved mode is visible, only selected aliases and native Start qualify, SMS-mode Start routes to Pause
    requirement: GAME-04
    verification:
      - kind: unit
        ref: abi.api_v5
        status: pass
      - kind: unit
        ref: vdp.gg.profiles
        status: pass
      - kind: unit
        ref: vdp.gg.ports
        status: pass
      - kind: integration
        ref: runner.load.video_override
        status: pass
    human_judgment: false
  - id: owned-native-playing-fixture
    description: Real GG-header image at boot_to_play.gg, every native LCD pixel and digest checked independently, Start baseline and press/hold/release/repress counters and raw bytes judged
    requirement: GAME-03
    verification:
      - kind: integration
        ref: fixtures.boot_to_play.gg.baseline
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.gg.start
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.gg.mailbox.baseline
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.gg.mailbox.start
        status: pass
    human_judgment: false
  - id: both-sms-pad-playing-paths
    description: Either pad drives the same independently checked counter, scroll and picture sequence; released card hashes remain unchanged
    requirement: GAME-01
    verification:
      - kind: integration
        ref: fixtures.boot_to_play.input
        status: pass
      - kind: integration
        ref: fixtures.boot_to_play.pad2
        status: pass
      - kind: integration
        ref: runner.hash.sms
        status: pass
      - kind: integration
        ref: runner.hash.gg
        status: pass
    human_judgment: false
actuals:
  tokens: 17102
  tasks: 2
  commits: 4
plan_head_before: 37d3e8f5fc6e7f42866cb231043bf15be70a0c6d
plan_head_after: c35795dfe1d5bb38a7344514107017afde994a11
duration: 9min
completed: 2026-10-20
---

# Phase 03 Plan 08: Native Game Gear and explicit SMS video Summary

Native GG shares the Mode 4 renderer with SMS, commits RGB444 palette words through the hardware latch, and preserves a frozen LCD independently of the live full field.

## Accomplishments

- API v5 appends media and info video mode without reading old media padding; behavior revision 8 and state format 9 describe the new model.
- Native crops match their field rectangles at both supported LCD heights. The selected 240-line model holds pixels, crop origin, height and backdrop across later fields and SOFT reset; POWER recovers.
- Native GG decodes C0/C1 alongside DC/DD and Start at 00. Explicit SMS mode uses immediate six-bit colours, full SMS geometry/PAR and Start-to-Pause edges.
- Runner accepts `--machine sms1` and `--video-mode auto|sms|gg`, rejecting impossible combinations. The shared-source owned `boot_to_play.gg` emits independently checked LCD pixels, counter/scroll values and raw Start samples. Either SMS pad separately proves the same edge sequence.

## Task Commits

1. Task 1 RED: `70e7dad` — native palette commit assertions.
2. Task 1 GREEN: `7011a64` — core mode, palette, LCD, ports, state, API and documentation.
3. Task 2 RED: `277f32a` — runner profile override acceptance and refusal assertions.
4. Task 2 GREEN: `c35795d` — runner overrides, shared native fixture, both-pad oracle and documentation.

## Validation

Commands ran from the repository root; local checks have no hosted run URL.

- `cmake --workflow --preset ci` — exit 0; 282 CI tests and four release archive/floor tests passed after the final changes.
- `cmake --workflow --preset hygiene` — exit 0; all 32 checks passed, including format, analyzer, source policy, strict repository budget and the 300-line hosted helper budget.
- Task 1 tracer recheck: `cmake --workflow --preset ci` — exit 0 before expansion.
- `git diff --check` — exit 0.

No new hosted run was needed by this plan. Plan 03-09 adds the GG screenshot comparison and exact-SHA portable proof. GAME-03 remains pending for that requirement; GAME-01 and GAME-04 retain their broader phase criteria.

## TDD Gate Compliance

Both target tests compiled and executed before implementation. `vdp.gg.palette` failed on expected odd-write RGB444 values (zero instead of 5AB/867/289); `runner.load.video_override` failed because sms1 exited 2 instead of 0. Actual CTest JUnit reports were classified with `check tdd-red-evidence`; both returned RED_EVIDENCE_OK. Semantic inspection confirmed planned behavior assertions rather than setup failures. Their RED commits precede the GREEN commits. No separate refactor was necessary.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Critical] Preserve prior media layout and exercise the appended API version.**
- Added a reserved 64-bit padding word before the new media field so a previous 40-byte caller cannot accidentally select a mode from tail padding. Added required `tests/api/v5.c` and ABI registration alongside the planned v3 test update.
- Commit: `7011a64`.

**2. [Rule 3 - Blocking] Extend existing state and native-palette test infrastructure.**
- Increased the static state-field inventory capacity for the saved LCD pixels and sampled that pixel domain with the existing boundary sweep strategy. Updated the old GG sprite fixture to native CRAM and mirror-read counts for the newly sourced aliases. These companion files are necessary for the new behavior's existing tests.
- Commit: `7011a64`.

**3. [Rule 3 - Blocking] Extend the shared pixel oracle and avoid a source-policy citation false positive.**
- Updated `tests/fixtures/play.c` for native crop coordinates, raw Start records and the second SMS pad. Reworded a C comment section citation that the lexical no-float check interpreted as a decimal literal. Kept all hosted helpers unchanged.
- Commit: `c35795d`.

## Decisions Made

Use explicit video mode independent of machine identity; physical LCD resampling stays with the host. Keep the confidence-qualified LCD freeze model deterministic without promoting it to measured hardware accuracy. Preserve the generic unknown-homebrew/port-decode uncertainty for later evidence.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: schema | include/cogmaster.h | New size-tagged mode input is range/profile validated before instance mutation and old tags read AUTO. |
| threat_flag: state | src/state.c | Saved palette latch and held LCD schema use matching-mode validation, RGB444 bounds and validate-before-apply. |

## Next Plan Readiness

Plan 03-09 can use `build/ci/fixtures/boot_to_play.gg` and the native Start input schedule. The owned wrapper remains `boot_to_play_gg.asm`; its built-image digest row is `boot_to_play_gg`. No dependency, ROM bytes, new external source, skipped verification or implementation stub was introduced.

## Self-Check: PASSED

All four created files and this summary exist. All four listed task commits are ancestors of HEAD. The persisted ledger reports four commits from the recorded base to the recorded task head; the working diff passes `git diff --check`.
