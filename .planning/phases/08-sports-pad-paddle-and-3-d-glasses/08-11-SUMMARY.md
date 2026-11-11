---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "11"
subsystem: fixtures
tags:
  - fixtures
  - paddle
  - mailbox
  - replay
status: complete

requires:
  - 08-10
provides:
  - "paddle.sms: a pure-assembly image that runs the title region test, sweeps a TR ones count at ten sample periods, and reads the position from the two nibbles once a frame"
  - "twenty-six mailbox rows: the region test on both profiles, ten sweep rows, fourteen position rows recomputed from one literal delta list with a saturating sum"
  - "the sweep verdict on the TR clock constant: kept"
  - "a Paddle device row in hashes.cmake and a replay on the paddle image on the Japan profile"
affects: [08-12, 08-13, 08-17]

actuals:
  tokens: 5700
  tasks: 3
  commits: 3

plan_head_before: 7c67311284a925d692b1de3f8e017d79e7bb00e0
plan_head_after: c0ddac181a479c62e58b1ddd377d414bebf85226
commits: 3

tech-stack:
  added: []
  patterns:
    - "One literal delta list is the oracle again: the runner scripts are written from it and every position row is recomputed with math() at configure time and compared with the CSV literals"
    - "The sweep loop is the title's own IN, BIT, JR, INC, DJNZ: 43 cycles on the 1 path and 44 on the 0 path, padded with counted NOP and LD A,0 to each period; the two shortest periods are the same four instructions unrolled with no DJNZ"
    - "Reads are bounded: the reader gives up on a TR that does not change after 64 polls and records 0xFFFF, so a frozen TR is a wrong row and never a hang"

key-files:
  created:
    - tests/roms/src/paddle.asm
    - tests/roms/src/paddle.link
  modified:
    - tests/roms/manifest.txt
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "PADDLE_HALF_TICKS stays 299: every one of the eight criterion lengths read 126 to 150 ones of 256 (49 to 59 percent), so src/internal.h is untouched"
  - "The position oracle clamps after every frame's motion, the way the core and a pot do, and not once over the whole sum; a delta list that saturates and then reverses makes the two differ, so the list does that on purpose (frame 5 reads 253, where a single clamp of the sum would read 255)"
  - "The sample period is the length of the 1 path; the 0 path is one cycle longer, as in a real loop"
  - "The export profile runs the same image with a plain pad: the image records the region test (0) and stops, which is what a title does when its region test finds no Paddle"

requirements-completed: []

coverage:
  - id: C1
    description: "The detect loop at sample periods of 29, 31, 47, 64, 97, 100, 101 and 200 Z80 cycles counts 77 to 179 ones of 256 on the Japan profile (measured 127, 134, 131, 130, 129, 127, 126 and 150); the lengths 199 and 398 are recorded as near-lock rows outside the criterion (measured 174 and 113)"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest fixtures.paddle.mailbox.japan"
      status: pass
    human_judgment: false
  - id: C2
    description: "After detection the reader waits for TR low and TR high, joins the nibbles, and the position equals the running saturating sum of the scripted x deltas from 128, with saturation at 255 and at 0, the button on TL, and a non-zero y ignored; the rows are recomputed from the same literal list with math() and compared with the CSV at configure time"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest fixtures.paddle.mailbox.japan"
      status: pass
    human_judgment: false
  - id: C3
    description: "The export profile runs the title's own region test through the TH path and records export; the Japan profile records Japan"
    requirement: PAD-02
    verification:
      kind: integration
      ref: "ctest fixtures.paddle.mailbox.export and fixtures.paddle.mailbox.japan"
      status: pass
    human_judgment: false
  - id: C4
    description: "A frozen TR (port 1 empty), a plain pad on the port and a script missing its first delta each fall outside their rows; a core clock changed to lock the period of 100 cycles fails the sweep rows"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest fixtures.paddle.control.frozen, fixtures.paddle.control.pad and fixtures.paddle.control.dropped_delta"
      status: pass
    human_judgment: false
  - id: C5
    description: "A script recorded on the library with a Paddle on the Japan profile replays through the runner to the same last-frame hash, a state saved half way finishes with the same picture and state bytes, one count changed in the last record gives a different hash, and the paddle frame joins the cross-lane hash file as a device row"
    requirement: PAD-03
    verification:
      kind: integration
      ref: "ctest fixtures.paddle.replay, fixtures.paddle.replay_control and runner.hashes_file"
      status: pass
    human_judgment: false

