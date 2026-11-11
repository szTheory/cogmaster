---
phase: 09-light-phaser
plan: "07"
subsystem: fixtures-gun
tags:
  - fixture
  - gun
  - tests
  - hosted
status: complete

requires:
  - 09-04
  - 09-06
provides:
  - light_phaser.sms, an owned pure-assembly image that polls both TH bits at ten loop lengths and reports 28 mailbox records
  - a configure-time oracle in tests/fixtures/tests.cmake that derives every expected row from the image's positions and the sensor's rule, and fails configure when tests/diag/expected.csv differs
  - eight fixture tests (four profiles, port 2, three controls), runner.gun.crop, runner.gun.noshot and one cross-lane device hash row
  - the settled timeouts of the fixture, runner and 09-05 gun cases from hosted dispatch 2
affects: [09-08, 09-12]

actuals:
  tokens: 8700
  tasks: 3
  commits: 3

plan_head_before: 5a6e296d886ba23551b798eb0487cf63b7d0b76f
plan_head_after: ade085d46f26bacefc87651f0bfee86bf667fa6a
commits: 3

tech-stack:
  added: []
  patterns:
    - "An oracle in CMake that re-derives the sensor's rule (disc, first dark dot, H of the release tick) from the image's own .DEFINEs and compares every CSV literal, so a pin is never taken from the core"
    - "A mailbox profile label (port1, port2) that selects a row of the same CSV, so a run with the gun on port 2 judges the one row that differs"

key-files:
  created:
    - tests/roms/src/light_phaser.asm
    - tests/roms/src/light_phaser.link
  modified:
    - tests/roms/manifest.txt
    - tests/built/expected.txt
    - tests/fixtures/tests.cmake
    - tests/diag/expected.csv
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/ports/tests.cmake

key-decisions:
  - "The square is at (96, 40), the dark square at (96, 104) and the line at row 152 with the aim at x 128; the square gives H 73 (0x49) and the line H 84 (0x54), so a shape error cannot hide behind an edge error"
  - "The set-up ends before the second frame flag and waits for two, so the first part runs in frame 4; the ten loop lengths take frames 5 to 14, the dark part 15, the lit run 16 and the line 17, and the script lines carry those numbers from the source's PHASE_ defines"
  - "Rows are judged on the profile labels port1 and port2 so the one row that names the TH bit (1A) differs between a gun on port 1 and a gun on port 2, and every other row is the wildcard"
  - "ports.state.gun is settled at 20 s, the floor of the other ordinary cases in tests/ports/tests.cmake, not at its provisional 120 s, because its slowest hosted time is 4.52 s"

requirements-completed: []

