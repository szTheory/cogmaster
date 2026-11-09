---
phase: 04-sound
plan: "07"
subsystem: runner-audio
tags: [runner, hash-audio, sha256, trace-merge, pcm, psg-sweep, wla-dx, fixture, c17]

requires:
  - phase: 04-sound
    provides: the trace log, the sample drain and the cadence (plan 04-02); the PSG with its tick-0 snapshot records (plan 04-05); the Game Gear $06 routing and the cost bound (plan 04-06)
provides:
  - cogmaster-run --hash-audio, which drains the level-change log and the samples after every frame, merges the log into one canonical stream and prints an audio-transitions line and an audio-pcm line
  - runner/audio.c, a module owning the drain, the sort and merge, the two SHA-256 contexts and the exit-1 checks
  - runner.hash_audio, proving two identical runs, the cadence total in CMake integer math, no clipping, a silent card and a failing 800-sample build
  - the owned psg_sweep fixture (19 segments, 38 mailbox records) and fixtures.psg_sweep, judged by tests/diag/expected.csv
affects: [04-08, 04-09, 04-10, 04-12, 04-14]

actuals:
  tokens: 12300
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "The runner owns the canonical order of the level-change log: sort by (tick, source, emission index) with only < and > comparisons, keep the last level per tick and source, drop a level the source already holds, hash 11 bytes per kept change"
    - "A runner built with COGMASTER_RUN_AUDIO_CAP=800 proves the per-frame sample-count branch fires, the way COGMASTER_RUN_RAISE_AT proves the stop"
    - "A table-driven WLA-DX player with assemble-time guards (.REDEFINE segment counter, .FAIL past the mailbox size) so a table edit cannot silently lose records"

key-files:
  created:
    - runner/audio.c
    - runner/audio.h
    - tests/roms/src/psg_sweep.asm
    - tests/roms/src/psg_sweep.link
  modified:
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - CMakeLists.txt
    - docs/runner.md
    - tests/runner/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/roms/manifest.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The runner's audio code is its own module, runner/audio.c with audio.h, added to COGMASTER_RUNNER_SOURCES, so main.c grows by 23 lines and every test runner built from that list picks it up"
  - "Both drops are checked after every frame and fail at once with exit 1, naming the frame, rather than at the end of the run; the trace overflow, the sample drop, a count off 801 or 802 (965 or 966 PAL) and a source whose ticks go backwards in the order the core logged them are four separate diagnostics"
  - "Levels are compared and serialised as 16 bits (the signed FM sum keeps its low 16 bits), and the no-level sentinel is 0x10000, so no real level equals it"
  - "psg_sweep counts frames waited, not true frames: the pops loop takes about one frame of Z80 time, so the counter and the real frame can differ by one; the mailbox rows state the counter"
  - "The Game Gear port $06 entries sit between a volume write and its WAIT in segments 12 to 15 (three per segment, eight single bits then $0F, $F0, $00, $FF), so the SMS and GG images report identical records"

patterns-established:
  - "Test cases in tests/runner/tests.cmake run a runner twice and compare stdout, then check each line by tolerant regex, then check totals in CMake integer math (group 1 of those regexes is the line start)"

requirements-completed: [SND-03, SND-01]