duration: 45min
completed: 2026-11-11
---

# Phase 8 Plan 11: The Paddle from the Z80 side Summary

**An owned assembly program reads the Paddle the way a title does: a ones count over 256 TR samples at the eight criterion loop lengths stays between 49 and 59 percent, the position rebuilt from the two nibbles follows the scripted deltas with saturation at both ends, and the tick constant stays at 299 on the evidence of the sweep.**

## Accomplishments

- `tests/roms/src/paddle.asm`: plain assembly written from the protocol. It runs the title region test through TH and `$DD` (Japan reads the outputs low, export reads them back), records it, and on the export console stops. On the Japan console it first reads the position in frames 1 to 14 (wait for TR low and keep the read, wait for TR high, join the nibbles, TL as the button; a bound of 64 polls gives `$FFFF`), then runs the sweep and shows the low six bits of the position as the backdrop colour so one count changes the frame.
- Sweep periods (the 1 path, the 0 path is one cycle more): 47 is 43 + 1 NOP; 64 is 43 + 3 x 7; 97 is 43 + 10 NOP + 2 x 7; 100 is 43 + 9 NOP + 3 x 7; 101 is 43 + 11 NOP + 2 x 7; 200 is 43 + 34 NOP + 3 x 7; 199 is 43 + 39 NOP; 398 is 43 + 87 NOP + 7. The periods 29 (IN, AND, JR, INC) and 31 (IN A,(C), BIT, JR, INC) are shorter than the DJNZ loop can be, so they are the same four instructions repeated 256 times with no DJNZ.
- `tests/diag/expected.csv`: 26 `paddle` rows (1 export, 25 Japan). `tests/fixtures/tests.cmake`: the script writer takes the list it writes as a parameter (the three Sports Pad calls follow), the 14-delta list, three scripts (full, first x dropped, no motion), the configure-time recomputation of the position rows, five mailbox registrations and two replay registrations. `tests/cmake/hashes.cmake`: one entry in the device row list. Inventories `ci` and `wasm32`: seven names, labels `fixtures,portable`.

## Sweep (evidence)

Command: `build/ci/cogmaster-run build/ci/fixtures/paddle.sms --machine sms --region ntsc-j --port1 paddle --input build/ci/tests/paddle.input --frames 26 --mailbox`, exit 0, `status=00`, `crc=ok`.

| Sample period (cycles) | 29 | 31 | 47 | 64 | 97 | 100 | 101 | 200 | 199 | 398 |
|---|---|---|---|---|---|---|---|---|---|---|
| Ones of 256 | 127 | 134 | 131 | 130 | 129 | 127 | 126 | 150 | 174 | 113 |

The criterion is 77 to 179 ones. The eight criterion lengths are 126 to 150; the two near-lock lengths (174 and 113) are outside the criterion by design and also lie inside it here. **Verdict: `PADDLE_HALF_TICKS` kept at 299; `git diff --quiet HEAD -- src/internal.h` exits 0 before and after the plan.** The documentation plan states the constant as 299 with no change.

## Negative controls

| Control | Command | Result |
|---|---|---|
| Port 1 empty (frozen TR), no motion | `ctest --test-dir build/ci -R '^fixtures\.paddle\.control\.frozen$'` | exit 0; the inner judge exits 1 with "outside its row"; position `0C=FFFF`, every count 0 |
| Plain pad on port 1, no motion | `ctest --test-dir build/ci -R '^fixtures\.paddle\.control\.pad$'` | exit 0, same message |
| Script missing the first x delta | `ctest --test-dir build/ci -R '^fixtures\.paddle\.control\.dropped_delta$'` | exit 0, "outside its row" |
| Core constant changed to 150 (a full period of 300 ticks is 100 cycles; `git diff --stat` showed the one-line edit, the build exited 0) | `ctest --test-dir build/ci -R '^fixtures\.paddle\.mailbox\.japan$'` | fails: record 07 (period 100) reads 0, record 09 reads 246, "outside its row"; reverted with `git checkout -- src/internal.h`, passes again |
| Last record's x1 changed by one count in a copy | `ctest --test-dir build/ci -R '^fixtures\.paddle\.replay_control$'` | exit 0, "differs from the recording" |