coverage:
  - id: C1
    description: "A self-authored image lights a 16 by 16 white square, a dark grey square and a one dot line, polls TH on both ports at ten loop lengths (29, 31, 47, 64, 97, 100, 101, 200, 114 and 228 cycles) and reports 28 records under the cap of 60, judging nothing"
    requirement: GUN-04
    verification:
      - kind: integration
        ref: "cmake --build --preset ci exit 0; ctest --test-dir build/ci -R '^(fixtures\\.|cart\\.fixture|runner\\.hash|compat\\.)' exit 0 (90 tests); cogmaster-run on the image with --port1 lphaser and the phaser script prints status=00 and 28 records"
        status: pass
    human_judgment: false
  - id: C2
    description: "The Z80-visible latch is judged against a closed form derived in CMake, never taken from the core: value A and value B at every length equal (SQ_X + 16 + 35) >> 1 plus or minus 1, the oracle asserts the light ends at SQ_X + 16 on all 13 disc rows, and the line gives (BAND_X + 6 + 35) >> 1; the rows pass on the export, Japanese, PAL and first-model profiles and with the gun on port 2"
    requirement: GUN-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.light_phaser\\.' exit 0 (eight tests); the same eight tests Passed on all nine hosted ctest lanes of run 43010180943"
        status: pass
    human_judgment: false
  - id: C3
    description: "A latch that is late, a missing sticky bit, an aim 16 dots off, no aim and a pad on the port each fall outside a row, so the rows are not vacuous"
    requirement: GUN-04
    verification:
      - kind: integration
        ref: "fixtures.light_phaser.control.left, .parked and .pad exit 0 under expect_fail with the regex 'outside its row'; one-shot python edits: GUN_DELAY_TICKS 38 to 54 turned the export mailbox test red (record 02=004D, build exit 0) and sensor_low(m, 3u) to 1u in src/io.c turned it red at lengths 47, 97, 101, 114 and 228 (records FFFF), each reverted with git checkout of that file and the cases green again"
        status: pass
    human_judgment: false
  - id: C4
    description: "The runner's script aim is the field dot: the fixture's record 0C is in its row with the script aim and outside it with the same script 16 dots left; and a phaser that is parked or aimed at the dark square prints the same frame hash, lag line and mailbox records as a pad run of the same script with its aim columns zeroed, while the aim on the lit square prints different records"
    requirement: GUN-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.gun\\.(crop|noshot)$' exit 0; both Passed on every hosted ctest lane of run 43010180943"
        status: pass
    human_judgment: false
  - id: C5
    description: "The fixture's frame is the same on every lane: one device row of the cross-lane hash file runs the image with the phaser script on all eight lanes and the byte-for-byte comparison passes"
    requirement: GUN-04
    verification:
      - kind: e2e
        ref: "hash-equality job of https://github.com/szTheory/cogmaster/actions/runs/43010180943 concluded success; the artifact line 'playing light_phaser.sms sms ntsc-j device frame 40 sha256 0043ed63...' is in the hashes file of the linux-x64 lane"
        status: pass
    human_judgment: false
  - id: C6
    description: "Nothing else moved: both workflows pass, the counted check code is unchanged at 723, no pinned line was removed, and the existing latch, audio and test-card pins are byte-equal to origin/main"
    requirement: GUN-04
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines); git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c exit 0; the diff of tests/built/expected.txt, tests/diag/expected.csv and tests/roms/manifest.txt has 0 removed lines"
        status: pass
    human_judgment: false

duration: 95min
completed: 2026-11-11
---

# Phase 9 Plan 7: The Light Phaser fixture and hosted dispatch 2 Summary

**`light_phaser.sms` polls both TH bits at ten loop lengths against a lit square, a dark square and a one dot line, and its latched H (0x49 for the square, 0x54 for the line) is judged on four profiles and both ports against a closed form that CMake derives from the image's own positions; three controls fail, the runner's script aim is shown to be the field dot, a parked or dark-aimed gun equals a pad run, and the first hosted view of all of it is green on every lane.**

## Performance

- **Duration:** about 95 min, most of it the hosted run
- **Completed:** 2026-11-11
- **Tasks:** 3
- **Files:** 11

## Accomplishments

- `tests/roms/src/light_phaser.asm` (pure assembly, `ROM_ID` 14) draws the picture once and then measures one part a frame. The 29 and 31 cycle periods are unrolled routines of `IN`, `CPL`, `AND`, `RET NZ` and a `NOP`, whose periods are 29 (with `IN A,(C)` and `AND D`) and 31; the other eight are a padded `DJNZ` loop of 37 plus the pad, with 114 and 228 dividing the 228 cycle line on purpose. A routine returns NZ when TH read low and Z after 256 polls, and `take` then reads V, `$7F`, waits for TH to return high and reads `$7F` again.
- The records are `01` (parked), `02-0B` and `0C-15` (value B and value A at ten lengths), `16` (dark), `17` (lit, trigger never pulled), `18-19` (V at the poll), `1A` (the TH bit that saw light) and `1B-1C` (the line). A part that finds no light records `$FFFF`.
- The oracle in `tests/fixtures/tests.cmake` parses `SQ_X`, `SQ_Y`, `DARK_X`, `DARK_Y`, `BAND_X`, `BAND_Y` and the `PHASE_` frames from the source, re-derives the disc's half widths and the dot the light ends at for each of the 13 rows and fails configure unless every one is `SQ_X + 16`, derives H from it, builds the 29 expected rows and fails configure when `tests/diag/expected.csv` holds any other literal or a different count.
- The same block writes five scripts (phaser, port 2, 16 dots left, parked, dark aim) with `file(CONFIGURE ... NEWLINE_STYLE UNIX)` and the existing `sports_axis` helper, 40 lines of 75 bytes each; it adds no `file(WRITE)` (11 before and after).
- The `mailbox` case passes `PORT2` beside `PORT1`. Eight fixture tests, `runner.gun.crop` and `runner.gun.noshot` are registered on every lane and sit in `ci.txt` and `wasm32.txt` in sorted position; one device row in `hashes.cmake` equates the frame across lanes.

