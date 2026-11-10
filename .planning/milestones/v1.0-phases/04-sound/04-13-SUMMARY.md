---
phase: 04-sound
plan: "13"
subsystem: sound-fm-fixture
tags: [ym2413, fm-sweep, port-f2, fm-detect, audio-hashes, pins, wla-dx]

requires:
  - phase: 04-sound
    provides: the FM unit on the Japanese console with its $F0/$F1/$F2 ports, mixer gate, reset and saved state (plan 04-12), the audio hash lines and pins (plan 04-07)
provides:
  - fm_sweep, an owned WLA-DX image with its own $F2 detection routine, its detect record per profile, and a frame-counted FM script
  - fixtures.fm_sweep.sms-japan, fixtures.fm_sweep.sms-export and fixtures.fm_sweep.gg, judged by rows in tests/diag/expected.csv
  - fm.detect, the same built image run with the unit forced on for an export console and off for a Japanese one
  - runner.hash_audio.fm and three new rows in each of the two pin files
affects: [04-14, 04-15]

actuals:
  tokens: 4664
  tasks: 2
  commits: 2
plan_head_before: b2c95eafe7be463784c401fb3f532bbf031b3098
plan_head_after: 0c1ade45bc7e87c22df2f61301a8acf999b59400

tech-stack:
  added: []
  patterns:
    - "A fixture's detection answer is judged twice: through the runner's default unit setting by a mailbox row per profile, and through the library's media.fm setting by a C test that reads the mailbox with cogmaster_peek"
    - "A hash test over a fixture runs the Japanese and export profiles together and requires them to differ, so a silent unit and a unit that leaks onto a console without one both fail"

key-files:
  created:
    - tests/roms/src/fm_sweep.asm
    - tests/roms/src/fm_sweep.link
  modified:
    - tests/roms/manifest.txt
    - tests/fixtures/tests.cmake
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/audio/test_opll.c
    - tests/audio/pcm.txt
    - tests/audio/transitions.txt
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - docs/testing.md

key-decisions:
  - "The detection routine reads $F2 first and writes that value back afterwards, so it leaves the port as it found it; it writes 0, 1, 2 and 3, compares only bits 1-0 (bits 7 to 5 of the unit's read are a counter), and answers 1 only when all four match, so an export console (which reads $DC, idle $FF) fails at the value 0 and open bus fails likewise"
  - "The script runs whatever the detect answer is, so the export and Game Gear runs prove the unit's ports change nothing there; the PSG tone in the control segment keeps those runs from being silent and gives the $F2 steps a second chip to gate"
  - "The unit writes are counted in the mailbox (each $F0/$F1 pair and each $F2 write once), the PSG bytes are not, and the records are cumulative like the PSG fixture's, so every value in the CSV was derived by hand from the table before the run printed it"
  - "fm.detect also runs the automatic setting on both regions (Japan finds the unit, export does not), beyond the plan's two cases, because it costs one more run of a 300 frame image and pins the region rule to the same program"
  - "runner.hash_audio.fm is a second case of the existing hash_audio script that returns before the card checks, so the audio_run helper (two runs, line shapes, sample formula, clipped 0) is shared rather than copied"

patterns-established:
  - "The three FM pin rows are placed in tests/cmake/hashes.cmake after the rows they sit beside, so regenerating the pins adds lines and never reorders existing ones"

requirements-completed: [SND-02, SND-03]

coverage:
  - id: D1
    description: "fm_sweep assembles to exactly 32,768 bytes from its own source with its own $F2 detection routine, and its digest is pinned in tests/built/expected.txt"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "ctest -R '^(fixtures\\.|compat\\.fixtures$)' and test $(wc -c < build/ci/fixtures/fm_sweep.sms) -eq 32768: exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The mailbox reports detect 1 on the Japanese console with the automatic setting and 0 on the export console and on the Game Gear, and the six script counters (frames 60, 126 and 214, writes 16, 39 and 47) equal the values derived by hand from the table on every profile"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "ctest fixtures.fm_sweep.sms-japan, fixtures.fm_sweep.sms-export, fixtures.fm_sweep.gg"
        status: pass
    human_judgment: false
  - id: D3
    description: "The built image run with media.fm on for an export console reports detect 1 and with media.fm off for a Japanese console reports 0, the automatic setting following the region, and the script runs to its last record under every setting"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "tests/audio/test_opll.c#detect (ctest fm.detect)"
        status: pass
    human_judgment: false
  - id: D4
    description: "fm_sweep's Japanese and export runs each print identical lines twice, hold the sample total formula, clip nothing, peak at most 29,204 and above 0 on the Japanese console (6,682 against 1,671 on the export one), equal their frozen lines and differ from each other"
    requirement: SND-03
    verification:
      - kind: integration
        ref: "ctest runner.hash_audio.fm"
        status: pass
    human_judgment: false
  - id: D5
    description: "Both pin files gained exactly three rows (fm_sweep.sms on ntsc-j and ntsc-u, psg_sweep.sms on ntsc-j) and lost or changed none; the Japanese psg_sweep row differs from the export one only because the unit is attached"
    requirement: SND-03
    verification:
      - kind: integration
        ref: "git diff --unified=0 -- tests/audio/pcm.txt tests/audio/transitions.txt shows six added lines and no removed audio line; ctest runner.hashes_file"
        status: pass
    human_judgment: false
  - id: D6
    description: "ctest -L fm lists and passes 23 tests: the lockstep, the chip and unit tests, fm.detect, the three fixtures.fm_sweep cases and runner.hash_audio.fm; the full ci and hygiene workflows exit 0 with the new tests in both inventories"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci: exit 0 (394 of 394 plus 4 of 4); cmake --workflow --preset hygiene: exit 0 (38 of 38)"
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-11-09
status: complete
---

