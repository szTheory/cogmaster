---
phase: 09-light-phaser
plan: "04"
subsystem: ports-gun-sensor
tags:
  - gun
  - vdp
  - io
  - tests
  - ci
status: complete

requires:
  - 09-02
  - 09-03
provides:
  - a lazy Light Phaser sensor that reads the emitted pixel, holds TH low while it sees light and latches H at the sensor's own tick
  - TH readback on $DD b6 and b7 as the live level or a pulse bit that one read or the picture's wrap takes
  - five guarded sync sites that cost a compare each when no gun is plugged in
  - thirteen new ports.gun cases with independent oracles and negative controls, on the ci and wasm32 lanes
  - the first hosted dispatch of the phase, green on every lane, with a perf delta of 0 basis points
affects: [09-05, 09-07, 09-08]

actuals:
  tokens: 15526
  tasks: 3
  commits: 5

plan_head_before: a316c5ac2e57187fbbc393b9d27b799c74b90d21
plan_head_after: 32282332e65c5185913f08c4f0cccf1712157870
commits: 5

tech-stack:
  added: []
  patterns:
    - "A device sensor is a lazy scanner that catches up to the beam in time order across both ports and renders only as far as a dot it needs"
    - "The test oracle is written from a per-dot distance test and its own luma and tick arithmetic, and a beam walker moves the bus without a CPU"

key-files:
  created: []
  modified:
    - src/gun.c
    - src/io.c
    - src/vdp.c
    - src/vdp_render.c
    - src/frame.c
    - src/internal.h
    - tests/ports/test_gun.c
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "gun_wrap runs after the height update inside the wrap branch, so it arms for the height the next picture has and the scan cursor satisfies the loader's bound at every instant a state can be saved"
  - "A sync renders ahead only when a due dot has not been emitted, so a gun that finds nothing to look at leaves the render cursor where an unplugged run has it"
  - "A sync that lands between the wrap and line 0 does nothing: the next picture is armed and has not begun, and the old rows are no longer the beam's"

requirements-completed: []

coverage:
  - id: C1
    description: "The sensor reads the emitted pixel: TH level at every tick of every row the disc meets, the latched H counter after every row and the first release edge equal an independent oracle exactly, over three picture heights, both ports and both regions"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.(model|edge|luma)$' exit 0; delay plus 16 ticks turns model red (exit 8) and a read-time latch turns edge red (exit 8)"
      status: pass
    human_judgment: false
  - id: C2
    description: "A late sensor, a read-time latch, a leading-edge latch and a sensor with no pulse bit each disagree with the core on a lit square, so the model is not satisfied by a weaker rule"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.control$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "The sensor renders early without changing a picture, a fault record, a lag counter, the open-bus count or the instance: a same-tick VRAM write, an unsupported mode, a polling frame, and cogmaster_peek with cogmaster__ports_pins"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.(order|fault_neutral|lag|peek)$' exit 0; rendering to the tick plus one turns order red and rendering under the unsupported mode turns fault_neutral red (exit 8 each)"
      status: pass
    human_judgment: false
  - id: C4
    description: "A light pulse is seen by one read after it and never by a read in a later picture, and a pulse while TH is an output sets no bit and latches nothing"
    requirement: GUN-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.seen_expiry$' exit 0; removing the wrap clear turns it red (exit 8)"
      status: pass
    human_judgment: false
  - id: C5
    description: "A game that polls every port with the gun plugged in, the trigger never pulled and nothing lit in view plays frame for frame and in the whole saved state as with a pad, apart from the gun's own fields, on four profiles for 600 frames"
    requirement: GUN-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.noshot$' exit 0; the lit aim changes the saved state and the pulled trigger changes the picture"
      status: pass
    human_judgment: false
  - id: C6
    description: "Sensor work is bounded and independent of the poll schedule for one gun and for two: a million accesses at one tick and at every tick of a picture, a read at every tick against whole-line steps, and a million TH writes latching exactly as the model's effective TH rises"
    requirement: GUN-02
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.(hammer|walk|th_toggle)$' exit 0; a sequential two-gun merge turns walk red and a row restart turns hammer red (exit 8 each)"
      status: pass
    human_judgment: false
  - id: C7
    description: "The hooks, the 24-byte block, the perf cost and the new tests are green on every hosted lane, and the perf delta against a fresh main dispatch is 0 basis points"
    requirement: GUN-02
    verification:
      kind: integration
      ref: "gh run view 43003756603 concludes success on every job; perf dispatch boot_to_play H=12303207 B=12303147 delta_bp=0"
      status: pass
    human_judgment: false
  - id: C8
    description: "Nothing else moved: the existing io, vdp, bus, audio, testcard pin and ports test sources equal origin/main, both workflows pass and the counted check code is unchanged"
    requirement: GUN-01
    verification:
      kind: integration
      ref: "git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c exit 0; cmake --workflow --preset ci exit 0 (562 of 562); cmake --workflow --preset hygiene exit 0 (50 of 50); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
      status: pass
    human_judgment: false