## Task Commits

1. **Task 1: the image, its link file, its digest and its manifest line** - `decd88d` (test)
2. **Task 2: the oracle, rows, scripts, profiles, controls, crop and no-shot cases, hash row** - `ddb4059` (test)
3. **Task 3: timeouts settled from hosted dispatch 2** - `ade085d` (test)

**Plan metadata:** the commit that carries this file, `STATE.md` and `ROADMAP.md`.

## Files Created/Modified

- `tests/roms/src/light_phaser.asm`, `tests/roms/src/light_phaser.link` - the image
- `tests/built/expected.txt`, `tests/roms/manifest.txt` - its digest and owned-source line
- `tests/fixtures/tests.cmake` - the oracle, the scripts, ten registrations, the `PORT2` option, the image in the build list
- `tests/diag/expected.csv` - 29 `light_phaser` rows
- `tests/cmake/hashes.cmake` - one device row
- `tests/runner/tests.cmake` - `gun_crop` and `gun_noshot` cases and their registration
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - ten names each
- `tests/ports/tests.cmake` - the `ports.gun.noshot` and `ports.state.gun` timeouts

## Decisions Made

- Square at (96, 40), dark square at (96, 104), line at row 152 with the aim at x 128: the three are at least 14 rows from each other, every `ay` is 49, 113 or 153, and the square's H (73) differs from the line's (84).
- The profile column of the new rows is `*` except row `1A`, which carries `port1` (0040) and `port2` (0080); the four console tests judge as `port1` and the port 2 test as `port2`.
- The program's own `E` register held the TH bits across a `mb_record` call, which clobbers `DE`; the count record now saves and restores it (found in the first run, before the first commit).

## Evidence

All from the repository root, no `cd`.

