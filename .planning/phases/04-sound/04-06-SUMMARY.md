---
phase: 04-sound
plan: "06"
subsystem: audio-core
tags: [audio, game-gear, stereo, port-06, io-decode, cost-bound, c17]

requires:
  - phase: 04-sound
    provides: the Sega PSG, its bus write path, the reset snapshot and the PSG state (plan 04-05); the stereo routing in the synth, the trace log and the cost counters (plans 04-02 and 04-04)
provides:
  - a write to port $06 in the Game Gear's native video mode catches the PSG up, keeps the mask in its one copy in io.gg[5] and records source 4 at the write tick
  - the Game Gear ports $00-$06 and the $3E and $3F decode follow the video mode, so a Game Gear console in Master System mode decodes $3E and $3F as a full-decode Master System, ignores $06 and powers on with $3E at $AB, and keeps its pad, full decode and Start as Pause
  - audio.cost, a hostile OUT hammer over a whole frame on NTSC, PAL and native Game Gear, asserting PSG steps, kernel additions, ring reach and trace log size against closed-form bounds
affects: [04-07, 04-08, 04-10, 04-12, 04-14]

actuals:
  tokens: 6300
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "A static has_gg_ports helper in src/io.c answers the video mode; is_gg stays the console, so full decode, the pad pins, $DD and chip_in keep following the console"
    - "A cost test hammers the bus in process through cogmaster__bus_out with m->ticks set per write, and asserts counters against formulas, never against a clock"

key-files:
  created: []
  modified:
    - src/io.c
    - src/state.c
    - tests/io/test_io.c
    - tests/io/tests.cmake
    - tests/audio/test_psg.c
    - tests/audio/test_synth.c
    - tests/audio/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/audio.md

key-decisions:
  - "Only the Game Gear port block $00-$06 and the $3E/$3F decode follow the video mode; is_gg (the console) still decides full decode, the pad pins, $DD and chip_in, so a Game Gear console in Master System mode keeps its pad and Start as Pause"
  - "A Game Gear console in Master System mode treats $00-$06 as a full-decode Master System treats any undecoded port: reads are open bus with an unmapped_read record, writes latch nothing and record an unmapped_write; no playing or fixture hash moves"
  - "A $06 write in native mode stamps at m->ticks after cogmaster__psg_catch_up, stores the byte once in io.gg[5] and calls cogmaster__synth_level for source 4, so the mask has a single copy and the log carries one source 4 record per write"
  - "io_io's mem_ctrl range check follows native mode: native Game Gear stays fixed at $A8, a console in Master System mode accepts any byte"
  - "audio.cost bounds kernel additions by 16 x outputs x (4 x steps + 5 x writes): a step changes at most four channel levels and a stereo write re-routes four sources plus its own record; the measured counts sit at about 16 percent (Master System NTSC), 16 percent (PAL) and 12 percent (Game Gear) of the bound, with ring reach 816, 981 and 816 against 992"

patterns-established:
  - "Tests that need a Game Gear console in Master System mode load with media.video_mode set explicitly; the io test helper load_video carries it and load keeps the machine's default"

requirements-completed: [SND-01, SND-05]

