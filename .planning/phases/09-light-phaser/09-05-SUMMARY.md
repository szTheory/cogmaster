---
phase: 09-light-phaser
plan: "05"
subsystem: ports-gun-state-and-fuzz
tags:
  - gun
  - state
  - fuzz
  - tests
  - ci
status: complete

requires:
  - 09-03
  - 09-04
provides:
  - ports.state.gun, a separate case that bounds every byte of the six gun fields, the aims and the devices of a phaser mid-picture against an independent checker, with a just-outside and cross-check table
  - the pad fuzz target drawing the Light Phaser, the field's edge aims, a paint op, an unsupported-mode op and a twin instance whose time steps are cut into polled pieces
  - four phaser seeds that reach a lit sensor, both ports at once, the unsupported mode, a reset and a device change under a scan
  - ports.gun.noshot at about a third of its cost, with its proof unchanged
affects: [09-08, 09-12]

actuals:
  tokens: 6554
  tasks: 2
  commits: 3

plan_head_before: a9223e03941ca7b8fe3247b31038024ce0e3a274
plan_head_after: 3214b5c4defc8b613e423fb01b5ffd6a79742569
commits: 3

tech-stack:
  added: []
  patterns:
    - "A schedule-independence leg runs every fuzz operation on the instance and on a twin whose time steps are cut into at most 16 pieces with a poll between them, and compares the two saved states byte for byte through a per-byte mask built from the record's names"
    - "A table of refusals carries beside each group the accepted row that satisfies the one rule, so a refusal is that rule and not the patch"

key-files:
  created: []
  modified:
    - tests/ports/test_state.c
    - tests/ports/tests.cmake
    - tests/ports/test_gun.c
    - tests/fuzz/fuzz_pad.c
    - tests/fuzz/seeds.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The twin's polls are the internal reads of $DC, $DD, $7E and $7F, not the bus read entry, because bus_in moves the saved bus data and timestamps; the compare masks the read flag, the lag and open-bus counts, the checksum and bit 1 of the gun's sense byte, which is the one thing a read of $DD takes"
  - "A fuzz frame first runs time to the frame's last tick and brings the sensor up to the beam, as the end of a frame does, because a device change applied at run_frame from a mid-window state drops the dots a lazy sensor has not yet looked at"
  - "ports.gun.noshot runs 160 frames per profile, the first frame count past the last new (instruction, overshoot) pair on every profile, instead of 600"

requirements-completed: []

coverage:
  - id: C1
    description: "Every byte of the six gun fields, the active and pending aims and the devices of two phasers mid-picture, lit and seen, set to 0x00, 0x01, 0x7F, 0x80, 0xFF and the complement, sealed and loaded: a refusal leaves the instance byte-identical, an acceptance equals the checker, saves back to the patched bytes and runs a frame"
    requirement: GUN-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.state\\.gun$' exit 0 (96 loads: 50 accepted, 46 refused over the 16 bytes of the six fields on two ports); widening the port_gun_dot bound to 65535 in src/state.c turns it red (exit 8)"
      status: pass
    human_judgment: false
  - id: C2
    description: "Each bound just outside and each cross-check of the saved gun state violated once is refused, with the accepted row beside it, and an aim at the far corner, lit, at the closing dot round-trips and runs a frame"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.state\\.gun$' exit 0 (31 rows: g_ax 257, g_ay 241, g_line 240, g_dot 257, g_sense 4, g_scan 2, a pending aim of 1 0, an aim on a pad port pending and in force, g_scan 0 with g_line 5, g_dot 3 or the light on, a held pad with a non-zero block, a held phaser with step 1, an edge, an acc or a pos)"
      status: pass
    human_judgment: false
  - id: C3
    description: "The pad fuzz target draws the Light Phaser and the canonical aim edges, checks that a stored aim equals the model's canonicalisation and that a refusal leaves the pending input and the port blocks unchanged, reaches a lit sensor with a paint op, selects an unsupported video mode with the display on, and holds every gun invariant and the sensor's work bound after every operation"
    requirement: GUN-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^fuzz\\.regress\\.' exit 0 (five targets, no sixth); 2,550 random and mutated streams replayed under the plain build and 840 mutated plus the seeds under cc -fsanitize=address,undefined, all exit 0"
      status: pass
    human_judgment: false
  - id: C4
    description: "The saved state does not depend on the poll schedule for one gun or for two: the instance and a twin whose time steps are cut into pieces with a poll between them save the same bytes at every frame, apart from the counters and the seen bit a poll moves and, after the unsupported mode, the gun's own fields"
    requirement: GUN-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^fuzz\\.regress\\.pad$' exit 0; a merge in src/gun.c that takes the first due port instead of the earlier dot turns it red (exit 8) on the two-gun band seed (replay exit 134), and ports.gun.walk red as well"
      status: pass
    human_judgment: false
  - id: C5
    description: "ports.gun.noshot proves what it proved before at about a third of the cost: four profiles, a dark aim and a parked aim byte-equal to a pad run in picture, sound and saved state apart from the masked gun names, a lit aim that differs and a pulled trigger that changes the image"
    requirement: GUN-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.' exit 0 (17 tests); the native run fell from 2.74 s to 0.79 s; counting the dark dots as lit in src/gun.c turns noshot red (exit 8)"
      status: pass
    human_judgment: false
  - id: C6
    description: "Nothing else moved: both workflows pass, the counted check code is unchanged, no existing pin or fuzz inventory name changed and the sources are untouched"
    requirement: GUN-01
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci exit 0 (563 of 563); cmake --workflow --preset hygiene exit 0 (50 of 50); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake tests/inventory/fuzz.txt exit 0; git diff --quiet HEAD -- src exit 0"
      status: pass
    human_judgment: false