duration: 35min
completed: 2026-11-11
---

# Phase 9 Plan 4: The Light Phaser sensor Summary

**A lazy, bounded Light Phaser sensor in `src/gun.c` that reads the emitted pixel of a 113-dot disc in time order across both ports, holds TH low while it sees light, latches H at the dot's own tick and sets a pulse bit one read or the picture's wrap takes, behind five guarded sync sites; thirteen new cases prove it against independent oracles, and the first hosted dispatch of the phase is green on every lane with a perf delta of 0 basis points.**

## Performance

- **Duration:** about 35 min, of which most was waiting for hosted runs
- **Completed:** 2026-11-11
- **Tasks:** 3 (the tracer, the sensor and bound cases in two commits, the hosted dispatch)
- **Files:** 10

## Accomplishments

- `src/gun.c` holds the model. `half_width` finds a row's chord in at most six steps with no table; `dot_lit` is the weighted brightness of the 8-bit expansion of the finished field pixel; `run` takes, across both ports, the port whose next dot has the smaller (row, tick) with a tie to port 1, skips a dot the VDP has not emitted, and looks at it. A dot is seen `GUN_DELAY_TICKS` after its own tick and a closing dot ends the light, so the release is at the first dot that is not bright. `cogmaster__gun_wrap` finishes the old picture, drops the pulse bit on every port and arms each phaser port from the aim in force.
- `GUN_DELAY_TICKS`, `GUN_RADIUS`, `GUN_LUMA` and the three weights are named once in `src/internal.h` with MAME as the cited default and the word unmeasured; `struct cogmaster__gun_stats` (samples, syncs, edges) sits in the instance, is never saved and never feeds behaviour.
- `src/io.c` feeds the existing sensor mask: `sensor_low` builds it from the two ports' `g_sense`, `$DD` reads back the level or the pulse bit, the H latch edge uses the level alone, and `cogmaster__io_sensor_edge` flips the lit bit, sets the pulse bit when an input TH falls and calls `th_edge` with the dot's own tick. `chip_in` syncs before a `$DD` read and takes the pulse bit of each port whose TH is an input; `io_control` syncs before the write so a sensor edge and a `$3F` write at one tick go sensor first. `port_dd`, `port_dc`, `cogmaster__ports_pins` and `th_pins` stay pure.
- `src/vdp.c` syncs on an `$7F` read and calls `gun_wrap` in the wrap branch; `src/frame.c` syncs after `bus_run_frame` inside the `cart.rom` branch; `src/vdp_render.c` exports `cogmaster__vdp_render_blocked`. Every call is behind `cogmaster__gun_attached`, one inline predicate of two loads. The diff adds nothing to `vdp_next_event` or the line branch.
- Thirteen cases in `tests/ports/test_gun.c`, 17 in all, registered on the ci and wasm32 lanes: `model`, `edge`, `luma`, `control`, `order`, `fault_neutral`, `lag`, `seen_expiry`, `peek`, `noshot`, `hammer`, `walk`, `th_toggle`.

## Task Commits

