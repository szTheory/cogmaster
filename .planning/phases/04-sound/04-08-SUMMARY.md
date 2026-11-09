---
phase: 04-sound
plan: "08"
subsystem: sound-fixtures-and-pins
tags: [psg-sweep, game-gear, hash-matrix, audio-pins, wla-dx, fixture, cmake]

requires:
  - phase: 04-sound
    provides: cogmaster-run --hash-audio and the table-driven psg_sweep fixture with its FIXTURE_GG entries (plan 04-07)
provides:
  - psg_sweep.gg, the Game Gear image of the sound table, 32,768 bytes, judged by the same 38 mailbox rows under the gg profile
  - audio-transitions and audio-pcm rows in hashes.txt for psg_sweep on the Master System (NTSC and PAL), the Game Gear in Master System mode and native
  - tests/audio/transitions.txt and tests/audio/pcm.txt, the frozen pins, compared inside runner.hashes_file in every lane and by runner.hash_audio
affects: [04-09, 04-10, 04-12, 04-13, 04-14, 04-15]

actuals:
  tokens: 4100
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "A sound row in hashes.txt is the runner's audio line with the tag '<file> <profile> <region> baseline' inserted after the keyword, so each line still starts with audio-transitions or audio-pcm and a grep regenerates its pin file"
    - "Pins are the audio subsets of hashes.txt: runner.hashes_file compares them line by line and names the first line that differs"

key-files:
  created:
    - tests/roms/src/psg_sweep_gg.asm
    - tests/roms/src/psg_sweep_gg.link
    - tests/audio/transitions.txt
    - tests/audio/pcm.txt
  modified:
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - docs/testing.md

key-decisions:
  - "The Game Gear image is built through the same loop as every fixture, with a second special case beside boot_to_play_gg, so the image is psg_sweep.gg while its digest line and its mailbox rows keep the names psg_sweep_gg and psg_sweep"
  - "The audio rows run after the playing rows and use the playing loop's profile names: sms, gg-sms and native; the Japanese-region row is left to the FM plan because an attached FM unit changes it"
  - "runner.hashes_file keeps TIMEOUT 30: slowest local run 2.3 s inside the parallel ci workflow (1.2 s alone), so twice that is well under the floor; the be32 and wasm32 timings of the first hosted run are still to be read"

patterns-established:
  - "Frozen pins regenerate with grep '^audio-transitions' and grep '^audio-pcm' over build/ci/hashes.txt; a mix retune changes only pcm.txt, a transitions.txt change means the emulated behaviour moved"

requirements-completed: [SND-03, SND-01]

coverage:
  - id: D1
    description: "psg_sweep.gg assembles from the same source with FIXTURE_GG into a 32,768 byte image and reports the same 38 mailbox records as the Master System image under the gg profile, with no Game Gear only rows"
    requirement: SND-01
    verification:
      - kind: integration
        ref: "ctest fixtures.psg_sweep.gg and fixtures.psg_sweep; wc -c build/ci/fixtures/psg_sweep.gg is 32768"
        status: pass
    human_judgment: false
  - id: D2
    description: "The Game Gear image plays in stereo through the runner: channels 2, clipped 0, peak 7688 over 300 frames; the Master System image's digest line is unchanged"
    requirement: SND-01
    verification:
      - kind: integration
        ref: "build/ci/cogmaster-run build/ci/fixtures/psg_sweep.gg --frames 300 --hash-audio, greps for channels 2 and clipped 0, exit 0; git diff of tests/built/expected.txt shows one added line and none removed"
        status: pass
    human_judgment: false
  - id: D3
    description: "hashes.txt carries the audio-transitions and audio-pcm rows for psg_sweep.sms on sms ntsc-u, sms pal and gg-sms ntsc-u and psg_sweep.gg native ntsc-u, each tagged with file, profile, region and variant"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.hashes_file (tests/runner/tests.cmake case hash_matrix, tests/cmake/hashes.cmake MODE write)"
        status: pass
    human_judgment: false
  - id: D4
    description: "tests/audio/transitions.txt and tests/audio/pcm.txt each hold 4 lines equal to the audio subsets of build/ci/hashes.txt; hash_matrix fails and names the first differing line when a pin is wrong; hash_audio requires its own psg_sweep.sms lines to be among the pins"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.hashes_file and runner.hash_audio; negative control: a pcm.txt copy with one digit changed, passed as PINS to the hash_matrix case, exit 1 naming line 1"
        status: pass
    human_judgment: false
  - id: D5
    description: "The hash matrix output is byte-identical between two independent output directories, so the cross-lane hashes.txt compare can carry the sound rows"
    requirement: SND-03
    verification:
      - kind: other
        ref: "cmp of the scratchpad hashes.txt against build/ci/hashes.txt, exit 0; the seven-lane comparison itself runs in the hosted hash-equality job"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/testing.md documents the regeneration commands, what each hash proves and the retune rule, drops the stale silence sentence and moves the hosted Game Gear test facts into the Hosted RetroArch section; the file stays under its 40,960 byte budget (39,305)"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest docs.budgets inside cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D7
    description: "The full ci and hygiene workflows pass with the new image, test, rows, pins and inventories"
    requirement: SND-03
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci (exit 0, 370 tests) then cmake --workflow --preset hygiene (exit 0, 38 tests)"
        status: pass
    human_judgment: false