coverage:
  - id: D1
    description: "cogmaster-run --hash-audio, independent of --hash, prints audio-transitions and audio-pcm lines for the frames that ran; a usage error with --bench or --probe; --hash-audio is no longer an unknown option"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.usage.unknown and runner.hash_audio (tests/runner/tests.cmake)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Two runs of the test card on the Master System and the Game Gear, and of psg_sweep.sms for 300 frames, print identical stdout; samples equal floor(N x 3,504,512 / 4,375) in CMake math, clipped is 0, the card's peak is 0 and psg_sweep's peak is above 0 and at most 29,204"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.hash_audio (case hash_audio in tests/runner/tests.cmake)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A runner built to offer the core 800 samples a frame exits 1 with a message naming frame 1 and 800 samples and prints no hash line, so the per-frame count branch is live"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.hash_audio (cogmaster-test-run-audio-short)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The merge rule is implemented as stated: sort by tick, source and emission index with no subtraction comparator, last level per tick and source kept, a level equal to the source's previous merged level dropped across frames, each kept change serialised as 11 little-endian bytes; the runner never hashes struct or int16 array memory"
    requirement: SND-03
    verification:
      - kind: other
        ref: "agent read runner/audio.c against the rule; the ci workflow's pinned-output lanes compare the lines in a later plan"
        status: pass
    human_judgment: false
  - id: D5
    description: "psg_sweep.sms is exactly 32,768 bytes, assembles with 19 segments and 38 records inside the 60-record mailbox, and its mailbox records equal the 38 rows of tests/diag/expected.csv; the Game Gear image assembled from the same source reports identical records"
    requirement: SND-01
    verification:
      - kind: integration
        ref: "ctest fixtures.psg_sweep; wc -c build/ci/fixtures/psg_sweep.sms is 32768; grep -c '^psg_sweep,' tests/diag/expected.csv is 38"
        status: pass
      - kind: other
        ref: "psg_sweep.gg assembled with -D FIXTURE_GG into the scratchpad, cogmaster-run --mailbox records diffed against the SMS image, diff exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "The assemble-time guards fire: an extra segment mark fails the 19-segment guard and twelve extra marks (past the mailbox's 60 records) fail the mailbox guard"
    requirement: SND-01
    verification:
      - kind: other
        ref: "wla-z80 over scratchpad copies of psg_sweep.asm with S_MARK lines added, exit 1 with each FAIL message"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/runner.md documents --hash-audio, both lines, the merge rule and the exit-1 conditions in user terms, and the stale silent-audio and sound-device sentences are corrected"
    requirement: SND-03
    verification:
      - kind: other
        ref: "ctest docs.budgets and docs.api_names pass inside cmake --workflow --preset ci; the agent read the new section against runner/audio.c"
        status: pass
    human_judgment: false
  - id: D8
    description: "The full ci and hygiene workflows pass with the new module, tests, fixture and inventories"
    requirement: SND-03
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci (exit 0, 369 tests) then cmake --workflow --preset hygiene (exit 0, 38 tests)"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-11-09
status: complete
plan_head_before: 2105da289872c8bf9507d35fd9bb49902e57660e
plan_head_after: bab292142c14e241fcf3c12fb929030f63b98e43
commits: 2
---

# Phase 4 Plan 07: --hash-audio and the psg_sweep fixture Summary

**cogmaster-run --hash-audio drains the core's level-change log and samples every frame, merges the log into a canonical stream of 11-byte transitions and prints two SHA-256 lines, and the owned table-driven psg_sweep fixture exercises every PSG register bit and extreme with 38 judged mailbox records.**

## Performance

- **Duration:** about 11 min
- **Started:** 2026-11-09T20:43:30Z (approximate; the previous plan's closing commit)
- **Completed:** 2026-11-09T20:54:00Z (approximate)
- **Tasks:** 2
- **Files modified:** 16 (4 created)

## Accomplishments

- `runner/audio.c` and `runner/audio.h`: after each frame the runner drains up to 32,768 trace records and up to `COGMASTER_RUN_AUDIO_CAP` (966) sample frames, fails with exit 1 and a one-line diagnostic when the log overflowed, a source's ticks go backwards, the core dropped samples, or the frame holds a count other than 801 or 802 (965 or 966 on PAL); it sorts the records by (tick, source, emission index), keeps the last level per tick and source, drops a level the source already holds (carried across frames) and hashes each kept change as 11 little-endian bytes, and hashes the samples as little-endian int16 (L, R on the Game Gear) while tracking peak and clipped.
- The card on a Master System prints `audio-transitions 60 count 878` and `audio-pcm 60 rate 48000 channels 1 samples 48061 peak 0 clipped 0` (48,061 = floor(60 x 3,504,512 / 4,375)); the Game Gear prints `channels 2`; PAL prints 57,945 samples for 60 frames. The count of 878 is the noise register's output bit changing at attenuation 15: levels are register state, not volume.
- `runner.hash_audio` (labels portable, psg, runner) runs two identical runs of the card on both machines and of `psg_sweep.sms` for 300 frames, checks both line shapes by tolerant regex, the sample total in CMake integer math, clipped 0, the card's peak 0 and the fixture's peak (7,222 locally) between 1 and 29,204, and runs `cogmaster-test-run-audio-short` (a runner built with `COGMASTER_RUN_AUDIO_CAP=800`), which must exit 1 naming frame 1 and 800 samples. Measured 0.25 s alone (0.45 s inside the parallel ci workflow); TIMEOUT 30 as for the other runner tests.
- `runner.usage.unknown` no longer lists `--hash-audio` as an unknown option; `--hash-audio` with `--bench` or `--probe` is a usage error.
- `psg_sweep`: a 32,768 byte image (checked) of a Z80 player that reads a byte stream (WAIT, PSG byte, `$06` byte under `FIXTURE_GG`, MARK, POPS, END). It covers protocol cases (latch then data, a data byte after a volume latch and after a noise latch, the latch surviving three data bytes), periods 0, 1, 2, 3, $0FE, $1FF and $3FF and a bit walked through all ten period bits on each tone, all 16 attenuations on all 4 channels, all 8 noise controls, mode 3 with tone 2 at 0, mid and high (white then periodic), the noise register resetting on a data byte, and 2,048 attenuation writes 29 cycles apart (`OUT (C),A` 12 + `EX AF,AF'` 4 + taken `DJNZ` 13). Tone 0 sounds at attenuation 0 from frame 20 to frame 70 with no input. It closes after 289 waited frames.
- 19 segments close with two records each (38 in all, inside the 60-record mailbox); assemble-time guards (`.REDEFINE` segment counter, `.FAIL` unless it equals 19 and twice it is at most `MB_RECORDS_MAX`) were each seen to fire on a scratch copy.
- The same source assembled with `FIXTURE_GG` reports identical records (diffed), adds the twelve `$06` entries and plays on two channels; the Game Gear image and the frozen pins follow in the next plan.

## Task Commits

1. **Task 1 (tracer): cogmaster-run --hash-audio hashes the drained trace and PCM** - `de7170c` (feat)
2. **Task 2: The psg_sweep fixture covers every PSG register bit and extreme** - `bab2921` (test)

**Plan metadata:** recorded in the following docs commit.

## Verification Record

Commands run from the repository root, with exit statuses.

- Tracer gate (Task 1 `<verify>`, automated only): `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(runner\.(hash_audio|usage\.|hashes_file)|psg\.|docs\.)'`: exit 0, 28 of 28 passed. Tracer verified end to end, expansion proceeded.
- Task 1 acceptance: `build/ci/cogmaster-run --frames 60 --hash-audio` exit 0 with one `audio-transitions` and one `audio-pcm` line (`samples 48061 peak 0 clipped 0`); `build/ci/tests/cogmaster-test-run-audio-short --frames 2 --hash-audio` exit 1.
- Task 2 `<verify>`: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fixtures\.|runner\.hash_audio$|compat\.fixtures$)'`: exit 0, 39 of 39 passed; `wc -c < build/ci/fixtures/psg_sweep.sms` is 32768; `grep -c '^psg_sweep,' tests/diag/expected.csv` is 38 (within 30 to 60).
- Task 2 acceptance: `build/ci/cogmaster-run build/ci/fixtures/psg_sweep.sms --frames 300 --hash-audio` exit 0, `channels 1`, `samples 240309` (= floor(300 x 3,504,512 / 4,375)), `peak 7222`, `clipped 0`.
- `cmake --workflow --preset ci`: exit 0 (369 tests). `cmake --workflow --preset hygiene`: exit 0 (38 tests). Hygiene also passed before the first commit.

## Files Created/Modified

- `runner/audio.c`, `runner/audio.h` - the drain, sort, merge, hashing and exit-1 checks
- `runner/main.c`, `runner/args.c`, `runner/args.h` - the option, the trace capacity in the config, the per-frame call and the printing
- `CMakeLists.txt` - `runner/audio.c` joins `COGMASTER_RUNNER_SOURCES`
- `docs/runner.md` - the option row, the Sound hashes section and the corrected silent-audio sentences
- `tests/runner/tests.cmake` - `runner.hash_audio`, `cogmaster-test-run-audio-short`, `--hash-audio` removed from the unknown-option lists
- `tests/roms/src/psg_sweep.asm`, `psg_sweep.link` - the owned PSG coverage fixture
- `tests/fixtures/tests.cmake` - psg_sweep in the build list and `fixtures.psg_sweep`
- `tests/built/expected.txt`, `tests/diag/expected.csv`, `tests/roms/manifest.txt` - the image digest, 38 rows and a manifest comment
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - `runner.hash_audio` and `fixtures.psg_sweep`

## Decisions Made

See `key-decisions`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Structure] The audio code is its own runner module**
- **Found during:** Task 1
- **Issue:** The plan lists runner/main.c as the file that holds the drain, sort, merge and hashing. That is about 200 lines in a function-heavy file already over 540 lines, and the project's rule is small modules with plain control flow.
- **Fix:** The code lives in `runner/audio.c` with `audio.h`; `main.c` only sets the trace capacity, opens the module after the load, calls it per frame, prints and closes. `CMakeLists.txt` adds the source to `COGMASTER_RUNNER_SOURCES` (the other test runners derive their source lists from it).
- **Files modified:** runner/audio.c, runner/audio.h, runner/main.c, CMakeLists.txt
- **Verification:** the plan's verify and acceptance commands, the ci and hygiene workflows
- **Committed in:** de7170c