1. **Task 1: the lazy scanner, the TH level and pulse bit, the wrap and the five guarded sync sites** - `78918be` (feat)
2. **Task 2a: the GUN-01 cases (model, edge, luma, control, order, fault_neutral, lag, seen_expiry, peek)** - `97099ae` (test)
3. **Task 2b: the GUN-02 cases (noshot, hammer, walk, th_toggle)** - `cd3d589` (test)
4. **Fixup after the first hosted run: the pulse-bit clear used a cast MSVC reads as a truncating constant** - `464c2e1` (fix)
5. **Task 3: the gun tests' timeouts settled from the hosted dispatch** - `3228233` (test)

**Plan metadata:** the commit that carries this file.

## How the arm is ordered against the height update

`gun_wrap` is called after `m->vdp.height` is updated, inside the same `if (line + 1u == cogmaster__frame_lines(m))` branch, not before it as the plan wrote. Finishing the old picture needs no height: the beam is on the last line, which is past every picture, so every remaining row is due and emitted. Arming does need the new height (a row of the disc below the new picture is not scanned), and the loader's rule that a scanning port's `g_line` is below the height the VDP holds must hold the instant the event returns. Called after the update, the arm reads the height the picture will have and the rule holds with no second pass or exported helper. The finishing step does not read the render cursor, which the branch has just reset.

Two further implementation choices that the plan left open:

- A sync renders ahead (`render_until(line, t / 2)`, exclusive, never when the line is at or past the height, never under the blocked predicate) only when a due dot has not been emitted, not on every call. A gun that finds nothing to look at then leaves the render cursor where an unplugged run has it, so the render cursor equals the no-gun run at every frame boundary of the dark-aim run.
- A sync between the wrap and the end of the last line does nothing. The wrap has armed the next picture, whose rows would otherwise look due against the old picture's beam line.

## Evidence