duration: 4min
completed: 2026-11-09
status: complete
plan_head_before: 4993ec519a3bbc2fdbb0c834856d49ea5fbe29d6
plan_head_after: 2ec8a85f83018d8c19b046cf7ba57f97b389bf69
commits: 2
---

# Phase 4 Plan 08: psg_sweep.gg and the frozen audio pins Summary

**The Game Gear image of the owned psg_sweep fixture plays in stereo through the runner, and the four psg_sweep transition and PCM hashes are frozen in tests/audio and compared on every lane through hashes.txt.**

## Performance

- **Duration:** about 4 min of execution after the reading
- **Started:** 2026-11-09T20:55:42Z
- **Completed:** 2026-11-09T21:00:00Z (approximate)
- **Tasks:** 2
- **Files modified:** 11 (4 created)

## Accomplishments

- `psg_sweep_gg.asm` defines `FIXTURE_GG` and includes `psg_sweep.asm`; the fixtures loop builds it into `psg_sweep.gg` (32,768 bytes, digest `6f2c5ee2...` in `tests/built/expected.txt`). `fixtures.psg_sweep.gg` judges it against the same 38 `psg_sweep` rows of `tests/diag/expected.csv` under profile `gg`: no new rows, no change to `psg_sweep.sms`, whose digest line did not move. The Game Gear counters already matched, so `psg_sweep.asm` needed no fix.
- The Game Gear run prints `channels 2 samples 240309 peak 7688 clipped 0`, and its transition count is 61,352 against 61,340 for the Master System image (the twelve `$06` entries).
- `tests/cmake/hashes.cmake` runs four sound rows after the playing rows (`psg_sweep.sms` on `sms` NTSC-U, `sms` PAL and `gg-sms` NTSC-U, `psg_sweep.gg` `native` NTSC-U, 300 frames each, exit 0 required) and appends each `audio-` line with the tag `<file> <profile> <region> baseline` after the keyword.
- `runner.hashes_file` extracts the `audio-transitions` and `audio-pcm` lines and requires them to equal `tests/audio/transitions.txt` and `tests/audio/pcm.txt` line for line (an empty pin file or a different count also fails), naming the first differing line. `runner.hash_audio` requires its own tagged `psg_sweep.sms sms ntsc-u` lines to be among the pins. Both pin files hold 4 lines.
- Measured `runner.hashes_file`: 1.21 to 1.58 s alone (four runs), 2.26 s inside the parallel `cmake --workflow --preset ci`; TIMEOUT stays 30 with a comment naming the measurement. The be32 (x8) and wasm32 (x5) timings still have to be read from the first hosted run; 2.3 s x 8 is 18 s against 30.
- `docs/testing.md` (39,305 of 40,960 bytes): the sound rows, the two regeneration commands, what each hash proves and the retune rule; the stale silence-test sentence is gone; the hosted Game Gear paragraph in "Pinned downloads" is replaced by one sentence in "Hosted RetroArch" (Start on frames 5, 6 and 8; native crop at field origin (48,24) with the shared oracle), and its past-session dispatch sentences are dropped. About 1,650 bytes of headroom remain for later plans.

## Task Commits

1. **Task 1 (tracer): psg_sweep.gg plays in stereo through the runner** - `9c2094e` (test)
2. **Task 2: The hash matrix carries the sound rows and compares them to frozen pins** - `2ec8a85` (test)