duration: 75min
completed: 2026-11-11
---

# Phase 9 Plan 5: The gun's saved state and the pad fuzz target Summary

**`ports.state.gun` bounds every byte of the six gun fields, the aims and the devices of two phasers mid-picture against an independent checker with a just-outside and cross-check table, and `fuzz_pad.c` now draws the phaser, the field's edge aims, a paint op and an unsupported mode and runs every operation on a twin whose time steps are polled in pieces, so the saved state is shown independent of the poll schedule for one gun and for two; `ports.gun.noshot` costs a third of what it did.**

## Performance

- **Duration:** about 75 min
- **Completed:** 2026-11-11
- **Tasks:** 2 plan tasks plus the orchestrator's extra task (3 commits)
- **Files:** 7

## Accomplishments

- `ports.state.gun` in `tests/ports/test_state.c` builds two phasers over a picture that is bright all over (VRAM and registers poked, both TH lines inputs), aimed at (100,100) and (104,100), and stops the beam inside the first chord of each disc: both ports hold `g_scan` 1, `g_sense` 3, an aim in the pending, in-force and snapshot records and a cursor, and the case checks that before it uses the state. It reuses the plan-03 independent `model_ok`, so the checker is shared with `src/state.c` by nothing. The per-byte mutation was factored out of the existing case (`mutate_item`) and the row judge out of `outside` (`judge_outside`), both with the same behaviour, so the new case adds no second copy of the load-and-judge logic. It sits outside the `cfgs` table on purpose: adding a profile there would have lengthened the 80 s mutation case.
- The table has 31 rows. Beside each refusal sits the accepted row with that one rule satisfied (g_ax 256, g_ay 240, a scan at row 191 against 192, g_scan 0 with the seen bit alone, a pad on the port with nothing left), so a refusal is the rule and not the patch.
- `fuzz_pad.c`: device 4 is drawn twice in the eight-entry table (5 and 255 stay illegal); the axis table is 20 values with 256, 257, 240 and 241 added behind the old sixteen so existing selectors keep their meaning; `stored_aim` is the independent canonicalisation after each OK call; a refusal now also leaves the two port blocks unchanged; `OP_PAINT` writes a CRAM entry, register 7 and display off through `cogmaster__bus_out`; `OP_BAD_MODE` selects mode bits the core does not draw with the display on; `check_pad` bounds the sensor's samples by 2 x (113 + 13) per picture run, the disc counted by distance.
- Leg B is `operate`: every op runs on the instance and, in step, on a twin built from the same stream where time advances in at most 16 pieces with a poll (`$DC`, `$DD`, `$7E`, `$7F`, the two that sync the sensor drawn more often) after each piece. At every frame the two saved states are compared byte for byte through a mask built once per profile from the record's names.
- Four seeds in `tests/fuzz/seeds.c`: a two-gun band (the first port's aim far right of the second's, so its edges are the later ones in every row, and the disc goes by with nothing reading), the field's edges after a parked gun, the unsupported mode in the middle of a picture, and a split run with a reset, a repaint and a device change under a scan.

## Task Commits

1. **Extra task: ports.gun.noshot cut for the 32-bit lane** - `b1eda3f` (test)
2. **Task 1: ports.state.gun** - `f41411f` (test)
3. **Task 2: the gun legs of the pad fuzz target and its seeds** - `3214b5c` (test)

**Plan metadata:** the commit that carries this file.

## The no-shot case's cost on the 32-bit lane