All from the repository root, no `cd`. Hosted evidence is in the section below.

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci` | 0, no warning |
| `ctest --test-dir build/ci -R '^(io\.\|bus\.\|vdp\.\|audio\.\|core\.\|ports\.\|abi\.\|fuzz\.regress\.\|fixtures\.)'` (250 tests, after task 1) | 0 |
| `ctest --test-dir build/ci -R '^ports\.gun\.'` (17 tests) | 0 |
| `docker run --rm -v "$PWD":/w -w /w gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I src -I include -I build/ci/generated` on `src/gun.c src/io.c src/vdp.c src/vdp_render.c src/frame.c` | 0 each |
| the same container command with `-I tests` on `tests/ports/test_gun.c` | 0 |
| `clang-format --dry-run -Werror` on every changed C file | 0 |
| `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` build of `src/*.c` and `tests/ports/test_gun.c`, the 17 cases run | 0 each |
| `cmake --workflow --preset ci` | 0 (562 of 562) |
| `cmake --workflow --preset hygiene` | 0 (50 of 50) |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (723 check lines / 9221 src lines; no counted check file edited) |
| `git diff --quiet origin/main -- tests/io tests/vdp tests/bus tests/audio tests/testcard_pins.cmake tests/ports/test_ports.c` | 0 |

One-shot mutation runs (python edit confirmed by `git diff --stat -- src`, rebuild exit 0, then the named case; each reverted and the 17 cases re-run green). ctest exits 8 when a case fails:

| Mutation | Case | ctest exit |
| --- | --- | --- |
| the sensor's edge tick plus 16 | `ports.gun.model` | 8 |
| the H latch taken at the bus tick instead of the edge tick | `ports.gun.edge` | 8 |
| the wrap no longer drops the pulse bit | `ports.gun.seen_expiry` | 8 |
| rendering to the tick over two plus one | `ports.gun.order` | 8 |
| a merge that runs port 1 before port 2 | `ports.gun.walk` | 8 |
| the scan restarting its row at each call | `ports.gun.hammer` | 8 |
| rendering while the display is on in an unsupported mode | `ports.gun.fault_neutral` | 8 |

The first attempt at two of these did not build (an unused parameter and an unused variable under `-Werror`), so the old binary ran; both were redone with a build exit of 0 before the result was trusted.

Acceptance counts: `cogmaster__gun_sync` in `src/io.c` 2, `src/vdp.c` 1, `src/frame.c` 1; `cogmaster__gun_wrap` in `src/vdp.c` 1; `^@@.*vdp_next_event` hunks in the vdp diff 0; added `src/vdp.c` lines naming sync or wrap 2; `] = {` in `src/gun.c` 0 and const array declarations 0; `abi.globals` exit 0; planning IDs in added `src` and `tests/ports` lines 0; `core.state.` and `fuzz.regress.` registered 5 each; `ports.gun.` 17 in ctest, `ci.txt` and `wasm32.txt`; `floor; provisional` in `tests/ports/tests.cmake` 0.

## Hosted dispatch 1

Commands, from the repository root:

| Command | Exit |
| --- | --- |
| `git push origin phase/09-light-phaser` (head `cd3d589`) | 0 |
| `gh workflow run ci.yml --ref phase/09-light-phaser` | 0 |
| `gh workflow run ci.yml --ref main` after `git fetch origin main` | 0 |
| `gh run watch` on each run | 0 for the base and the third branch run |
| `gh run view 43003756603 --json conclusion --jq .conclusion` equals success | 0 |
| `gh run view 43003064146 --json conclusion --jq .conclusion` equals success | 0 |
| the base run's head SHA equals `git rev-parse origin/main` | 0 |
| `jq -e` over the branch run's wasm32, be32, asan, nofp, perf and hash-equality jobs: at least six, all success | 0 |
| `jq -e` over the branch run's `build` jobs: at least one, all success | 0 |
| `git diff --quiet origin/main -- tests/perf/allow.txt` | 0 |

Runs, in the order they happened:

1. Branch run at head `cd3d589`, https://github.com/szTheory/cogmaster/actions/runs/43003041792, concluded failure: `build (windows-x64)` and `build (windows-arm64)` stopped on MSVC C4310 (cast truncates constant value) at `(uint8_t)~2u` in `src/io.c` and `src/gun.c`. asan, be32, perf, nofp and the other four build legs were green; wasm32, retroarch and hash-equality were skipped behind the failed build. Its be32 and asan logs are the first readings of the timings below.
2. A dispatch of the fixed head https://github.com/szTheory/cogmaster/actions/runs/43003682113 was made before GitHub had picked up the push and ran at `cd3d589`; it was cancelled and is not evidence.
3. Branch run at head `464c2e1` (the fix), https://github.com/szTheory/cogmaster/actions/runs/43003756603, concluded success on every job: hygiene, asan, be32, wasm32, both nofp jobs, perf, all six build legs including the two MSVC legs, retroarch and hash-equality. The asan job carries `ports.gun.hostile_aim` and `ports.gun.hammer` (3.24 s and 0.19 s) and the other fifteen gun cases. This is the run the perf line and the timings are read from.
4. Base run on `main` at head `54631cb` (the v0.8.0 release, equal to `git merge-base origin/main HEAD` and to `origin/main` after the run), https://github.com/szTheory/cogmaster/actions/runs/43003064146, concluded success.

The control lines, verbatim:

```
perf control boot_to_play runs=12303147,12303147,12303147 spread=0
perf control boot_to_play runs=12303207,12303207,12303207 spread=0
```

The first is the base run on `main`, the second the branch run 43003756603 (run 1 printed the same three values). Both spreads are 0, so H is 12303207 and B is 12303147.

```
perf dispatch boot_to_play H=12303207 B=12303147 delta_bp=0
```

computed in shell as `(H - B) * 10000 / B` and well under the 100 basis point limit, as expected: the diff adds nothing to `vdp_next_event` or the line branch and each of the five sites is one predicted branch with no gun. The old value 12302854 of run 42963279510 was not reused.

The dispatched head `464c2e1` differs from the commit that carries the timeouts (`3228233`) in `tests/ports/tests.cmake` only (`git diff --stat` of the two, excluding `.planning`, lists that one file). The MSVC legs, wasm32 (64-bit struct growth), be32 and nofp all saw the hooks and the 24-byte block for the first time in run 3 and were green.

### Measured test durations

Slowest hosted time per case over every lane of runs 1 and 3 (the be32 and asan jobs of both, every other job of run 3), in ctest seconds. All came from the be32 lane except where named.

| Case | Slowest | Timeout set |
| --- | --- | --- |
| `latch_at` (wasm32), `aim` (wasm32), `edge`, `luma`, `control`, `order`, `fault_neutral`, `lag`, `seen_expiry`, `peek`, `pins` | 3.31 s (`pins`) | 20 s floor |
| `hostile_aim`, `model`, `hammer`, `walk`, `th_toggle` | 13.83 s (`hostile_aim`) | 60 s floor |
| `noshot` | 124.50 s | 250 s |

Reported per the plan's quarter-of-floor rule: `noshot` took 124.50 s on be32 (87.20 s in run 3, 34.41 s under asan, 20.34 s on wasm32, 6.64 s on linux-x64). It runs five instances of four profiles for 600 frames each by the plan's own definition, so the time is the emulation and not a fault; its timeout is 2 x 124.50 s rounded up, as the neighbouring state test did for its be32 reading, and the be32 lane multiplies it by 8.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] MSVC read the pulse-bit clear as a truncating constant**
- **Found during:** Task 3, the first hosted run
- **Issue:** `(uint8_t)~2u` in `src/io.c` and `src/gun.c` is C4310 under `/W4 /WX`; no local compiler reads it so.
- **Fix:** the block's sense byte holds two bits, so the clear is `g_sense &= 1u`.
- **Files modified:** src/io.c, src/gun.c
- **Verification:** run 43003756603, both MSVC legs green
- **Commit:** `464c2e1`

### Plan steps done differently

- **The wrap hook sits after the height update, not before it** (see the section above). The plan's acceptance counts are unaffected.
- **The sync renders only for a due dot that is not yet emitted**, a narrowing of "call render_until when the line is below the height and the renderer is not blocked". The bound, the never-at-the-end rule and the blocked predicate are as the plan says.
- **`ports.gun.noshot` compares the state record by name except the sixteen masked names and `body_crc32`.** The checksum is derived from every field including the masked ones, so it differs whenever they do; it is the one name besides the masked ones that is not compared, and the test says so.
- **`ports.gun.peek` covers `cogmaster_peek` and `cogmaster__ports_pins`**; `th_pins` is a static function of `src/io.c` with no caller outside it, so it is covered through the `$DD` read cases.
- **`ports.gun.lag` aims the gun over black dots** for the frame-lag comparison: over a lit square the H counter latches, `$7F` then returns the latched value, and `bus.data` differs by the program's own read, not by anything the sensor does.
- **`ports.gun.edge` runs four first-read distances** (right at the edge and 1, 16 and 66 ticks after it). A read at every tick cannot tell a latch at the read from a latch at the edge, since the first read after the edge is at the edge.
- **The `walk` equality leaves the render cursor out**, because a polled run's cursor is where its last need put it; it compares the blocks without the pulse bit, the latch, its hold and the picture.
- **The orchestrator's decision, applied as given:** a soft or power reset keeps the aim and only a load parks it; no `src` change was made, and the docs plan states it.
- The two guns' edges are compared in `walk` by a poll-schedule equality and the formula for the later release; the sequential-merge control is a one-shot mutation as the plan's acceptance list says.

**Total deviations:** 1 auto-fixed (blocking), 7 narrowings or clarifications inside the plan's files. **Impact:** none on behaviour or on any requirement.

## Known Stubs

None. Every gun field is set and cleared by the sensor, and a zero block still means parked.

## Threat Flags

None. The sensor reads only `m->field` at an index the loader bounds (`g_line` below the height, a chord inside 0..255), indexes no table, loops at most six steps per row and at most 126 dots per picture per gun, and the new inputs were already range-checked by plan 03.

## Self-Check: PASSED

- `src/gun.c`, `src/io.c`, `src/vdp.c`, `src/vdp_render.c`, `src/frame.c`, `src/internal.h`, `tests/ports/test_gun.c`, `tests/ports/tests.cmake` and both inventories exist with the described edits; commits `78918be`, `97099ae`, `cd3d589`, `464c2e1` and `3228233` are ancestors of HEAD.
- Every acceptance criterion of the three tasks was re-run and passes; the Task 3 verify command is run below and exits 0.
