---
phase: 01-a-test-frame-in-retroarch
plan: 05
subsystem: libretro
tags:
  - libretro
  - adapter
  - vendored
  - info
requires:
  - "01-01: cogmaster_get_info, cogmaster_get_frame crop, cogmaster_add_test and the inventory check"
  - "01-03: cogmaster-host (cogmaster__rgb444_to_rgb888, cogmaster__rgb444_to_rgb565)"
  - "01-01/01-02: runner/sha256.c and the pinned card hashes in tests/testcard_pins.cmake"
provides:
  - "libretro/libretro.h, vendored unmodified from RetroArch v1.22.2, with its notice in THIRD-PARTY-NOTICES.md and its digest in PROVENANCE §8"
  - "Module cogmaster_libretro (installed to cores/, .info to info/); test-only modules cogmaster_libretro_rgb565 and cogmaster_libretro_gg"
  - "Option COGMASTER_BUILD_LIBRETRO (top level by default, OFF under WASI); compile definitions COGMASTER_LIBRETRO_RGB565 and COGMASTER_LIBRETRO_TEST_GG"
  - "build/<preset>/libretro/cogmaster_libretro.info with display_version from version.txt"
  - "tests/cmake/vendored.cmake and tests/cmake/info_claims.cmake"
  - "cogmaster-test-libretro-host <module> xrgb8888|rgb565 <sha256> W H AW AH"
  - "CTests libretro.vendored, libretro.info_claims, libretro.info_claims.self_test, libretro.host.xrgb8888, libretro.host.rgb565, libretro.host.gg"
affects:
  - "hosted RetroArch screenshot compare (loads cogmaster_libretro with no content)"
  - "packaging and release archives (libretro zip holds the module and the .info)"
  - "hygiene.format, which must exclude libretro/libretro.h"
  - "content loading, which adds supported_extensions with the test that loads them"
actuals:
  tokens: 11800
  tasks: 3
  commits: 3
plan_head_before: 39f291dd6f9c2df1c0c0f0583e37ba63dcb88405
plan_head_after: d55645924b24902046e8dd769b6322b6f1c78664
tech-stack:
  added:
    - "libretro.h (RetroArch v1.22.2, MIT, vendored)"
  patterns:
    - "The adapter reads every rate and size from cogmaster_get_info; its only floating point is the av_info division"
    - "Audio length is an exact rational carried as a remainder: 48000 x fps_den / fps_num stereo frames per frame"
    - "A test-only build of the same adapter source, selected by one compile definition, covers a second pixel format or machine"
    - "The in-process host hashes the delivered frame against the same pinned constant as the runner, so no second rendering path exists"
key-files:
  created:
    - libretro/libretro.h
    - libretro/libretro.c
    - libretro/CMakeLists.txt
    - libretro/cogmaster_libretro.info.in
    - THIRD-PARTY-NOTICES.md
    - tests/cmake/vendored.cmake
    - tests/cmake/info_claims.cmake
    - tests/libretro/tests.cmake
    - tests/libretro/host.c
  modified:
    - CMakeLists.txt
    - PROVENANCE.md
    - tests/tests.cmake
    - tests/inventory/ci.txt
decisions:
  - "One instance lives from retro_init to retro_deinit; retro_load_game reloads it and allocates the frame buffers at the largest field; retro_unload_game frees the buffers only"
  - "The audio remainder uses the unreduced rational 48000 x fps_den over fps_num from cogmaster_get_info, so no second copy of the rate exists; it gives the same 801/802 sequence as 3,504,512/4,375"
  - "cogmaster-host moved out of the runner block: it builds when the runner or the core is on, position-independent with hidden visibility"
  - "The library version reaches retro_get_system_info through the compile definition COGMASTER_LIBRETRO_VERSION, set from PROJECT_VERSION"
  - "The host test takes the pinned hash, the crop and the display aspect as arguments, so the SMS and GG cases share one binary"
requirements-completed:
  - FRAME-03