coverage:
  - id: D1
    description: "A write to $06 in native Game Gear mode routes each channel by its bit (bits 4 to 7 left, bits 0 to 3 right): $FF, $0F, $F0 and $00 sound both outputs, the right only, the left only and neither with the other output exactly zero, each of the eight bits alone sounds exactly its own output for exactly its own channel, and the write leaves the chip caught up to the write tick and one source 4 record at that tick carrying the mask"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.gg.routing (tests/audio/test_psg.c#test_gg_routing)"
        status: pass
    human_judgment: false
  - id: D2
    description: "In Master System video mode a Game Gear ignores $06: no source 4 record, the mask stays $FF, every sample has left equal to right, and the same writes in native mode split the two outputs"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.gg.port06_sms_mode (tests/audio/test_psg.c#test_gg_port06_sms_mode)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A Game Gear console in Master System mode powers on with $3E at $AB, latches $3E and $3F, answers $00-$06 as open bus, latches nothing on a $06 write or a mirror of $3E (unmapped write), keeps the pad at $DC and $C0, $DD and the chip's other mirrors, and returns $3E to $AB on reset"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest io.gg_sms_mode (tests/io/test_io.c#test_gg_sms_mode)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Native Game Gear decode, every other io case, the save-state round trip and every fixture are unchanged by the decode fix, and the 66 playing hash rows (14 of them gg-sms) are identical before and after"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest -R io. psg. core.state. fixtures. runner.hashes_file compat.fixtures: 67 passed"
        status: pass
      - kind: other
        ref: "grep '^playing' build/ci/hashes.txt before and after the change, diff exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "A hostile hammer of OUT writes 33 ticks apart over a whole frame on NTSC, PAL and native Game Gear keeps PSG steps at or under ceil(frame_ticks / 48), kernel additions at or under 16 x outputs x (4 x steps + 5 x writes), ring reach plus 32 under 1,024 and the frame's trace log inside 32,768 records with none dropped, and the counters show real work (more than 16,000 kernel additions)"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "ctest audio.cost (tests/audio/test_synth.c#test_cost)"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/audio.md states the Game Gear stereo routing, the Master System mode behaviour and the bounded per-frame cost without planning identifiers"
    requirement: SND-01
    verification:
      - kind: other
        ref: "ctest docs.budgets and docs.api_names pass inside cmake --workflow --preset ci; the agent read the section against src/io.c"
        status: pass
    human_judgment: false
  - id: D7
    description: "The full ci and hygiene workflows pass with the new tests, inventories and decode fix"
    requirement: SND-05
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci (exit 0, 367 tests) then cmake --workflow --preset hygiene (exit 0, 38 tests)"
        status: pass
    human_judgment: false

duration: 8min
completed: 2026-11-09
status: complete
plan_head_before: d594c2790af99ac0c9da1e6c253f24f3c00c6110
plan_head_after: fef39e2eb0d1c25e2f2d4c672b18df542c6734a4
commits: 2
---

# Phase 4 Plan 06: Game Gear stereo on $06, the native-mode port decode and the hostile-hammer cost bound Summary

**A write to port $06 in the Game Gear's native mode catches the PSG up and routes each channel left or right by its mask bit with one stored copy, the Game Gear port block and $3E/$3F decode now follow the video mode (so a Game Gear console in Master System mode ignores $06 and decodes $3E like a Master System), and a whole-frame OUT hammer proves PSG steps, kernel additions, ring reach and the trace log stay under closed-form bounds on NTSC, PAL and Game Gear.**

## Performance

- **Duration:** about 8 min
- **Started:** 2026-11-09T20:35:00Z (approximate; the previous plan's closing commit)
- **Completed:** 2026-11-09T20:43:00Z (approximate)
- **Tasks:** 2
- **Files modified:** 10 (none created)

## Accomplishments

- `src/io.c`: a static `has_gg_ports` helper (the video mode) decides the `$00-$06` branches of `cogmaster__io_in` and `cogmaster__io_out`, the Game Gear branch versus the `$3E`/`$3F` branch, and `io_reset`'s `$3E` default ($A8 native, $AB in Master System mode). `is_gg` (the console) still decides `full_decode`, `tr_pin`, `port_dc`, `th_pins`, `port_dd` and `chip_in`. A native `$06` write calls `cogmaster__psg_catch_up`, stores `io.gg[5]` and calls `cogmaster__synth_level` for source 4.
- `src/state.c`: `io_io`'s `mem_ctrl` range follows native mode.
- `io.gg_sms_mode`, `psg.gg.routing` and `psg.gg.port06_sms_mode`, each first seen failing on its planned assertions (mask not applied, `$3E` stuck at $A8, `$06` splitting outputs in Master System mode), then passing.
- `audio.cost` in `cogmaster-test-audio`: measured steps 3,733 of 3,734 (NTSC and Game Gear) and 4,460 of 4,461 (PAL); kernel additions 105,728, 126,336 and 158,464 against bounds of 673,312, 804,400 and 1,346,624; ring reach 816, 981 and 816.
- `docs/audio.md`: the Game Gear stereo and Master System mode bullet and the cost sentence in the PSG section.

## Task Commits

1. **Task 1 (tracer): Game Gear stereo on $06 and the native-mode port decode** - `bb31872` (feat; the three tests were written first and run failing on their planned assertions, then committed with the implementation in one commit because the plan is type execute)
2. **Task 2 (tdd): The hostile-hammer cost bound** - `fef39e2` (test; the bound is an existing property of the code, so the test passed on first run, and the printed counters confirmed it sits well inside its bounds)

**Plan metadata:** recorded in the following docs commit.

## Verification Record

Commands run from the repository root, with exit statuses.

- Tracer re-check: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(psg\.|io\.|core\.state\.|fixtures\.|runner\.hashes_file$|compat\.fixtures$)'`: exit 0, 67 of 67 passed.
- Playing rows: `grep '^playing' build/ci/hashes.txt > before.txt` before the change; after it `rm build/ci/hashes.txt`, `ctest -R '^runner\.hashes_file$'`, then `grep '^playing' build/ci/hashes.txt | diff before.txt -`: exit 0 (66 rows identical, 14 of them gg-sms).
- `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^audio\.cost$'`: exit 0.
- `cmake --workflow --preset ci`: exit 0 (367 tests). `cmake --workflow --preset hygiene`: exit 0 (38 tests). Hygiene also passed before the first commit.

## Files Created/Modified

- `src/io.c` - `has_gg_ports`, the mode-following decode, the `$06` write path
- `src/state.c` - `mem_ctrl` range check follows native mode
- `tests/io/test_io.c`, `tests/io/tests.cmake` - `io.gg_sms_mode` and the `load_video` helper
- `tests/audio/test_psg.c`, `tests/audio/test_synth.c`, `tests/audio/tests.cmake` - `psg.gg.routing`, `psg.gg.port06_sms_mode`, `audio.cost`
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the four new tests
- `docs/audio.md` - stereo routing, Master System mode and cost

## Decisions Made

See `key-decisions`. One consequence to know: a Game Gear console in Master System mode now treats `$00-$06` like any other undecoded port on a full-decode console, so a read records an unmapped read and a write records an unmapped write (first fault only, as everywhere). Before this plan those ports answered from the Game Gear latches in that mode. No fixture or playing row depends on it.

## Deviations from Plan

None - plan executed exactly as written.

The plan's checks named `tests/inventory/ci.txt` and `wasm32.txt` as sorted lists; the new names sort as `audio.cost` after `audio.cadence`, `io.gg_sms_mode` after `io.gg_ports` and `psg.gg.*` after `psg.frame_flush`, which configure accepted.

## TDD Gate Compliance

The plan is type `execute`, not `tdd`, so the plan-level gate sequence does not apply. For Task 1 the RED evidence was a local run of the three new cases before the source change: `io.gg_sms_mode` failed on `mem_ctrl == 0xAB` (was $A8) and the open-bus reads, `psg.gg.routing` on the caught-up chip tick, the missing source 4 record and the unsplit outputs, and `psg.gg.port06_sms_mode` on `io.gg[5] == 0xFF` and the missing split in native mode. All target assertions, none a build or load fault. Task 2 is a test of an existing bound, so it has no RED run.

## Issues Encountered

A helper named `make_traced` already existed in `tests/audio/test_synth.c` with another signature; the new constructor is `make_logged`.

## Known Stubs

None.

## Threat Flags

None. T-04-04 (a hostile ROM hammering the PSG and the stereo register) is mitigated and now asserted by `audio.cost`.

## Next Phase Readiness

- The stereo mask and its routing are complete; the FM unit plans can add sources 5 and 6 to the same bound (its FM-samples term is zero on these profiles).
- `audio.cost` timeout is the 10 s floor (0.00 s locally); confirm against the first hosted be32 and wasm32 timings before freezing, as the plan asks.

## Self-Check: PASSED

- `src/io.c`, `tests/io/test_io.c`, `tests/audio/test_psg.c`, `tests/audio/test_synth.c`, `docs/audio.md` present with the changes described.
- Commits `bb31872` and `fef39e2` are ancestors of HEAD.
- `audio.cost`, `io.gg_sms_mode`, `psg.gg.routing`, `psg.gg.port06_sms_mode` pass; `cmake --workflow --preset ci` and `--preset hygiene` exit 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
