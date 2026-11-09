---
phase: 04-sound
plan: "09"
subsystem: sound-fixtures-and-pins
tags: [boot-to-play, psg, game-gear-stereo, hash-matrix, audio-pins, wla-dx, fixture]

requires:
  - phase: 04-sound
    provides: the audio rows in hashes.txt, the frozen pins in tests/audio and the hash_matrix comparison (plan 04-08)
provides:
  - boot_to_play with a 20 frame boot chime, a decaying tone 1 note on each new press and Game Gear left/right toggling, picture and mailbox evidence unchanged
  - four boot_to_play audio rows (Master System and native Game Gear, baseline 60 frames and input 14 frames) in hashes.txt, frozen in tests/audio/transitions.txt and tests/audio/pcm.txt
affects: [04-10, 04-12, 04-13, 04-14, 04-15]

actuals:
  tokens: 3000
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "The sound rows of the hash matrix are one table of fields (file, profile, machine, video, region, frames, variant, input script), so a sound run with or without an input script is one more row"
    - "A fixture added to the sound rows is also required by name in the hash_matrix case, so a dropped row fails instead of shrinking the pin comparison"

key-files:
  created: []
  modified:
    - tests/roms/src/boot_to_play.asm
    - tests/built/expected.txt
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - tests/audio/transitions.txt
    - tests/audio/pcm.txt
    - docs/conformance.md

key-decisions:
  - "Chime and note state live at $C210 (chime frames left) and $C212 (tone 1 attenuation, $0F when silent); the note decay reuses the attenuation as its timer, one step a frame"
  - "The Game Gear loop back-edge becomes jp forever on both images: the added code put the old jr out of its 8-bit range"
  - "The stale-statement edit in docs/conformance.md became an addition: that file made no claim that boot_to_play changes PSG channel 0, so it now says what the fixture plays and how its audio rows are judged"

patterns-established:
  - "Adding PSG writes after the video set-up leaves every playing row, the ledger rows and the mailbox rows identical; only the image digests and the audio pins move"

requirements-completed: [SND-03]