| Command | Exit |
| --- | --- |
| `cmake --build --preset ci` then `ctest --test-dir build/ci -R '^(fixtures\.\|cart\.fixture\|runner\.hash\|compat\.)'` (90 tests) | 0 |
| `ctest --test-dir build/ci -R 'light_phaser\|runner\.gun\|ports\.gun'` (29 tests) | 0 |
| `cmake --workflow --preset ci` (after task 2, and again after task 3) | 0 |
| `cmake --workflow --preset hygiene` (after task 2, and again after task 3) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` prints 723 check lines | 0 |
| `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c` | 0 |
| `git diff origin/main -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt`, removed lines counted | 0 removed |
| `ctest -N -R 'light_phaser\|runner\.gun'` lists the 8 fixture names and `runner.gun.crop`, `runner.gun.noshot` | 0 |
| `grep -c 'file(WRITE' tests/fixtures/tests.cmake` against the `origin/main` copy | 11 and 11 |
| planning IDs in the added lines of `tests/fixtures tests/runner tests/cmake tests/diag` and in the `.asm` | 0 |
| `LC_ALL=C sort -c` on both inventories | 0 |

The image's local readings on the phaser script, for the record: value A and value B 0x49 at all ten lengths, V 0x2A (row 42), lit count 13 and TH bit 0x40 on port 1 (0x80 on port 2), and 0x54 on the line. The control runs read 0x43 at every length for the aim 16 dots left (6 counts off), `FFFF` for the parked and pad runs.

One-shot mutation controls (python edits, confirmed with `git diff --stat -- src`, rebuild exit 0 before the result was trusted, reverted with `git checkout` of the single file, then the cases green again):

| Edit | Result |
| --- | --- |
| `GUN_DELAY_TICKS` 38 to 54 in `src/internal.h` | `fixtures.light_phaser.mailbox.export` red, every `02-15` record 004D against 0048-004A; reverted, exit 0 |
| `sensor_low(m, 3u)` to `sensor_low(m, 1u)` in the `$DD` readback of `src/io.c` | red: `FFFF` at lengths 47, 97, 101, 114 and 228 in both rows, `17` and `1A` 0000, and the line's rows; reverted, the 29 gun cases exit 0 |

The first edit moves the latch by 16 ticks, which is 4 H counts, against a tolerance of 1: well outside. The plan text calls that 8 counts away; the arithmetic of one count per 4 ticks gives 4, still above the 3.5 the tolerance derivation requires.

## Hosted dispatch 2

Commands:

| Command | Exit |
| --- | --- |
| `git push origin phase/09-light-phaser` (head `ddb40594f236c17d0d0381a968f1d895d6b730fd`; it carried the 09-06 commits, whose runner code and cases had no hosted view before, with it) | 0 |
| `gh workflow run ci.yml --ref phase/09-light-phaser` | 0 |
| `gh run list --workflow ci.yml --branch phase/09-light-phaser --event workflow_dispatch --json databaseId,headSha,url`, then `jq` for the run whose `headSha` is the head: prints 43010180943 | 0 |
| `gh run watch 43010180943 --exit-status` | 0 |
| `gh run view 43010180943 --json conclusion --jq .conclusion` equals success | 0 |
| `jq -e` over the jobs named hash-equality, wasm32, be32, asan or nofp: at least five, all success | 0 |

Run: https://github.com/szTheory/cogmaster/actions/runs/43010180943 at head `ddb40594f236c17d0d0381a968f1d895d6b730fd`, concluded success on the first attempt (no re-dispatch). Jobs: changes, hygiene, asan, nofp (ubuntu-26.04), nofp (ubuntu-26.04-arm), be32, wasm32, build on linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64 and windows-arm64, perf, retroarch, hash-equality and CI required all success; title, cleanup and rehearsal skipped as allowed. The perf job printed `perf control boot_to_play runs=12303209,12303209,12303209 spread=0`; this plan computes no delta.

The `hash-equality` job carries the `light_phaser.sms` row: it compared the hashes file of eight lanes byte for byte, and the file of the linux-x64 lane holds `fixture light_phaser.sms sha256 0135e12e...320c` (the digest in `tests/built/expected.txt`) and `playing light_phaser.sms sms ntsc-j device frame 40 sha256 0043ed635241b36f1a0a5864dba62b8268c15d95c581c47cf082426dcef44258`, the same value the local run printed. be32 passed 503 tests and wasm32 469.

### Measured test durations

Every one of the 14 cases below Passed on all nine ctest lanes (asan, be32, wasm32 and six build legs, 126 readings). Seconds, the slowest lane of the run; it was be32 for every case.

| Case | be32 | asan | wasm32 | slowest other lane | Timeout set |
| --- | --- | --- | --- | --- | --- |
| `fixtures.light_phaser.mailbox.{export,japan,pal,sms1}`, `.port2` | 0.37 to 0.41 | 0.07 | 0.08 | 0.14 | 10 s floor |
| `fixtures.light_phaser.control.{left,parked,pad}` | 0.37 to 0.38 | 0.07 | 0.08 | 0.18 | 10 s floor |
| `runner.gun.args` | 1.16 | 0.26 | 0.26 | 0.40 | 30 s floor |
| `runner.gun.gg_refused` | 0.18 | 0.04 | 0.05 | 0.12 | 30 s floor |
| `runner.gun.crop` | 0.70 | 0.11 | 0.13 | 0.14 | 30 s floor |
| `runner.gun.noshot` | 1.43 | 0.24 | 0.23 | 0.41 | 30 s floor |
| `ports.gun.noshot` | 34.46 | 5.27 | 6.28 | 10.76 (macos-x64) | 69 s |
| `ports.state.gun` | 4.52 | 2.03 | 2.05 | 3.27 (macos-x64) | 20 s floor |

`ports.gun.noshot` on be32 is 34.46 s after plan 05's cut from 600 to 160 frames, against 124.50 s before (the plan expected about 35 s). Its timeout is 2 x 34.46 s rounded up, 69 s, and the comment drops the word floor. The comments in the two plan files are rewritten in the repository's form (`2 x <S> s, slowest of 1 run, run 43010180943`), and no `floor; provisional` remains in `tests/fixtures/tests.cmake` or `tests/runner/tests.cmake`.

The dispatched head `ddb4059` differs from the head that carries the timeouts (`ade085d`) in comments and `TIMEOUT` values of three files only: `tests/fixtures/tests.cmake`, `tests/runner/tests.cmake` and `tests/ports/tests.cmake` (the third is outside the plan's file list; see the deviations). `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0 on `ade085d`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The count record clobbered the TH bit register**
- **Found during:** Task 1, the first run of the image
- **Issue:** `mb_record` changes `DE`, so record `1A` (the TH bit that saw light, kept in `E`) read 0x0D, the count.
- **Fix:** `push de` and `pop de` around the first record of the lit run.
- **Files modified:** `tests/roms/src/light_phaser.asm` (before its first commit)
- **Commit:** `decd88d`

