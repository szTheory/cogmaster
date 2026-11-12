---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "10"
subsystem: fixtures
tags:
  - fixtures
  - sports-pad
  - mailbox
  - replay
status: complete

requires:
  - 08-09
provides:
  - "sports_pad.sms: a pure-assembly image that polls the pad with waits of 286, 143, 143 and 143 cycles, reports X and Y through the mailbox and shows them as the backdrop colour"
  - "fifteen mailbox rows on sms-export and sms-japan, computed at configure time from one literal delta list and compared with the CSV"
  - "a deliberate device row in hashes.cmake so the sports_pad frame is compared across lanes"
  - "a record mode in cogmaster-test-ports-devices and a replay case that plays the written script through cogmaster-run to the same frame hash"
affects: [08-11, 08-12, 08-13]

actuals:
  tokens: 7200
  tasks: 3
  commits: 3

plan_head_before: 46349598145d9535a5021e55119ff14dc9e96468
plan_head_after: 4fa80bf41a84ab05c565e5b500666cad9414f23e
commits: 3

tech-stack:
  added: []
  patterns:
    - "One literal delta list is the oracle: the runner scripts are written from it and the expected row values are recomputed from it with math() at configure time, then compared with the CSV literals, so neither can drift"
    - "Frame alignment is fixed in the guest: the first read is made in frame 1 and each later one after the frame flag, so read N sees the sum of deltas 1 to N"
    - "The idle timeout is probed at whole Z80 cycles: gaps of 511 and 512 cycles between TH writes, 1,533 and 1,536 ticks"

key-files:
  created:
    - tests/roms/src/sports_pad.asm
    - tests/roms/src/sports_pad.link
  modified:
    - tests/roms/manifest.txt
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/ports/test_devices.c
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The idle probe uses gaps of 511 and 512 cycles because a gap is a whole number of Z80 cycles (3 ticks each): 511 cycles is 1,533 ticks, the largest gap below the limit, and 512 cycles is exactly the limit"
  - "The pad control runs the same buttons with zero motion, because a script with motion for a pad is exit 2 before frame 1 (plan 08-09) and would never reach the judge"
  - "Delta magnitudes in the replay recorder stay within 100 counts, below the 127 a read delivers, so the last frame's bytes are the negated deltas and a one-count change shows in the picture"
  - "The recorder drains audio each frame like a host: the audio ring is not part of the saved state, so without it the half-way copy differs from the continuous run in state bytes"

requirements-completed: []

coverage:
  - id: C1
    description: "The Sports Pad fixture decodes X and Y over cycle-counted waits of 286, 143, 143 and 143 cycles; with scripted deltas +5/-3, +127/-128 saturated, +300/-300 and -200/+50 carried across frames and single counts, the decoded bytes equal the negated scripted sums, and the rows are recomputed from the same literal list with math() and compared with the CSV at configure time"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest fixtures.sports_pad.mailbox.export"
      status: pass
    human_judgment: false
  - id: C2
    description: "A pause with TH high of 511 cycles keeps the read phase (the next nibble is the Y high nibble) and 512 cycles starts a new read (the X high nibble of the motion that is left); the buttons TL and TR read as buttons 1 and 2"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest fixtures.sports_pad.mailbox.export"
      status: pass
    human_judgment: false
  - id: C3
    description: "The same rows hold on the Japanese console, a script missing one delta falls outside its row, and a plain pad on the port falls outside its rows"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest fixtures.sports_pad.mailbox.japan and fixtures.sports_pad.control.dropped_delta and fixtures.sports_pad.control.pad"
      status: pass
    human_judgment: false
  - id: C4
    description: "The library records a run with a Sports Pad on three TV standards and writes the 75-byte script; the runner replays it to the same last-frame hash, a state saved half way and loaded into a second instance finishes with the same picture and state bytes, and one count changed in the last record gives a different hash"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest fixtures.sports_pad.replay and fixtures.sports_pad.replay_control"
      status: pass
    human_judgment: false
  - id: C5
    description: "The sports_pad frame joins the cross-lane hash file as a device row; no existing line of hashes.txt, no audio pin and no existing data row moves"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest runner.hashes_file"
      status: pass
    human_judgment: false

duration: 70min
completed: 2026-11-11
---