coverage:
  - id: D1
    description: "boot_to_play plays a 20 frame boot chime on tone 0 near 440 Hz after the video set-up, with every attenuation silenced first and, on the Game Gear, $FF written to port $06 first"
    requirement: SND-03
    verification:
      - kind: integration
        ref: "build/ci/cogmaster-run build/ci/fixtures/boot_to_play.sms --frames 60 --hash-audio: exit 0, audio-pcm line with clipped 0 and peak 7357"
        status: pass
    human_judgment: false
  - id: D2
    description: "The frame-14 picture, the compat ledger rows, the mailbox rows and all 66 playing rows of hashes.txt are unchanged by the added sound; only the image digests in tests/built/expected.txt moved"
    requirement: SND-03
    verification:
      - kind: integration
        ref: "ctest compat.fixtures, fixtures.boot_to_play.*, libretro.screenshot.*, libretro.host.replay*, runner.hashes_file; diff of grep '^playing' build/ci/hashes.txt against the copy saved before the edit exits 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "Each new press starts a tone 1 note from an eight entry period table indexed by the counter's low three bits, decaying over 16 frames; on the Game Gear every channel goes left only on an even counter and right only on an odd one"
    requirement: SND-03
    verification:
      - kind: integration
        ref: "tests/audio/pcm.txt rows boot_to_play.sms sms ntsc-u input 14 and boot_to_play.gg native ntsc-u input 14; control: the same Game Gear source with the port $06 write replaced by a nop gives a different PCM hash (19f4fb16 against b43d4457), exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "hashes.txt carries audio-transitions and audio-pcm rows for boot_to_play.sms sms ntsc-u and boot_to_play.gg native ntsc-u, baseline at 60 frames and input at 14 frames with the play and play-gg scripts; hash_matrix requires all four tags"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.hashes_file (tests/runner/tests.cmake case hash_matrix, tests/cmake/hashes.cmake MODE write)"
        status: pass
    human_judgment: false
  - id: D5
    description: "tests/audio/transitions.txt and tests/audio/pcm.txt hold 8 lines each; the four new rows have clipped 0 and peak at most 29,204 (7357, 8838, 7378, 11284), and no psg_sweep line changed"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest runner.hashes_file and runner.hash_audio; git diff --unified=0 of tests/audio shows added lines only (grep -Eq '^-audio' build/pins.diff finds none, exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/conformance.md says what the fixture now plays and how its audio rows are judged, and stays under its byte budget (29,621 of 40,960)"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest docs.budgets inside cmake --workflow --preset ci"
        status: pass
    human_judgment: false
  - id: D7
    description: "The full ci workflow passes with the changed fixture, digests, rows and pins"
    requirement: SND-03
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci: exit 0, 370 of 370 tests passed"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-09
status: complete
plan_head_before: d5b4d6d59b776e42a12e1279ca4ddb155b064dc1
plan_head_after: a88a3e5c7f5e76331ffdad0287754f9a70118b6a
commits: 2
---

# Phase 4 Plan 09: boot_to_play sound and its pinned audio rows Summary

**The playable fixture now chimes at boot, plays a decaying tone 1 note on each new press and alternates Game Gear speakers by counter parity, with its picture, ledger and mailbox evidence unchanged and four audio rows frozen in tests/audio.**

## Performance

- **Duration:** about 5 min of execution after the reading
- **Started:** 2026-11-09T21:01:00Z (approximate)
- **Completed:** 2026-11-09T21:05:00Z (approximate)
- **Tasks:** 2
- **Files modified:** 7 (0 created)

## Accomplishments

- `boot_to_play.asm` writes, after the two frame waits that end the video set-up, `$FF` to `$06` (Game Gear only), the four silencing attenuation bytes, tone 0 at period `$0FE` and attenuation 0, and starts a 20 frame timer at `$C210`; the per-frame tail of the main loop counts it down and silences tone 0. Every existing instruction, counter and address is unchanged and only RAM from `$C210` up is new.
- On each 0-to-1 edge that increments the counter, tone 1 takes a period from an eight entry table indexed by the counter's low three bits (about 395 to 880 Hz) at attenuation 0; `$C212` holds the attenuation and rises one step a frame to `$0F`. Under `FIXTURE_GG` the same edge writes `$F0` (left only) for an even counter and `$0F` (right only) for an odd one to port `$06`.
- Tracer gate, run before the expansion task: the 18 picture, ledger, mailbox and replay tests passed, the saved and regenerated `playing` rows were identical (66 lines, diff exit 0), and the 60 frame audio line showed peak 7357 and clipped 0.
- `tests/cmake/hashes.cmake` now drives the sound rows from one table of fields (frames, variant and optional input script included) and adds `boot_to_play.sms sms ntsc-u` and `boot_to_play.gg native ntsc-u`, each baseline (60 frames) and input (14 frames with `play.input` and `play-gg.input`). `hash_matrix` requires all four tags with both hashes.
- The pin files grew from 4 to 8 lines each by adding only the four `boot_to_play` lines (peaks 7357, 8838, 7378 and 11284, all clipped 0).
- `docs/conformance.md` describes the chime, the notes, the Game Gear speaker toggling and how the rows are judged.

## Task Commits

1. **Task 1 (tracer): boot_to_play chimes at boot without moving its picture** - `5e16450` (test)
2. **Task 2: Notes on input edges, Game Gear stereo toggling and the pinned rows** - `a88a3e5` (test)

**Plan metadata:** recorded in the following docs commit.

## Verification Record

Commands run from the repository root, with exit statuses.

- Tracer gate (Task 1 `<verify>`, automated only): `cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fixtures\.boot_to_play|compat\.fixtures$|libretro\.screenshot\.|libretro\.host\.replay|runner\.hashes_file$)'` (18 of 18 passed) then `build/ci/cogmaster-run build/ci/fixtures/boot_to_play.sms --frames 60 --hash-audio > build/boot_to_play.audio` and the two greps: exit 0. Tracer verified end to end, expansion proceeded.
- Playing-row diff: `grep '^playing' build/ci/hashes.txt` saved before the first edit, regenerated after Task 1 and after Task 2, compared with `diff`: exit 0 both times (identical, 66 lines). The ledger is untouched (`git status` shows no change to `tests/compat/`).
- Task 2 `<verify>` first command: `cmake --build --preset ci && ctest ... -R '^(runner\.hashes_file|runner\.hash_audio|fixtures\.boot_to_play.*|compat\.fixtures|docs\.budgets)$'`: 14 of 14 passed; the four `grep -q` checks on `tests/audio/pcm.txt` exit 0.
- Task 2 second command: `git diff --unified=0 -- tests/audio/pcm.txt tests/audio/transitions.txt > build/pins.diff && ! grep -Eq '^-audio' build/pins.diff`: exit 0 (added lines only).
- Stereo control (run once, not committed): the Game Gear source copied to the scratchpad with the `out ($06),a` toggle replaced by a `nop`, assembled and run with `--frames 14 --hash-audio --input build/ci/tests/play-gg.input`: exit 0, PCM `19f4fb16...` against the fixture's `b43d4457...`, so the speaker toggling is in the data.
- `cmake --workflow --preset ci`: exit 0, 370 of 370 tests. Hygiene also passed in both commit hooks.

## Files Created/Modified

- `tests/roms/src/boot_to_play.asm` - chime, note table, decay and Game Gear speaker toggling; `jp forever` on both images
- `tests/built/expected.txt` - new `boot_to_play` and `boot_to_play_gg` digests
- `tests/cmake/hashes.cmake` - sound rows as a field table with frames, variant and input script; four boot_to_play rows
- `tests/runner/tests.cmake` - `hash_matrix` requires the four boot_to_play tags
- `tests/audio/transitions.txt`, `tests/audio/pcm.txt` - four new frozen lines each
- `docs/conformance.md` - what the fixture plays and how its audio rows are judged

## Decisions Made

See `key-decisions`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Game Gear loop back-edge out of relative range**
- **Found during:** Task 2
- **Issue:** the Game Gear image ended its main loop with `jr forever`; the added note and decay code put the target 181 bytes away, beyond the 8-bit range, so `boot_to_play_gg` failed to assemble. A first quick check of the build output missed the failure and a stale image kept the old digest until the next build showed it.
- **Fix:** both images end the loop with `jp forever` (the Master System image already did); the two `IFDEF` branches became one line.
- **Files modified:** tests/roms/src/boot_to_play.asm
- **Verification:** the image assembles, `fixtures.boot_to_play.*` and the replays pass, digests regenerated
- **Committed in:** a88a3e5 (Task 2 commit)

**2. [Plan wording] docs/conformance.md had no stale PSG claim**
- The plan asked to replace a statement that boot_to_play already changes PSG channel 0; `docs/conformance.md` contains none (it says only that no sound claim is established by the bounded observations of line 13, which concerns another section). The what-it-plays paragraph was added instead.

---

**Total deviations:** 1 auto-fixed (1 blocking), 1 plan-wording note
**Impact on plan:** No scope change.

## Issues Encountered

- The baseline `boot_to_play.sms` transition hash after Task 2 (`3a5bdadd...`) differs from the one measured at the Task 1 gate (`e1ab4222...`) with the same count (1,720): Task 2 adds instructions to boot and to the frame loop, which shifts the write ticks. Only the Task 2 values are pinned.

## Known Stubs

None.

## Threat Flags

None. The change touches a test fixture, test CMake code and text pins; no new network, input or trust surface.

## Next Phase Readiness

- The hosted `hash-equality` job compares the new rows on seven lanes for the first time; the `runner.hashes_file` timeout (30 s) now covers 8 sound runs instead of 4, still small beside the 2.3 s local measurement, but the hosted be32 and wasm32 timings remain to be read.
- The libretro and hosted RetroArch checks can use `boot_to_play`'s sound (chime from frame 3, press notes) for their audio checks; its picture evidence is unchanged.
- `docs/conformance.md` has about 11,300 bytes of room.

## Self-Check: PASSED

- `tests/roms/src/boot_to_play.asm`, `tests/audio/transitions.txt` and `tests/audio/pcm.txt` exist; the four `boot_to_play` pcm rows are present.
- Commits `5e16450` and `a88a3e5` are ancestors of HEAD.
- `compat.fixtures`, `fixtures.boot_to_play.*`, `runner.hashes_file` and `runner.hash_audio` pass; `cmake --workflow --preset ci` exits 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