coverage:
  - id: D1
    description: "libretro.h is vendored byte for byte from RetroArch v1.22.2 (SHA-256 bd3398d2...947acb), with its MIT notice and a PROVENANCE §8 row; libretro.vendored fails naming the file when the digest differs"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/cmake/vendored.cmake#libretro.vendored"
        status: pass
      - kind: other
        ref: "cmake -DPROVENANCE=build/ci/prov_bad.md -P tests/cmake/vendored.cmake with one digest digit changed (exit 1, names libretro/libretro.h)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The core exports all 25 retro_* entry points and nothing else; the host resolves each by name before anything else and exits non-zero naming the first missing one"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.xrgb8888"
        status: pass
      - kind: other
        ref: "nm -gU build/ci/libretro/cogmaster_libretro.dylib listed 25 symbols, all retro_* (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "With no content the shipped core shows the SMS card: 60 runs give 60 video callbacks of 256x192, each equal to the first although the host poisons its copy with 0xA5, and frame 60 turned back into RGB444 hashes to the runner's pinned SMS constant, in XRGB8888 and in RGB565"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.xrgb8888"
        status: pass
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.rgb565"
        status: pass
    human_judgment: false
  - id: D4
    description: "av_info reports fps 1640625/27379, sample_rate 48000, base 256x192, max 256x240 and aspect 32/21, all from cogmaster_get_info"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.xrgb8888"
        status: pass
    human_judgment: false
  - id: D5
    description: "Over 4,375 frames the adapter delivers exactly 3,504,512 stereo frames of silence, each frame 801 or 802, through the batch path and the single-sample path"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.xrgb8888"
        status: pass
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.rgb565"
        status: pass
    human_judgment: false
  - id: D6
    description: "The GG test module shows the 160x144 card at aspect 4/3 and the NTSC rate, and its frame 60 hashes to the pinned GG constant"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.gg"
        status: pass
    human_judgment: false
  - id: D7
    description: "retro_serialize_size and retro_get_memory_size return 0, serialize and unserialize return false, retro_load_game with a 1-byte game returns false, and deinit then init and load starts clean"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/libretro/host.c#libretro.host.xrgb8888"
        status: pass
    human_judgment: false
  - id: D8
    description: "The configured .info has supported_extensions empty, license MIT, and only supports_no_game and is_experimental true; every true claim maps to a test in the inventory, and a fixture with savestate true fails naming that claim"
    requirement: FRAME-03
    verification:
      - kind: integration
        ref: "tests/cmake/info_claims.cmake#libretro.info_claims"
        status: pass
      - kind: integration
        ref: "tests/libretro/tests.cmake#libretro.info_claims.self_test"
        status: pass
    human_judgment: false
duration: 6min
completed: 2026-10-19
status: complete
---

# Phase 1 Plan 05: The libretro core with no content Summary

**`cogmaster_libretro` loads with no content and shows the Master System test card. It reports the NTSC rate 1,640,625/27,379 Hz and its geometry from `cogmaster_get_info`, and sends silence of the exact rational length. An in-process host runs it as RetroArch does. The host turns frame 60 back into RGB444 and gets the same pinned hash as `cogmaster-run --frames 60 --hash`, in both pixel formats and for the Game Gear geometry too.**

## Performance

- **Duration:** about 6 min
- **Started:** 2026-10-19T21:40Z
- **Completed:** 2026-10-19T21:46Z
- **Tasks:** 3
- **Files created:** 9, modified: 4

## Accomplishments

- **Vendored header.** `libretro/libretro.h` was fetched with `gh api` at tag v1.22.2 (commit `69a4f0ea1e8a…`). Its digest matched before the commit. `git rev-list --count HEAD -- libretro/libretro.h` prints 1. `THIRD-PARTY-NOTICES.md` holds its MIT text, checked line by line against the file's own header, and PROVENANCE §8 has its row.
- **Adapter.** `libretro/libretro.c` is a thin client of `cogmaster.h`. It takes base size, maximum size, pixel aspect and rate from `cogmaster_get_info` of the loaded machine. The only floating point is the av_info division. The crop is converted with the `host/` palette helpers into a buffer allocated at load, then sent with one video call per frame. Audio is silence: `acc += 48000 × fps_den; n = acc / fps_num; acc %= fps_num`.
- **Test modules.** `cogmaster_libretro_rgb565` covers the 16-bit path and the single-sample audio path. `cogmaster_libretro_gg` loads the Game Gear card with no content. Both are registered only when tests are on, and neither is installed.
- **Claims.** The `.info` lists no extensions and claims only `supports_no_game`, plus the `is_experimental` label. `info_claims.cmake` maps that claim to `libretro.host.xrgb8888`, and its self-test fails on `savestate = "true"`.

## Task Commits

1. **Task 1: Vendor libretro.h with notice, digest and check**: `5be47b1` (chore)
2. **Task 2: Adapter, .info and the claims check**: `081a910` (feat)
3. **Task 3: In-process host for both pixel formats and both machines**: `d556459` (test)

## Evidence (command, exit status)