# Phase 8 Plan 10: The Sports Pad from the Z80 side Summary

**An owned assembly program reads the Sports Pad with cycle-counted waits and reports the decoded bytes, the idle timeout at 511 and 512 cycles and the buttons through the mailbox; the rows are recomputed from one literal delta list, two controls fail as designed, and a script recorded on the library replays through the runner to the same frame hash.**

## Accomplishments

- `tests/roms/src/sports_pad.asm`: plain assembly written from the protocol. Each frame it drives TH low through `$3F`, waits 286 cycles, reads `$DC`, then drives TH and reads at 143-cycle waits for the other three nibbles, and joins the nibbles into X and Y. Frames 1 to 10 record the two bytes; frame 11 reads the X high nibble, pauses TH high for 511 cycles (11 + 493 + 7 write to write) and reads again; frame 12 does the same with 512; frames 13 to 15 record the buttons as the first and the last read saw them. Every later frame polls and shows `(Y & 7) << 3 | (X & 7)` as the backdrop colour in palette entry 16, so the frame hash follows the bytes. Delay blocks use a counted `DJNZ` routine and padding whose cycle totals are in the comments.
- Rows: 15 per profile on `sms-export` and `sms-japan` in `tests/diag/expected.csv` (30 rows). Rows 0B and 0C also cite the MAME timeout at pin 1d7a454ed79e.
- `tests/fixtures/tests.cmake`: the delta list (18 frames), script writers for the full script, a script with the first x delta dropped and a zero-motion script with the same buttons, the configure-time recomputation of every row with `math()` (a `FATAL_ERROR` if the CSV disagrees), the `PORT1` option of the mailbox case, four registrations (two profiles, two controls), the `replay` case and its two registrations.
- `tests/cmake/hashes.cmake`: a 14-line block in the image loop that runs an entry `<file>|<script>|<arguments>` with `--frames` equal to the script's line count on the Japanese profile and appends `playing <file> sms ntsc-j device <frame line>`; the list holds the sports_pad entry.
- `tests/ports/test_devices.c`: `record IMAGE DEVICE REGION SCRIPT FRAMES` on the public API with a splitmix sequence of motion within 100 counts a frame; it writes the runner's 75-byte lines, prints `frame N sha256 <hex>` computed the way the runner does, and finishes the run a second time from a half-way saved state in another instance, requiring the same picture and state bytes.
- `tests/ports/tests.cmake`: the devices test links `runner/sha256.c`.
- Inventories `ci` and `wasm32`: the six names with labels `fixtures,portable`.

## Negative controls

| Control | Command | Result |
|---|---|---|
| Frame 1 x delta dropped | `ctest --test-dir build/ci -R '^fixtures\.sports_pad\.control\.dropped_delta$'` | exit 0, the inner judge exits 1 with "outside its row" |
| Plain pad on the port | `ctest --test-dir build/ci -R '^fixtures\.sports_pad\.control\.pad$'` | exit 0, every record reads FFFF, "outside its row" |
| Script cut at 14 lines, `--frames 14` | `cmake -DCASE=mailbox ... -DINPUT=<14 lines> -DFRAMES=14 -P tests/fixtures/tests.cmake` | exit 1, "the status is not 00 at the frame cap (hang)" |
| Recorder writes x1 one count off what it sent (`git diff --stat` showed the edit, the build exited 0) | `ctest --test-dir build/ci -R '^fixtures\.sports_pad\.replay$'` | fails; reverted, passes |
| Last record's x1 changed by one count in a copy | `ctest --test-dir build/ci -R '^fixtures\.sports_pad\.replay_control$'` | exit 0, "differs from the recording" |

## Verification run