**Plan metadata:** recorded in the following docs commit.

## Verification Record

Commands run from the repository root, with exit statuses.

- Tracer gate (Task 1 `<verify>`, automated only): `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fixtures\.|runner\.hash_audio$|compat\.fixtures$)'` then the 32,768 byte check, `cogmaster-run build/ci/fixtures/psg_sweep.gg --frames 300 --hash-audio` and the two greps for `channels 2` and `clipped 0`: exit 0, 40 of 40 tests passed. Tracer verified end to end, expansion proceeded.
- Task 1 acceptance: `fixtures.psg_sweep` and `fixtures.psg_sweep.gg` Passed; `git diff --unified=0 -- tests/built/expected.txt` shows one added line (`psg_sweep_gg`) and no removed line.
- Task 2 `<verify>` first command: `cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(runner\.hashes_file|runner\.hash_audio|docs\.budgets)$'`: exit 0, 3 of 3 passed; `grep -c '^audio-pcm ' tests/audio/pcm.txt` prints 4; `grep -c '^audio-transitions ' tests/audio/transitions.txt` prints 4.
- Negative control (run once, not committed): `cmake -DCASE=hash_matrix ... -DPINS=<scratchpad copy with one digit of one pcm line changed> -P tests/runner/tests.cmake`, exit 1 with `audio-pcm line 1 differs` naming got and want. The same run's hashes.txt is byte-identical to `build/ci/hashes.txt` (`cmp`, exit 0).
- `cmake --workflow --preset ci`: exit 0 (370 tests). `cmake --workflow --preset hygiene`: exit 0 (38 tests). Hygiene also passed in both commit hooks.

## Files Created/Modified

- `tests/roms/src/psg_sweep_gg.asm`, `psg_sweep_gg.link` - the Game Gear image's source and link file
- `tests/fixtures/tests.cmake` - `psg_sweep_gg` in the build loop (image `psg_sweep.gg`), `psg_sweep.asm` as a build dependency, `fixtures.psg_sweep.gg`
- `tests/built/expected.txt` - the `psg_sweep_gg` digest line
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - `fixtures.psg_sweep.gg`
- `tests/cmake/hashes.cmake` - the sound rows in mode `write`
- `tests/runner/tests.cmake` - pin comparison in `hash_matrix` and `hash_audio`, the `PINS` argument, the `runner.hashes_file` timeout comment
- `tests/audio/transitions.txt`, `tests/audio/pcm.txt` - the frozen pins
- `docs/testing.md` - regeneration, what each hash proves, the retune rule, the two prose edits

## Decisions Made

See `key-decisions`.

## Deviations from Plan

None - plan executed exactly as written. (The plan told the agent to read the hosted be32 and wasm32 timings of the first CI run before freezing the timeout; no hosted run that includes the audio rows exists yet, so the timeout is recorded as provisional, as plan 04-07's summary anticipated.)

## Issues Encountered

- The first negative-control run failed earlier than the pin comparison because the hash matrix reads the play input scripts from `<OUT>/tests/`; the scratchpad output directory was given copies of them and the control then failed on the changed pin as intended.

## Known Stubs

None.

## Threat Flags

None. The change adds a fixture image, test code and text pins; no new network, input or trust surface.

## Next Phase Readiness

- The hosted `hash-equality` job (seven lanes) will be the first cross-platform comparison of the sound rows; read the be32 and wasm32 `runner.hashes_file` timings from that run and raise the TIMEOUT with a measured comment if 8 x 2.3 s or 5 x 2.3 s does not fit.
- The FM plans add the Japanese-region and FM rows to the same loop and regenerate the pins with the commands in `docs/testing.md`.
- Plans 04-14 and 04-15 have about 1,650 bytes of room in `docs/testing.md`.

## Self-Check: PASSED

- `tests/roms/src/psg_sweep_gg.asm`, `psg_sweep_gg.link`, `tests/audio/transitions.txt` and `tests/audio/pcm.txt` exist.
- Commits `9c2094e` and `2ec8a85` are ancestors of HEAD.
- `fixtures.psg_sweep.gg`, `runner.hashes_file` and `runner.hash_audio` pass; `cmake --workflow --preset ci` and `--preset hygiene` exit 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