---

**Total deviations:** 1 auto-fixed (1 structure)
**Impact on plan:** No change to behaviour, output formats or tests; one extra file pair and one line in CMakeLists.txt.

## Issues Encountered

- The first run of `runner.hash_audio` read the wrong regex groups because group 1 of the line-start alternation shifts the captures by one; fixed by indexing groups 2 to 5 with a comment.
- The pops segment takes about one frame of Z80 time, so the mailbox's frame counter (frames waited) and the real frame count can differ by one; the rows state the counter, and the fixture finishes near frame 291, inside the 300-frame run.

## Known Stubs

None.

## Threat Flags

None. The runner reads no new input and allocates fixed buffers (512 KiB of records, 768 KiB of sort entries, a 966-frame sample buffer) once; a hostile ROM can only trip the exit-1 checks.

## Next Phase Readiness

- Plan 04-08 adds the Game Gear image (`psg_sweep.gg`), the frozen transition and PCM pins and the hash matrix; the fixture already assembles with `FIXTURE_GG` and reports identical records.
- `runner.hash_audio` timeout is the 30 s of the other runner tests (0.25 s locally); confirm against the first hosted be32 and wasm32 timings before freezing.
- The FM unit plans add sources 5 and 6, which the runner already accepts (it checks sources below 7).

## Self-Check: PASSED

- `runner/audio.c`, `runner/audio.h`, `tests/roms/src/psg_sweep.asm` and `psg_sweep.link` exist.
- Commits `de7170c` and `bab2921` are ancestors of HEAD.
- `runner.hash_audio` and `fixtures.psg_sweep` pass; `cmake --workflow --preset ci` and `--preset hygiene` exit 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