- Task 1: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(fixtures\.|policy\.)'` exit 0 (69 of 69); `test -s build/ci/fixtures/sports_pad.sms` exit 0; one `sports_pad` line in `tests/built/expected.txt`; 30 `sports_pad,` rows in the CSV; `grep -c 1536` on the asm prints 0; no removed line against origin/main in the three data files; `cmake --workflow --preset hygiene` exit 0.
- Task 2: `ctest --test-dir build/ci -R '^(fixtures\.sports_pad\.|fixtures\.judge\.|runner\.hashes_file)'` exit 0 (8 of 8); `git diff --numstat origin/main -- tests/cmake/hashes.cmake` prints 14 added and 0 removed; `grep -c '^playing sports_pad.sms sms ntsc-j device frame ' build/ci/hashes.txt` prints 1 and `grep -c '^playing boot_to_play.sms ' build/ci/hashes.txt` prints 32 before and after; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8793 src lines, unchanged).
- Task 3: `ctest --test-dir build/ci -R '^(fixtures\.sports_pad\.|ports\.)'` exit 0 (37 of 37); clang-format 21 dry run on `tests/ports/test_devices.c` exit 0.
- Plan: `cmake --workflow --preset ci` exit 0 (513 tests plus 4 release tests); `cmake --workflow --preset hygiene` exit 0 (48 tests); `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0.
- No push and no hosted dispatch (the plan names none). Run URLs: none for this plan.

## Task Commits

1. **Task 1: The sports_pad program, its build entry and its mailbox rows** - `70f7028` (feat)
2. **Task 2: Mailbox judging on two profiles, the controls and the cross-lane hash row** - `a29cd34` (test)
3. **Task 3: The recorded-delta replay differential** - `4fa80bf` (test)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The devices test program needed the runner's SHA-256**
- **Found during:** Task 3
- **Issue:** the record mode prints the runner's hash line, which needs `runner/sha256.c`; `tests/ports/tests.cmake` is not in the plan's file list.
- **Fix:** the `cogmaster-test-ports-devices` target adds `runner/sha256.c` and the runner include directory (the same two lines `cogmaster-test-play` uses).
- **Files modified:** tests/ports/tests.cmake
- **Committed in:** 4fa80bf

**2. [Rule 1 - Bug] The first guest read landed one frame late**
- **Found during:** Task 1
- **Issue:** a `wait_frame` before the first read returns after the core's frame cut, so read N saw deltas 1 to N+1 and the expected sums were shifted.
- **Fix:** the first read is made in frame 1 and each later read follows the frame flag, so read N sees deltas 1 to N, as the runner document says of record N.
- **Files modified:** tests/roms/src/sports_pad.asm
- **Committed in:** 70f7028

### Plan wording not followed literally

- **Idle gaps in whole cycles.** The plan says `SPORTS_IDLE_TICKS - 1` ticks; a gap is whole Z80 cycles of 3 ticks, so the probes are 511 cycles (1,533 ticks) and 512 cycles (1,536 ticks), stated in the program's comments.
- **No `-` default in the device row list.** The plan wanted a `-` script field meaning 14 frames. With the 14-line cap on the block, the field is always a script name, and the frame count is the script's line count. Plans 08-11 and 08-12 each give their entry a script (both need one for motion or per-frame pad bytes).
- **The pad control uses zero motion.** The plan says `--port1 pad` on the image; a script with motion for a pad stops at exit 2 before frame 1, so the control plays a script with the same buttons and zero motion.
- **Rows listed per profile.** 30 CSV rows (15 on each of `sms-export` and `sms-japan`) rather than `*` rows, as the plan says.

**Total deviations:** 2 auto-fixed, 4 wording notes. **Impact:** none on scope or budgets.

## Phase notes

- `sports_pad.asm` is final here. Any later edit re-runs plan 08-13's identity row (its image digest in `tests/built/expected.txt` changes with it).
- The replay compares the last frame only (the runner prints one hash line), and the picture depends on the last read; the recorder stays within 100 counts a frame so that read is the negated last delta and a one-count change shows. A divergence earlier in a run that leaves no mark on the last frame would not show here; the state-bytes comparison in the recorder covers that.
- The audio ring is not in the saved state, so state-byte equality between a continuous run and a restored copy needs the host to take its audio each frame, as the recorder does.

## Known Stubs

None.

## Threat Flags

None. The record mode reads one image file and writes one script file named on its command line, in a test program; it adds no surface to the library or the runner.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-11 (paddle fixture); it appends one entry to the `hashes.cmake` row list and reuses the `replay` case with `DEVICE=paddle` and `REGIONS=ntsc-j`. The branch has the 08-05 to 08-10 commits locally (pushed through `b05c7b4`).

## Self-Check: PASSED

- The two created files and the nine modified files exist; commits `70f7028`, `a29cd34` and `4fa80bf` are ancestors of HEAD.