Hosted dispatch 1 measured `ports.gun.noshot` at 124.50 s on be32 (87.20 s in the later run). It ran four profiles x (three instances for 600 frames, two for 40). Every frame is Z80 emulation of a fixed polling loop, so the cost is frames.

What 600 frames bought was every alignment of the loop against the frame. Printing the program counter and the frame's overshoot at each frame boundary shows 40 distinct (counter, overshoot) pairs on SMS1, SMS2 export and SMS2 Japan, the last new one at frame 127, and 38 on PAL, the last new one at frame 149. The remaining frames only revisit them. The case now runs 160 frames per profile (the first count past 149 with a margin), and the lit-aim and trigger controls run 8 frames instead of 40 (they differ in the first frame). The case, its name, its masked names and every assertion are unchanged; only `NOSHOT_FRAMES` and `NOSHOT_SHORT` moved, with the measurement in the comment above them.

- Native: 2.74 s to 0.79 s (3.5 x). By the same ratio the be32 lane falls from 124.50 s to about 35 s on the slow reading and from 87.20 s to about 25 s on the other.
- Timeout: back to a 60 s floor in the form `# Timeout: 60 s floor; provisional.`, to be derived from the next hosted dispatch.
- Controls: 09-04 recorded no mutation that touches `noshot` (its seven mutations name other cases), so two were run now. Counting the dark dots as lit in `look` turns `noshot` red (build exit 0, ctest exit 8, reverted green). Rendering ahead on every sync, which the case is not meant to catch, stays green both before and after the cut (run against the 600-frame file too), so nothing was lost there.
- 600 frames is the number in CONTEXT decision 6 and in the coverage of plan 04; the measurement above is why this plan changes it. The alignments, not the count, were the claim.

## Evidence