## Verification run

- Task 1: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(fixtures\.|policy\.)'` exit 0 (75 of 75); `test -s build/ci/fixtures/paddle.sms` exit 0; one `paddle` line in `tests/built/expected.txt`; 26 `paddle,` CSV rows; `grep -cE '\b(299|598)\b'` on the asm prints 0; no removed line against origin/main in the three data files; `cmake --workflow --preset hygiene` exit 0.
- Task 2: `ctest --test-dir build/ci -R '^(fixtures\.paddle\.|fixtures\.sports_pad\.|runner\.hashes_file)'` exit 0 (12 of 12 at that commit); five `fixtures.paddle.*` names in each inventory; `grep -c '^playing paddle.sms sms ntsc-j device frame ' build/ci/hashes.txt` prints 1; `git diff --numstat origin/main -- tests/cmake/hashes.cmake` prints 15 added and 0 removed.
- Task 3: `ctest --test-dir build/ci -R '^fixtures\.paddle\.replay'` exit 0 (2 of 2); `git diff --quiet HEAD -- tests/ports/test_devices.c` exit 0 (the recorder is reused); `cmake --workflow --preset ci` exit 0 (520 tests plus 4 release tests).
- Plan: `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0 (48 tests); `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8793 src lines, unchanged).
- No push and no hosted dispatch (the plan names none). Run URLs: none for this plan.

## Task Commits

1. **Task 1: The paddle program, its build entry and its mailbox rows** - `c8466e7` (feat)
2. **Task 2: Mailbox judging, the sweep decision, the controls and the region path** - `89efdeb` (test)
3. **Task 3: Replay on the paddle image** - `c0ddac1` (test)

## Deviations from Plan

### Auto-fixed Issues

None.

### Plan wording not followed literally

- **The shortest two periods are not padding.** The plan says every period comes from padding the 43/44 cycle loop with NOPs; a padded loop cannot be shorter than 43, so 29 and 31 are the unrolled form of the same IN, test, JR, INC sequence. The other eight are padded as written.
- **The position is a saturating sum, not one clamp of the sum.** The plan's row text says `clamp(128 + sum)`; the core (and a pot) stops at its ends after each frame, and the two differ once a list saturates and then reverses. The oracle clamps per frame, and the list reverses after saturating at both ends, so the difference is exercised.
- **The sweep runs after the 14 position frames**, not before: the script is consumed one record a frame, so the first read has to be in frame 1 as in the Sports Pad program. The script has 12 motionless records after the 14 deltas (26 frames; the program is done at frame 22).
- **The controls use a zero-motion script**, as in the Sports Pad plan: a script with motion for a plain pad or an empty port is exit 2 before frame 1. The export case also plays the zero-motion script on `--port1 pad`.
- **The source section is 3.6.** The plan cites section 3.4 for the Paddle; in the preparation document that is the 3-D glasses and the Paddle is section 3.6, which the rows and comments cite.
- **Position rows carry TL in the high byte** (1 when pressed); the plan names positions only, and the button costs no extra record.

**Total deviations:** 0 auto-fixed, 6 wording notes. **Impact:** none on scope or budgets.

## Phase notes

- Final `PADDLE_HALF_TICKS` is **299** and it did not move; `docs/controllers.md` (plan 08-17) states it with its derivation from the clock sources and may cite this sweep: eight lengths between 126 and 150 ones of 256, the lock at a full period of 100 cycles shown by changing the constant once.
- `paddle.asm` is final here. Any later edit re-runs plan 08-13's identity row (its image digest in `tests/built/expected.txt` changes with it).
- The sweep tests that no polling loop of these lengths locks to the clock; it does not test the clock's frequency against hardware (sources agree on about 18 kHz, `PADDLE_HALF_TICKS` is within half a percent of both), which only the owner's Paddle carts could.

## Known Stubs

None.

## Threat Flags

None. The plan adds a self-authored test image and test data; no library, runner or script surface changes.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-12 (the glasses fixture); it appends one entry to the same `hashes.cmake` row list, which now holds the sports_pad and paddle entries. The branch has the 08-05 to 08-11 commits locally (pushed through `b05c7b4`).

## Self-Check: PASSED

- The two created files and the seven modified files exist; commits `c8466e7`, `89efdeb` and `c0ddac1` are ancestors of HEAD.
