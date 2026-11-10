---
phase: 04-sound
plan: "05"
subsystem: audio-core
tags: [audio, psg, sn76489, lazy-catch-up, closed-form-oracles, state, c17]

requires:
  - phase: 04-sound
    provides: cogmaster__synth_level, the ring, the mixer and the synth state (plan 04-04); the trace log and cadence (plan 04-02); the state header byte and audio guide (plan 04-03)
provides:
  - src/psg.c, the SN76489 model - lazy 48 tick steps, latch and data protocol, noise register, held-high periods 0 and 1, attenuation-coded levels
  - Z80 writes to ports $40-$7F reach the chip through the bus, stamped at the write tick
  - frame end runs the chip to the frame deadline inclusive; load and both resets return it to power-on constants with its step origin at tick 0 and hand its levels to the synth at tick 0
  - the PSG state in the save format with range checks and cross-checks
  - cogmaster-test-psg with eleven cases labelled psg, in both inventories, and the PSG section of docs/audio.md
affects: [04-06, 04-07, 04-08, 04-10, 04-12, 04-14]

actuals:
  tokens: 13700
  tasks: 3
  commits: 4

tech-stack:
  added: []
  patterns:
    - "A chip reports a level change by comparing its channels' levels before and after a step or a write, so it keeps no copy of what the synth holds"
    - "The frame end runs a chip to the closing frame's deadline with the frame counter stepped back, so its events are placed from the start the frame had while it ran"
    - "A state's lazy step origin is bounded to one frame plus one instruction behind the tick count, so a hostile state cannot make the next catch-up run unbounded steps"
    - "Closed-form oracles read chip registers and the drained trace, never a second chip model: spacing 48N, the volume law in integers, hand-derived shift register walks"

key-files:
  created:
    - src/psg.c
    - tests/audio/test_psg.c
  modified:
    - CMakeLists.txt
    - src/bus.c
    - src/internal.h
    - src/state.c
    - src/synth.c
    - tests/audio/test_synth.c
    - tests/audio/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/audio.md

key-decisions:
  - "The chip compares its own levels before and after each step or write and emits the differences; the synth's stored levels are not consulted, so tests that set synth levels directly are not overwritten except by the chip's own changes (those tests park the chip)"
  - "The noise channel uses counter[3] as the rate counter, noise_div as the divider flip-flop (shift on its rising edge: every 32, 64 or 128 steps) and out[3] as the shifted-out bit; the first step toggles the divider 0 to 1, so the first shift is on the first step"
  - "A saved PSG step origin must be a multiple of 48, not after the tick count, and at most one frame plus 69 plus 48 ticks behind it; the bound is the T-04-09 mitigation for a state that would otherwise make the next catch-up run far past one frame of steps"
  - "State is saved between frames: the synth saves 32 ring slots, so a mid-frame save restores the chip exactly but not the taps the frame had already placed; psg.state_continue compares records and PCM across frames and records only (not PCM) for the mid-frame case"
  - "A Game Gear console in Master System video mode routes every PSG channel to both outputs (the mask is not decoded there), so it is not left silent between this plan and the stereo plan"
  - "The volume law oracle is derived in integers: round(32767 x 10^(-i/10)) from the ratio 213,225,862 / 2^28 on a Q20 value, each unrounded value at least 0.04 from a half"

patterns-established:
  - "Tests that set synth levels directly park the chip at the frame's last step before running a frame (tests/audio/test_synth.c frame helper)"
  - "A trace-log test after a load or a reset expects the snapshot (one record per active source at tick 0), not an empty log"

requirements-completed: [SND-01]