# Phase 04 Plan 13: The FM Fixture, Its Detection and Its Frozen Hashes Summary

**fm_sweep, an owned WLA-DX program that detects the YM2413 through its own $F2 routine (1 on the Japanese console, 0 on export and Game Gear), plays instrument-1 notes, every rhythm key and $F2 steps 0 to 3, and is judged by mailbox rows, a C detection test under media.fm and three new frozen pin rows each for transitions and PCM.**

## Performance

- **Duration:** 13 min by the system clock (the start time was not captured; taken from the end of the previous plan, 22:02Z)
- **Completed:** 2026-11-09T22:16:00Z
- **Tasks:** 2
- **Files modified:** 14 (two created, twelve changed)

## Accomplishments

- `tests/roms/src/fm_sweep.asm` (with its link file) detects the unit by writing 0 to 3 to `$F2`, reading each back and comparing bits 1 to 0, then restoring the value it read first. The routine is written here; no SMS Power code was opened or copied. A mailbox record carries the answer; three segments follow: instrument 1 as a chord and then an octave lower (60 frames, 16 writes), rhythm mode with each drum and all five together (126 frames and 39 writes so far), and an FM note under a PSG tone while `$F2` steps 0, 1, 2, 3 for 20 frames each (214 frames and 47 writes). The image is 32,768 bytes; `fixtures.fm_sweep.sms-japan` reports detect 1 and `.sms-export` and `.gg` report 0. The six counters were derived from the table before the first run and the run printed the same values.
- `fm.detect` loads the built image through `cogmaster_load` with the unit on for an export console (detect 1) and off for a Japanese one (0), runs to the mailbox's done status taking each frame's sound, and reads the records with `cogmaster_peek`. It also runs the automatic setting on both regions and requires the script's last record under every setting. A scratch change that made the routine answer 1 on a miss failed it with `japan, unit off: detect record 1, want 0` and `export, automatic: detect record 1, want 0`; the restored source passes.
- `runner.hash_audio.fm` runs `fm_sweep.sms` on the Japanese and export consoles, each twice: the lines repeat, the sample total follows the formula (240,309 for 300 frames), nothing clips, the peaks are 6,682 and 1,671 against the 29,204 line, both runs equal their pins and they differ from each other.
- The pins gained `fm_sweep.sms sms ntsc-j`, `fm_sweep.sms sms ntsc-u` and `psg_sweep.sms sms ntsc-j` in `transitions.txt` and `pcm.txt`; regenerated with the two `grep` commands of the docs. The Japanese `psg_sweep` row (61,342 changes against 61,340 on export, peak 4,221 against 7,222) moves only because the unit adds the control record and puts the PSG at its lower gain; every existing row is byte for byte unchanged.

## Task Commits

1. **Task 1: fm_sweep detects the FM unit per profile and plays its script (tracer)** - `05df3dd` (feat)
2. **Task 2: fm.detect, the FM hash rows and the frozen pins** - `0c1ade4` (test)

**Plan metadata:** the commit that adds this summary, then the state and roadmap commit.

## Measured values and commands

Every command ran from the repository root.