- `cmake --workflow --preset ci`: exit 0, 53 tests.
- `shasum -a 256 libretro/libretro.h | grep -q '^bd3398d29c3763d18617087020ff56b5450a48a63123403a4b674f8e79947acb '`: exit 0.
- `ctest --test-dir build/ci --no-tests=error -L '^libretro$' && ctest --test-dir build/ci -L '^libretro$' --show-only=json-v1 | jq -e '.tests | length == 6'`: exit 0.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro\.info_claims' && ctest … --show-only=json-v1 | jq -e '.tests | length == 2'`: exit 0.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `grep -E '= "true"' build/ci/libretro/cogmaster_libretro.info | grep -Evq '^(supports_no_game|is_experimental) '`: exit 1, so no other claim is true. The four required fields are present (grep, exit 0).
- `grep -c fps_num libretro/libretro.c`: 5.
- `grep -c 'libretro/libretro.h' PROVENANCE.md THIRD-PARTY-NOTICES.md`: 2 and 1.
- `grep -rEn '\b(D-[0-9]{2}|FRAME-[0-9]{2})\b' libretro/libretro.c libretro/CMakeLists.txt tests/libretro`: exit 1, nothing printed.
- `sh scripts/hygiene.sh --tree`: exit 0.
- One-off negative control, wrong digest: `build/ci/tests/cogmaster-test-libretro-host build/ci/libretro/cogmaster_libretro.dylib xrgb8888 000…000 256 192 32 21` exited 1 and printed `frame 60 sha256 e14d77ce…, expected 000…: the digest differs`.
- One-off mutation controls, each built outside the tree from a modified copy of the adapter. With the audio remainder dropped, the host exited 1 (3504375 != 3504512). With video skipped on frame 30, it exited 1 (`frame 30 differs from frame 1`, 59 != 60 video calls).
- One-off: `nm -gU build/ci/libretro/cogmaster_libretro.dylib` listed exactly 25 global symbols, all `retro_*`.

## TDD Gate Compliance

- Task 3 is `tdd="true"`, but the plan puts the implementation it tests in Task 2, before it. The host test therefore passed on its first run. There was no failing RED commit, and `d556459` is a `test` commit that adds only tests.
- To show the test can fail, three controls were run once instead of a RED run: a wrong digest, an audio remainder that was dropped, and a missing video callback. Each made the host exit 1 with a message naming the fault (see Evidence).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `cogmaster-host` had to build for the libretro core**
- **Found during:** Task 2
- **Issue:** Plan 03 defined `cogmaster-host` inside the `COGMASTER_BUILD_RUNNER` block, without position-independent code. A module linking it needs PIC on Linux, and must still build with the runner off.
- **Fix:** It now builds when `COGMASTER_BUILD_RUNNER OR COGMASTER_BUILD_LIBRETRO`, with `POSITION_INDEPENDENT_CODE ON` and hidden visibility.
- **Files modified:** CMakeLists.txt
- **Commit:** `081a910`

### Interpretations and additions

- **One commit not green.** At `081a910`, `libretro.info_claims` and its self-test failed. The test that backs `supports_no_game`, `libretro.host.xrgb8888`, could not be listed in the inventory until Task 3 registered it. `d556459` made the tree green, and the commit body says so.
- **Rate source.** The audio remainder uses `48000 × fps_den / fps_num` from `cogmaster_get_info`, not the literal `3,504,512 / 4,375`. The sequence of 801 and 802 is identical, and the adapter keeps no second copy of the rate.
- **Instance lifetime.** The instance is created in `retro_init` and destroyed in `retro_deinit`, as the plan states. The seed created it at load instead. `retro_load_game` resets every per-game static through `lr_unload` before loading.
- **Host arguments.** The host takes `<sha256> W H AW AH`. The digest comes from `tests/testcard_pins.cmake`. The crop and display aspect are literals in `tests/libretro/tests.cmake`, because the pins file holds no geometry.
- **Extra host checks.** The host also checks `retro_load_game_special` (false), `retro_get_region` (NTSC), `valid_extensions` (empty), and that a power-cycle reset still shows the card. It checks that after deinit, init and load the first frame carries 801 stereo frames, which shows the remainder was reset.
- **Seed changes.** The vendored-check message now reads `<path>: sha256 <got>, PROVENANCE.md records <want>`, as the plan asks.

## Requirement status

`requirements-completed` lists FRAME-03. In-process, the core delivers the runner's frame byte for byte for both machines, with the exact rate and geometry. No later plan in this phase declares FRAME-03, so its REQUIREMENTS.md box is ticked. The hosted RetroArch screenshot compare (plans 09 and 11) belongs to FRAME-07.

## Known Stubs

None. Silence as audio and the refusal of content are this release's stated behaviour (assumptions A-05-2 and the context block), not stubs.

## Issues Encountered

- **Not yet documented.** The adapter's user-facing behaviour (no content, test card, silence, no extensions) is not described in `docs/` yet. No plan-05 file names a doc. The README and docs plan should add a short libretro page.
- **Format check.** `hygiene.format` (plan 08) must exclude `libretro/libretro.h`, as the plan notes. `clang-format` is not installed locally, so the new C files were not format-checked in this session.

## Next Phase Readiness

The hosted RetroArch case can load `build/<preset>/libretro/cogmaster_libretro.dylib` with its `.info` and no content. It can then compare a screenshot with `cogmaster-run --frames 60 --png 60:FILE`. Packaging can install the `libretro` component (`cores/` and `info/`).

## Self-Check: PASSED

- All 9 created files exist on disk.
- Commits 5be47b1, 081a910 and d556459 are ancestors of HEAD.