coverage:
  - id: D1
    description: "A Z80 OUT to $40-$7F (through the bus the CPU drives) reaches the PSG, is caught up and stamped at the write tick and is heard: tone 0 at period 254 gives source 0 records exactly 12,192 ticks apart, a non-silent PCM peak, and a read of $7E leaves the chip untouched"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.through_bus (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A load and both reset kinds leave one record per active source at tick 0 (four on a Master System, five on a native Game Gear, four on a Game Gear in Master System mode), and the synth's stored levels equal the chip's"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.snapshot (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D3
    description: "SOFT reset contract: the step origin restarts at 0 (a stale origin freezes the chip, mutation-checked), the held tone falls to silence in the first 16 samples of the next frame, the cumulative sample count stays on the cadence formula, and re-writing the pre-reset volume is audible"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.reset_soft (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D4
    description: "PSG state round trip: a state saved between frames continues with identical records and PCM for six frames; a state saved mid-frame restores the chip exactly; every chip field one past its range, LFSR 0, a step origin off the 48 tick grid, after the tick count or a frame behind is refused with the instance unchanged"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.state_continue (tests/audio/test_psg.c)"
        status: pass
      - kind: unit
        ref: "ctest core.state.record_sweep and core.state.round_trip (tests/core/test_state.c) over the new psg fields"
        status: pass
    human_judgment: false
  - id: D5
    description: "All 1,024 tone periods on tone 0 toggle every 48N ticks for N from 2, hold the output high with no records for 0 and 1, and a spread of periods holds on tones 1 and 2"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.periods (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D6
    description: "All 16 attenuations on all 4 channels give the record output << 4 | attenuation, a gained level equal to the integer-derived law (32767 at 0, 6538 at 7, 0 at 15) through the gain, and a PCM step within 90 to 109 percent of it"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.volumes (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The 8 noise controls shift at the documented interval (32, 64, 128 steps, or tone 2's rising edge with period 0 as 1), walk the register as hand-derived for white and periodic feedback, and raise the noise output at the 16th shift; the 16 bit white register returns to $8000 after exactly 57,337 shifts and periodic after 16"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.noise_controls and psg.lfsr_period (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D8
    description: "Write protocol: a data byte after a volume latch rewrites the volume, the latch persists across data bytes, power-on latches the volume of channel 1, a noise control byte (latch or data) resets the register and a noise volume write does not"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.protocol (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D9
    description: "Tie rule and frame flush: a write stamped on step tick 48 is applied after that step (records and counter show the order) and one at 47 before it; after the second frame the chip has taken 7,467 steps with the last at the 358,416 deadline; one catch-up call and many give the same records; a frame run through the bus equals writes plus a catch-up by hand; an empty frame leaves no tone-channel record"
    requirement: SND-01
    verification:
      - kind: unit
        ref: "ctest psg.tie_rule and psg.frame_flush (tests/audio/test_psg.c)"
        status: pass
    human_judgment: false
  - id: D10
    description: "docs/audio.md states the tie rule, the frame-end flush, the period 0 and 1 rule, the noise rules and the reset behaviour as the model's choices, flagging the hardware-open ones as defaults, without planning identifiers"
    requirement: SND-01
    verification:
      - kind: other
        ref: "ctest docs.budgets and docs.api_names pass inside cmake --workflow --preset ci; the agent read the section against src/psg.c"
        status: pass
    human_judgment: false
  - id: D11
    description: "The full ci and hygiene workflows pass with the chip, its state, its tests and the inventories; existing fixture and playing hashes do not move"
    requirement: SND-01
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci then cmake --workflow --preset hygiene: both exit 0"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-09
status: complete
plan_head_before: c60f30345e665db0c5d0bd033c378bb9d3993d9f
plan_head_after: 17078d524b2960c2ee26d5ee513562fdf73fcbf4
commits: 4
---

# Phase 4 Plan 05: The Sega PSG, its reset contract and its state Summary

**The SN76489 model in src/psg.c: Z80 writes to $40-$7F stepped lazily on a 48 tick grid and stamped at the write tick into the band-limited synth, reset to power-on constants with its levels snapshotted through the synth at tick 0, saved with a bounded step origin, and proven by eleven closed-form cases (all 1,024 periods, all 16 volumes on 4 channels, the 8 noise controls, the 57,337 shift cycle).**

## Performance

- **Duration:** about 20 min
- **Started:** 2026-11-09T20:14:00Z (approximate; the previous plan's closing commit)
- **Completed:** 2026-11-09T20:35:00Z (approximate)
- **Tasks:** 3
- **Files modified:** 12 (2 created)

## Accomplishments

- `src/psg.c`: `cogmaster__psg_reset`, `_catch_up`, `_write`, `_level` and `_snapshot`. A step at tick 48k runs every tone counter (reload and toggle at 0, so the first step toggles every flip-flop 0 to 1), the noise rate counter and divider, and the 16 bit register (output is the bit shifted out, white feedback on bits 0 and 3). A channel whose period is 0 or 1 holds its output high. Level changes are found by comparing each channel's level before and after a step or write and are passed to `cogmaster__synth_level` at the step or write tick.
- `src/bus.c` sends every `$40-$7F` write to the chip at `m->ticks`; `src/synth.c` resets the chip with the audio reset (snapshot at tick 0 through the synth, so the stored levels are the chip's) and runs it to the closing frame's deadline before integrating.
- `psg_io` in `src/state.c`: every field range-checked, LFSR not zero, step origin a multiple of 48 and within one frame plus 117 ticks behind the tick count; a size tripwire on `struct cogmaster__psg`.
- `cogmaster-test-psg` with `psg.through_bus`, `snapshot`, `reset_soft`, `state_continue`, `periods`, `volumes`, `noise_controls`, `lfsr_period`, `protocol`, `tie_rule` and `frame_flush`, all portable and registered in both inventories; the slowest takes 0.08 s.
- The PSG section of `docs/audio.md` and the snapshot sentence in the trace log section.

## Task Commits

1. **Task 1 (tracer): A Z80 OUT to the PSG port is heard** - `9e7f49b` (feat)
2. **Task 2 (tdd): Reset snapshots, the SOFT-reset contract and the PSG state** - `bb9bcde` (test), `32fccea` (feat)
3. **Task 3 (tdd): Closed-form PSG oracles** - `17078d5` (test, with docs/audio.md and a comment fix in src/psg.c)

**Plan metadata:** recorded in the following docs commit.

## Files Created/Modified

- `src/psg.c` - the chip
- `src/internal.h`, `src/bus.c`, `src/synth.c`, `src/state.c`, `CMakeLists.txt` - the PSG struct, stats, bus write, reset and frame-end hooks, state visitor
- `tests/audio/test_psg.c`, `tests/audio/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the eleven cases
- `tests/audio/test_synth.c` - the synth tests park the chip, and the trace drain case expects the reset snapshot
- `docs/audio.md` - the PSG section

## Decisions Made

See `key-decisions`. Measured and derived numbers:

- **Cost:** `psg.periods` runs 2.6 million steps in 0.02 s; the whole `ctest -L psg` set is under 0.2 s.
- **Frame steps:** an NTSC frame is 3,733 steps with the second frame ending exactly on a step (deadline 358,416 is step 7,467), so the inclusive deadline rule is exercised.
- **Volume law oracle:** the closest unrounded value to a half is index 12 at 2,067.458; the ratio constant is `round(10^(-1/10) x 2^28) = 213,225,862`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The synth's own tests lost their levels to the live chip**
- **Found during:** Task 1 (first full run: `audio.no_clip` peaked 23,988 instead of 28,944)
- **Issue:** The tests set sources 0 to 3 directly and ran test-card frames; once the frame end runs the chip, its noise output change overwrote the test's source 3 level.
- **Fix:** the `frame()` helper in `tests/audio/test_synth.c` parks the chip at its last step of the frame, so the synth tests keep driving the synth interface alone.
- **Files modified:** tests/audio/test_synth.c
- **Committed in:** `9e7f49b`

**2. [Rule 1 - Bug] A Game Gear console in Master System mode would be silent**
- **Found during:** Task 1 (reading `synth_routed` against the snapshot rule)
- **Issue:** the snapshot emits the stereo mask only in native mode, and `synth_routed` routed by that mask on every Game Gear, so the stored mask 0 would silence both outputs in Master System mode once the chip was audible.
- **Fix:** `synth_routed` sends every channel to both outputs when the console is a Game Gear and the mode is not native; the stereo plan keeps the native routing.
- **Files modified:** src/synth.c
- **Committed in:** `9e7f49b`

**3. [Rule 2 - Missing critical functionality] A hostile state could make the next catch-up unbounded**
- **Found during:** Task 2 (the plan asked for "at not after the instance tick")
- **Issue:** the chip is lazy, so a loaded step origin far behind the tick count would run an enormous number of steps in the next write or frame end, against the bounded-call rule.
- **Fix:** `psg_io` also requires the origin to be a multiple of 48 and at most one frame plus 117 ticks behind the tick count; `psg.state_continue` refuses a frame-behind origin, an off-grid origin and one after the tick count.
- **Files modified:** src/state.c, tests/audio/test_psg.c
- **Committed in:** `32fccea`

**4. [Rule 1 - Plan error] "State saved mid-frame" cannot carry the synth's pending taps**
- **Found during:** Task 2 (the first `psg.state_continue` run: PCM differed from sample 32 of the frame after the load)
- **Issue:** the synth saves 32 ring slots at and after the head because state is saved between frames (plan 04-04); a mid-frame save loses the taps the frame had already placed.
- **Fix:** the test saves between frames with the chip part way through every count and compares records and PCM, and a mid-frame variant (chip origin lagging by thousands of ticks) compares the restored chip and the following records only. The documented API saves between frames, so no behaviour changed.
- **Files modified:** tests/audio/test_psg.c
- **Committed in:** `bb9bcde`

**5. [Rule 1 - Test bug] The trace drain case expected an empty log after a load or reset**
- **Found during:** Task 2 (`audio.drain` after the snapshot landed)
- **Issue:** the plan 04-02 case asserted zero records after a load and both resets; the snapshot rule now leaves four.
- **Fix:** `expect_snapshot_only` asserts exactly the four tick 0 records and nothing from before.
- **Files modified:** tests/audio/test_synth.c
- **Committed in:** `bb9bcde`

**6. [Rule 1 - Test bug] Two cases wrote at ticks behind the instance**
- **Found during:** Task 3 (`psg.reset_soft` took 3 s)
- **Issue:** a write stamped at tick 40,000 after a frame had already run sent the bus's beam sync 4 billion ticks round the 32 bit difference.
- **Fix:** the write ticks are relative to `m->ticks`; the case now runs in 0.005 s.
- **Files modified:** tests/audio/test_psg.c
- **Committed in:** `17078d5`

**7. [Rule 3 - Blocking] Stats and tripwire**
- **Found during:** Task 1 (build)
- **Issue:** the plan's `psg_steps` counter made the synth struct one 32 bit word odd, failing its size tripwire.
- **Fix:** a second counter, `psg_writes`, keeps it even and is asserted by `psg.through_bus`; the tripwire counts five stats.
- **Files modified:** src/internal.h, src/state.c, src/psg.c
- **Committed in:** `9e7f49b`

### Plan variations (not defects)

- `struct cogmaster__psg` carries `noise_div` as the divider flip-flop and uses `counter[3]` and `out[3]` for the noise rate counter and output bit, as the plan listed; no separate noise counter exists.
- `psg.reset_soft` holds tone 0 at period 1 (output held high) rather than period 254, so the pre-reset gained level is deterministic; a square wave's level at the reset instant depends on its phase. The toggle grid is tested at period 5 after the reset.
- Ledger entry 9 (`src/synth.c` frame end writes silence, recorded by plan 04-02) is marked fixed: the frame end now integrates real chip output, tested end to end.

---

**Total deviations:** 7 auto-fixed (4 Rule 1, 1 Rule 2, 1 Rule 3, 1 test-ordering Rule 1). **Impact on plan:** no scope change; the hostile-state bound and the Game Gear mode routing are small additions inside the planned files.

## TDD Gate Compliance

The plan is type `execute` and `workflow.tdd_mode` is off. Task 1 is a tracer; Tasks 2 and 3 are `tdd="true"`.

- **Tracer:** `9e7f49b` committed, then its verify command (`ctest ... -R '^(psg\.|audio\.|core\.|bus\.|io\.|libretro\.host\.)'`) exited 0 and the full ctest passed (357 of 357) before expansion. Tracer verified end to end, then expanded.
- **Task 2 RED:** the three cases were written and run against the Task 1 code before any Task 2 source existed. Target tests and their failing assertions, from the real run (output is a CTest executable's stderr, a format the red-evidence classifier does not read, so no machine record was produced): `psg.snapshot` "sms load: 0 records, wanted 4" and "synth.level[ch] == power_on_level[ch] failed: 16 != 31"; `psg.reset_soft` "synth.gained[ch][0] == 0u failed: 6720 != 0" and "CHECK(fall > 4000) failed"; `psg.state_continue` "ra == rb failed: 561 != 42" (the unsaved chip diverges). Semantic assessment: each failed on the planned assertion, not on a build, load or fixture fault. The test commit `bb9bcde` precedes the source commit `32fccea`.
- **Task 2 GREEN:** `32fccea`; the three cases pass and the existing audio and state tests pass.
- **Non-vacuity by mutation, each reverted and never committed:** the snapshot tracing without feeding the synth failed `psg.snapshot` and `psg.reset_soft`; a step origin kept across a reset failed both and froze the chip (no toggles); reloading a period plus one failed `psg.periods` and `psg.tie_rule`; the noise reload halved failed `psg.noise_controls`; white taps on bits 0 and 2 failed `psg.noise_controls` and `psg.lfsr_period`; period 1 no longer held high failed `psg.periods`; the table entry 6538 changed to 6568 failed `psg.volumes`; a noise reset on a volume write failed `psg.protocol`; a flush one step short of the deadline failed `psg.frame_flush`. Removing the catch-up before a write failed `psg.tie_rule` only.
- **Task 3:** the cases characterise the Task 1 code (as in plan 04-04), so there is no failing RED commit; the oracles found no defect in `src/psg.c`.

## Issues Encountered

None beyond the deviations.

## Verification

Run from the repository root; exit statuses only, no logs committed.

- Task 1 verify (`cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(psg\.|audio\.|core\.|bus\.|io\.|libretro\.host\.)'`): exit 0.
- `ctest --test-dir build/ci -N -L psg`: lists 11 tests (psg.through_bus first).
- `ctest --test-dir build/ci --no-tests=error -R '^(fixtures\.|compat\.fixtures|runner\.hashes_file)'`: exit 0.
- Task 2 verify (`ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(psg\.|audio\.|core\.state\.)'`): exit 0.
- Task 3 verify (`ctest --test-dir build/ci --no-tests=error --output-on-failure -L psg`): exit 0, 11 of 11.
- `cmake --workflow --preset ci`: exit 0 (363 of 363 tests, then 4 of 4).
- `cmake --workflow --preset hygiene`: exit 0 (38 of 38; clang-format 21, hygiene.no_float and the analyzer included). The first hygiene run failed `hygiene.no_float` on two digit-dot-digit comment citations in `src/psg.c`; the comments were reworded and the rerun passed.
- The be32 and wasm32 legs were not run locally (qemu-ppc and wasmtime are not installed); the tests use no floating point, no signed shifts and fixed-width integers, and the timeout scales leave 80 s and 50 s over a native cost under 0.1 s.

## Known Stubs

None. The earlier open ledger entry for the frame end writing silence is resolved: real chip output now reaches the PCM. The FM unit (sources 5 and 6) is not wired yet; that is plan scope for the FM plans, not a stub in this plan's files.

## Threat Flags

None. Port writes, the PSG fields of a loaded state and the catch-up after a reset are the three surfaces the plan's threat model names (T-04-05 and T-04-09); each has its mitigation and a test.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for the Game Gear stereo plan: it adds the `$06` port in `cogmaster__io_out` (catching the chip up, storing the mask, emitting source 4) and the native-mode decode fix; `synth_routed` already routes by the mask in native mode and by both outputs in Master System mode, and the snapshot already emits source 4 in native mode. Ready for the fixture and runner plans: real games now produce PCM and a trace, so `--hash-audio` runs should read `clipped` and `peak` as plan 04-04 noted. SND-01 stays pending in the traceability table until the sibling plans that declare it (stereo routing, the `psg_sweep` fixture and the hash plans) finish.

## Self-Check: PASSED

Created files found: src/psg.c, tests/audio/test_psg.c. Modified files found: docs/audio.md and the sources and tests listed above. Commits found on the branch: 9e7f49b, bb9bcde, 32fccea, 17078d5.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