| Check | Command | Result |
|---|---|---|
| Task 1 verify | `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fixtures\.\|compat\.fixtures$)' && test "$(wc -c < build/ci/fixtures/fm_sweep.sms)" -eq 32768` | exit 0 (42 tests, 1.6 s; image 32,768 bytes, SHA-256 d6fb59bc0e18...) |
| Tracer gate | the Task 1 verify re-run before Task 2 (automated-only verify, end-of-phase mode) | exit 0, expanded |
| Task 2 verify, fm | `ctest --test-dir build/ci --no-tests=error --output-on-failure -L fm` | exit 0 (23 of 23, 1.1 s) |
| Task 2 verify, runner | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(runner\.hashes_file\|runner\.hash_audio\|fixtures\.)'` | exit 0 (43 tests, 2.7 s) |
| Pin diff | `git diff --unified=0 -- tests/audio/pcm.txt tests/audio/transitions.txt > build/pins.diff && ! grep -Eq '^-audio' build/pins.diff` | exit 0; 6 added lines, 0 removed |
| Workflows | `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` | both exit 0 (394 of 394 plus 4 of 4; 38 of 38) |
| Mutation | the detect routine made to answer 1 on a miss, rebuilt, `ctest -R '^fm\.detect$'`, then restored | exit 8 under the mutation, exit 0 restored |

The pin rows added, shortened: `fm_sweep.sms sms ntsc-j` (count 86,278, peak 6,682), `fm_sweep.sms sms ntsc-u` (count 6,939, peak 1,671), `psg_sweep.sms sms ntsc-j` (count 61,342, peak 4,221), each at 300 frames.

The be32 and wasm32 legs were not run here (`qemu-ppc` and the pinned wasmtime are not installed); the timeouts in `tests/fixtures/tests.cmake` and `tests/runner/tests.cmake` are marked provisional, and the hosted lanes run every new test and compare the new rows byte for byte across the seven lanes.

## Files Created/Modified

- `tests/roms/src/fm_sweep.asm`, `tests/roms/src/fm_sweep.link` - the owned FM fixture and its link file
- `tests/roms/manifest.txt`, `tests/built/expected.txt` - the built-only note and the image digest
- `tests/diag/expected.csv` - nine rows: the detect row for each of three profiles and the six counters
- `tests/fixtures/tests.cmake` - fm_sweep in the build list, the three mailbox tests and `fm.detect`
- `tests/audio/test_opll.c` - the `detect` case of `cogmaster-test-fm`
- `tests/cmake/hashes.cmake`, `tests/runner/tests.cmake` - the three sound rows and `runner.hash_audio.fm`
- `tests/audio/pcm.txt`, `tests/audio/transitions.txt` - three new rows each
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the five new tests
- `docs/testing.md` - the sound rows, the detection tests and the `fm` label run

## Decisions Made

See key-decisions. In short: the routine restores what it read, the script runs on every console so the no-unit runs prove the ports change nothing, the unit writes are counted and the PSG bytes are not, and the FM hash case shares the existing audio helper.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] The sound docs named only the PSG fixture**
- **Found during:** Task 2
- **Issue:** `docs/testing.md` listed the sound rows and the detection tests nowhere, and the project rule is that a behaviour change lands tested and documented together. The plan's file list has no doc.
- **Fix:** one paragraph in `docs/testing.md` naming the three new rows, why the Japanese `psg_sweep` row moves, and the three fm tests.
- **Files modified:** docs/testing.md
- **Committed in:** 0c1ade4

**2. [Additions beyond the plan's case list]** `fm.detect` also runs the automatic setting on both regions, and `runner.hash_audio.fm` also runs the export console and requires its run to differ from the Japanese one; both close a gap the plan's two cases left (a unit that stays silent and a unit that leaks).

---

**Total deviations:** 1 auto-fixed (Rule 2) and one set of additions
**Impact on plan:** no scope change; the additions are one more run each of a 300 frame image.

## Issues Encountered

The first `cmake --preset ci` failed with `registered tests differ from sorted inventory` because the three fixture tests were registered before their inventory rows; the rows were added and configure passed. The first `runner.hashes_file` run after the rows were added failed on the line count, as expected, and wrote the `hashes.txt` the pins were regenerated from. `fm.detect` first looped zero times because the mailbox status is 0 before the program starts; the loop now waits for the running status before accepting done, as the runner does.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The FM unit is covered end to end: plan 04-14 can build on `fm_sweep` and its pins, and the libretro and hosted work sees the same image.
- Not run here: the be32 and wasm32 legs; the hosted lanes run every new test.
- No stubs and no skipped tests; no open ledger entry was added.

## Self-Check: PASSED

`tests/roms/src/fm_sweep.asm`, `tests/roms/src/fm_sweep.link`, `tests/audio/test_opll.c` and `docs/testing.md` exist; commits 05df3dd and 0c1ade4 are ancestors of HEAD; `ctest -L fm` lists 23 passing tests; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