### Scope extensions and clarifications

**2. [Instruction from the orchestrator] The 09-05 gun timeouts were settled in the same commit**
- `ports.gun.noshot` (60 s floor, provisional) is now 69 s and `ports.state.gun` (120 s floor, provisional) is 20 s, so the dispatched head differs from `HEAD` in `tests/ports/tests.cmake` as well as the two files the plan names. The plan's acceptance check that lists only two files would list three. `ports.state.gun` takes the 20 s floor of the other ordinary cases in that file, not its provisional 120 s, because its hosted maximum is 4.52 s.
- **Commit:** `ade085d`

**3. [Clarification] The row profile labels**
- Row `1A` has two rows (`port1` 0040, `port2` 0080) and the other 27 rows are the `*` wildcard, so the CSV holds 29 rows; the plan left the profile name for the port 2 row to the judge.

**4. [Clarification] The distance of the controls**
- The aim 16 dots left reads 0x43 (6 counts from 0x49) and the delay of 16 ticks more reads 0x4D (4 counts): both beyond the 3.5 counts the tolerance needs, though the plan's text gives 8 for the delay.

**Total deviations:** 1 auto-fixed, 3 clarifications or scope notes. **Impact:** none on behaviour or on a requirement.

## Known Stubs

None.

## Threat Flags

None. The plan adds a test image and test registrations; no network, auth or file-access surface.

## Phase notes

The last `.asm` edit of the phase lands here: `tests/roms/src/light_phaser.asm` is final at digest `0135e12e6baa46041a0226088d4e5e164a154a6e1865fa852b7acaa2d20d320c` (in `tests/built/expected.txt`).

- The square covers dots 96 to 111 and rows 40 to 55 (`SQ_X` 96, `SQ_Y` 40); the aim is `ax` 112, `ay` 49, the disc covers dot rows 42 to 54, and the light ends at dot 112 on all 13 rows, H = (112 + 35) >> 1 = 73.
- The dark square is at (96, 104) with the aim at (105, 113); the line is dot row 152 (`BAND_Y`) with the aim at (128, 153); its light ends at dot 134 and H = (134 + 35) >> 1 = 84.
- The frames: the first part runs in frame 4, parked (`PHASE_PARKED` 4); the ten loop lengths take frames 5 to 14 (`PHASE_PERIODS` 5); the dark aim is frame 15, the lit run with the trigger never pulled frame 16 and the line frame 17. The scripts are 40 lines long, the cap the tests run to; a run finds the mailbox done in frame 19.
- The V sync starts each part two rows above the first row the sensor reaches (`SQ_Y`, `DARK_Y`, `BAND_Y` minus 2), and the V counter at the poll read 0x2A (row 42, the first disc row) at lengths 29 and 64.

## Self-Check: PASSED

- FOUND: tests/roms/src/light_phaser.asm, tests/roms/src/light_phaser.link
- FOUND commits decd88d, ddb4059 and ade085d (`git merge-base --is-ancestor` each, exit 0)