All from the repository root, no `cd`. No hosted dispatch was made in this plan (it names none); the hosted asan, be32 and wasm32 reading of the new fuzz leg and of `ports.state.gun` comes with the next dispatch.

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci` | 0, no warning |
| `ctest --test-dir build/ci -R '^ports\.state\.'` (6 tests, `gun` 0.62 s) | 0 |
| `ctest --test-dir build/ci -N -R '^ports\.state\.'`: `ports\.state\.gun$` 1, `ports\.state\.` 6 | 0 |
| `ctest --test-dir build/ci -R '^fuzz\.regress\.'` (5 tests, `pad` 0.32 s) | 0 |
| `ctest --test-dir build/ci -R '^ports\.gun\.'` (17 tests) | 0 |
| `docker run ... gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I src -I include -I build/ci/generated -I tests` on `tests/ports/test_state.c` | 0 |
| the same with `-I tests/fuzz -D_POSIX_C_SOURCE=200809L` on `tests/fuzz/fuzz_pad.c tests/fuzz/seeds.c` | 0 |
| `clang-format --dry-run -Werror` on the four changed C files | 0 each |
| `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` build of `src/*.c`, `replay.c` and `fuzz_pad.c`; the four gun seeds and `pad-ops.bin`, then 840 mutated seeds | 0 |
| plain replay of 1,500 random streams and 1,050 and 840 mutated seeds | 0 each |
| `cmake --workflow --preset ci` | 0 (563 of 563) |
| `cmake --workflow --preset hygiene` | 0 (50 of 50) |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (no counted check file edited) |
| `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` | 0 |
| `git diff --quiet origin/main -- tests/inventory/fuzz.txt` | 0 |
| `git diff --quiet HEAD -- src` after each control | 0 |
| `git diff -U0 origin/main -- tests/ports` and `tests/fuzz`, planning IDs in the added lines | 0 found in each |

Counts: `OP_PAINT` in `fuzz_pad.c` 2; `fuzz.regress.` registered 5; `ports.state.gun` in `ci.txt` and `wasm32.txt` 1 each, both inventories sorted. `tests/fuzz` is 68,911 bytes against the 524,288 cap (56,894 by the same sum before this plan: the sources, `tests.cmake` and every file there). The pad seeds are 272 to 507 bytes each. `fuzz.regress.pad` runs in 0.32 s locally, under the 3 s that would have moved its timeout, so `tests/fuzz/tests.cmake` is byte-equal; the be32 lane multiplies that by about 8 and the 10 s floor holds, which the next hosted reading confirms.

One-shot negative controls (python edit confirmed by `git diff --stat HEAD -- src`, rebuild exit 0, then the named case; each reverted and re-run green). ctest exits 8 when a case fails:

| Mutation | Case | Result |
| --- | --- | --- |
| the `port_gun_dot` bound in `src/state.c` widened to 65535 | `ports.state.gun` | ctest exit 8, `accepted a state the checker refuses`; reverted, the six `ports.state.` cases exit 0 |
| the sensor merge takes the first due port instead of the earlier dot, in `src/gun.c` | `fuzz.regress.pad` | ctest exit 8, the band seed aborts (replay exit 134) on the twin compare; `ports.gun.walk` exit 8; reverted, exit 0 |
| a dark dot counted as lit in `look` | `ports.gun.noshot` | ctest exit 8; reverted, exit 0 |

The first attempt at the merge control did not build (an unused variable under `-Werror`), so the old binary ran and passed; it was redone with a build exit of 0. The control then still passed on the first two seed layouts, which is why the band seed changed: with the aims 4 dots apart, or with the beam not yet at the disc when the reads came, the twin's slices put both ports' edges of the last row in one group and the sequential merge ended where the time-ordered one does. The seed now puts the two aims far apart and lets the disc go by with only time steps, so the twin's pieces separate the two ports' edges and the instance catches up in one go.

## Deviations from Plan

### Auto-fixed Issues

None in the core. `git diff HEAD -- src` is empty.

### Plan steps done differently

- **A finding that the leg's frame op had to work around.** The first run of the twin aborted on the band seed: at a frame, port 2's block had been cleared in the instance and not in the twin. A device change is applied when `cogmaster_run_frame` starts. The fuzz moves time inside a frame's window and then runs a frame, so the frame starts mid-window, and the instance, which had not been polled, still had the picture's dots to look at when the change dropped the port's block; the twin, polled, had looked at them. A host never calls `run_frame` mid-window (each call ends with the sensor brought up to the beam and the saved states are taken between calls), so this is not reachable through the public API and no source change was made. The frame op now brings the sensor up to the beam before `run_frame`, as the end of a frame does, and its comment says why. A core that synced the sensor at the start of `run_frame` as well would remove the need; that is a design change to the five sync sites and is left for the owner to weigh.
- **The compare masks two more things than the plan names.** The plan names `input_read`, the open-bus and lag words and, after the unsupported mode, the `port_gun_*` fields. A read of `$DD` takes the pulse bit by design, so the twin, which polls more, differs in bit 1 of `port_gun_sense`; plan 04's `walk` case leaves the same bit out. The checksum is derived from every field including the masked ones. Both are masked by name, bit 1 only for the sense byte.
- **The twin's polls are the internal reads, not the bus read entry.** `cogmaster__bus_in` writes the saved bus data and its two timestamps, which the instance, which does not poll, would not match. The reads used are `cogmaster__io_in` for `$DC` and `$DD` and `cogmaster__vdp_read` for the two counters, as `ports.gun.hammer` does.
- **The fault record is compared with the twin, not with a run with the gun detached.** The plan's wording for the unsupported-mode op also asks for equality with a detached run; that is what `ports.gun.fault_neutral` proves deterministically, and a third instance with the same op list cannot be built because the ops change the devices. The twin compare shows the record does not depend on when the sensor looks, which is what a fuzz stream can add.
- **The sensor's work bound counts pictures by operations.** `check_pad` allows 2 x 126 samples for each frame, reset or time step the input ran, plus two; a runaway rescan fails it and a loose count cannot fail a correct run.
- **Four seeds, not five.** The parked gun is the first step of the edges seed.
- **The plan's gcc command lacks `-I build/ci/generated`** (`cogmaster_version.h` is generated); it was added.
- **Two table entries of the device list now name the phaser** (the repeated Sports Pad and pad entries), so a draw is a phaser about a quarter of the time; the other six selectors keep their meaning.
- **`MAX_PATCH` is 14, not 3**, for the table rows that reset a whole port.

**Total deviations:** 0 auto-fixed, 8 narrowings or clarifications inside the plan's files, one finding recorded. **Impact:** none on behaviour or on a requirement; the SUMMARY of plan 04's `noshot` claim of 600 frames is replaced by 160 with the measurement.

## Known Stubs

None.

## Threat Flags

None. The edits are tests; the one new surface in the tree is an in-test frame op that calls an internal sync.

## Self-Check: PASSED

- `tests/ports/test_state.c`, `tests/ports/tests.cmake`, `tests/ports/test_gun.c`, `tests/fuzz/fuzz_pad.c`, `tests/fuzz/seeds.c` and both inventories exist with the described edits; commits `b1eda3f`, `f41411f` and `3214b5c` are ancestors of HEAD.
- Every acceptance criterion of both tasks was re-run and passes; `git diff --quiet HEAD -- src` exits 0.
