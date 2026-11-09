---
phase: 04-sound
plan: "04"
subsystem: audio-core
tags: [audio, band-limited-step, impulse-ring, dc-blocker, integer-dft, c17]

requires:
  - phase: 04-sound
    provides: the cadence, the PCM drain and the trace log (plan 04-02); the synth state visitor, cogmaster__sio_i64 and the fm_unit field (plan 04-03)
provides:
  - cogmaster__synth_level, which traces a raw level, gains it and feeds the change to the ring as a band-limited step
  - the 32 x 16 Q15 kernel (nesturbator table only), a 1,024-slot int64 ring per output channel, the level-gaining mixer, the leaky DC blocker and the saturating int16 conversion
  - synth state in the save format - ring slots at and after the head, integrator, DC estimate, head, levels and gained levels, all range-checked on load
  - cogmaster__synth_kernel and cogmaster__synth_gained_max, internal accessors for the tests and the state bounds
  - audio.step, audio.spectral, audio.no_clip, audio.amplitude_bound, audio.ring_overshoot, audio.stereo_mask and audio.state_continue, all portable and in both inventories
  - the synthesis section of docs/audio.md and the kernel row of PROVENANCE.md section 4
affects: [04-05, 04-06, 04-07, 04-12, 04-14]

actuals:
  tokens: 14180
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "A change is placed once per call: one unsigned division gives the sub-sample row, the 16 taps are interpolated into a local array and added to each routed channel"
    - "The mixer gains each level and feeds the difference to the ring, so rounding never drifts"
    - "Signed division by a constant is built on unsigned division of the magnitude, so no signed shift and no signed 64-bit division is used in src"
    - "A loaded state is made safe by bounds that no legitimate run reaches, and the integrator is clamped to its bound at every sample so the bounds hold for every state a load can reach"

key-files:
  created: []
  modified:
    - src/synth.c
    - src/internal.h
    - src/state.c
    - tests/audio/test_synth.c
    - tests/audio/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - PROVENANCE.md
    - docs/audio.md

key-decisions:
  - "D-04 shift deviation: the shipped 32-row table needs t = (frac << 21) / mod with row = t >> 16, since frac is below 2,128,137 which is below 2^22 (frac << 21 is below 2^43); the 64-row fallback would restore << 22. The fallback was not taken."
  - "Ring slots and the integrator are bounded at 2^56 on load (a legitimate slot is under 2^55 by the static assertion), the DC estimate at 2^41, and the integrator is clamped to its bound at every sample, so a hostile state cannot overflow int64 and every reachable state reloads"
  - "Gains: PSG 840, PSG beside the FM unit 491, FM 1402, all Q12, applied to the level and stored per source and channel"
  - "Sources 5 and 6 store and trace their levels but are not mixed yet; the FM wiring plan adds them"
  - "An event's sample index is capped at 975 so a corrupt caller cannot place taps beyond the saved window; a legitimate event lands at most one sample past the frame's count"

patterns-established:
  - "A synth test drives cogmaster__synth_level between frames with absolute ticks and the frame's own start (m->frame times the frame length) places each change"
  - "Tests that need a headroom number sweep the event tick across one sample so every kernel row is hit"

requirements-completed: [SND-05, SND-03]

coverage:
  - id: D1
    description: "A level change entering cogmaster__synth_level is heard as a band-limited, DC-blocked step at its place in the stream: zero before its sample, the rise through the kernel, about 96 percent of the gained level 16 samples later, then the 256-sample decay; the silent card stays exactly zero"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest audio.step (tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every kernel row sums to 32,768 and the largest taps are those the accumulation bound was derived from (tap sums 70,842, biggest tap 28,452)"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "ctest audio.step (kernel_table in tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Alias suppression: the 111.9 kHz square leaves nothing above the 16-bit rounding below 16 kHz and the 18.6 kHz square's worst non-harmonic bin is 85.0 dB under the reference, against the 80 dB rule, in an integer DFT whose verdict is the same on every platform; the second tone's fundamental is present, so the test is not vacuous"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "ctest audio.spectral (tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Headroom: four PSG channels at full level stepping together on both Game Gear channels peak at 28,944 at the worst of 75 kernel rows (7.6 percent overshoot) against the 29,204 line, 16,918 with the FM-profile gain, and nothing saturates, including a rise followed by a fall"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "ctest audio.no_clip (tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Accumulation bound: 128 changes per sample position and channel alternating in sign for a frame keep every ring slot under the bound the state and the static assertion use, the integrator and DC estimate far below theirs, and the ring is empty two frames later"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "ctest audio.amplitude_bound (tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D6
    description: "Ring overshoot: a change stamped 69 ticks past the deadline lands at sample index N or later (801 NTSC, 965 PAL), max_index stays below 992, is silent in its own frame and is heard in the next"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "ctest audio.ring_overshoot (tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D7
    description: "Game Gear stereo: masks F0, 0F, 00 and FF route channel 0 left only, right only, neither and both; a mid-tone mask change is a band-limited step on the left channel only and leaves the right channel bit for bit as a run without the change"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest audio.stereo_mask (tests/audio/test_synth.c)"
        status: pass
    human_judgment: false
  - id: D8
    description: "State continuity: a state saved mid-tone and loaded into a second instance gives byte-identical PCM for 10 frames, and a third instance that was told nothing stays silent beside them, so two instances share no synth state"
    requirement: SND-03
    verification:
      - kind: unit
        ref: "ctest audio.state_continue (tests/audio/test_synth.c)"
        status: pass
      - kind: unit
        ref: "ctest core.state.record_sweep and core.state.round_trip (tests/core/test_state.c) over the new synth fields"
        status: pass
    human_judgment: false
  - id: D9
    description: "docs/audio.md states the kernel origin, the volume law, the gains and the one decibel headroom figure, the DC corner, the stereo mask and what moves PCM versus transitions; PROVENANCE.md section 4 has the kernel row"
    requirement: SND-03
    verification:
      - kind: other
        ref: "ctest docs.budgets and docs.api_names pass; grep -c nesturbator PROVENANCE.md rose from 5 to 6"
        status: pass
    human_judgment: true
    rationale: "That the prose is accurate against the code is a judgment the agent made by reading the section beside src/synth.c; no test asserts the wording."
  - id: D10
    description: "The full ci and hygiene workflows pass with the synthesis, the state fields, the tests and the inventories"
    requirement: SND-05
    verification:
      - kind: other
        ref: "cmake --workflow --preset ci then cmake --workflow --preset hygiene: both exit 0"
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-09
status: complete
plan_head_before: d0c21054cb54fbc89358711bfa3b23711fa792d9
plan_head_after: cf28a218c18f1ea462ac4736398f922e8fa054ba
commits: 2
---

# Phase 4 Plan 04: Band-limited synthesis, mixer, DC blocker and proofs Summary

**Band-limited step synthesis behind cogmaster__synth_level: the nesturbator 16 x 32 Q15 kernel, a 1,024-slot int64 impulse ring per channel, a level-gaining mixer (840, or 491 and 1402 beside the FM unit), a leaky DC blocker and a saturating int16 output, saved in the state and proven by seven portable integer tests (85.0 dB alias suppression, 28,944 peak against 29,204).**

## Performance

- **Duration:** 15 min
- **Started:** 2026-11-09T19:57:00Z (approximate; the previous plan's closing commit)
- **Completed:** 2026-11-09T20:12:00Z (approximate)
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- `cogmaster__synth_level(m, tick, source, level)` traces the raw level, stores it, gains each PSG source's level (`(amplitude x g + 2048) / 4096`, non-negative arithmetic), routes it (one output on the Master System; the `$06` mask byte on the Game Gear) and feeds only the difference from the last gained level to the ring. A source-4 change re-evaluates sources 0 to 3 on both channels with one placement.
- One unsigned division per placed change gives the kernel row; each tap is `a x (65536 - f) + b x f`, row 31 interpolating toward row 0 shifted one tap right. `cogmaster__synth_frame_end` integrates exactly N slots, runs the DC blocker (`dc += (x - dc) / 256`, `out = x - dc`) and saturates to int16, counting clamps.
- The state carries the ring slots at and after the head (32 per channel), integrator, DC estimate, head, levels and gained levels with bounds derived from the table and the gains; the synth size tripwire is updated.
- The kernel row is in `PROVENANCE.md` section 4 and the commit body carries the same-author seed line; no fallback kernel tool was needed.
- Seven portable integer tests: step, spectral (an integer DFT of M = 8,192, Q30 trig by half-angle square roots, Q30 Blackman-Harris window, no floating point), no_clip, amplitude_bound, ring_overshoot, stereo_mask, state_continue.

## Task Commits

1. **Task 1 (tracer): A level change becomes a band-limited step in the drained PCM** - `f93362a` (feat)
2. **Task 2 (tdd): Spectral, headroom, accumulation, ring and continuity proofs** - `cf28a21` (test, with the docs/audio.md section)

**Plan metadata:** recorded in the following docs commit.

## Files Created/Modified

- `src/synth.c` - kernel, volume law, gains, ring, placement, mixer, DC blocker, saturating conversion, static assertions for the accumulation bound
- `src/internal.h` - ring and bound constants, the stats struct, the new synth fields, the new declarations
- `src/state.c` - `synth_io` extended through `cogmaster__sio_i64`, the size tripwire
- `tests/audio/test_synth.c`, `tests/audio/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - the seven cases
- `PROVENANCE.md` - the kernel row; `docs/audio.md` - the synthesis section and the SOFT reset sentence

## Decisions Made

See `key-decisions`. The measured numbers the plan asked for:

- **Spectral margin:** half period 48 ticks (111.9 kHz): every sample after the settle is zero, so no bin has energy above the 16-bit rounding (reported as the 120 dB cap). Half period 288 ticks (18.6 kHz): worst non-harmonic bin 1,947, 85.0 dB under the reference (the rule needs 80). The sibling table passed, so the fallback was not taken: no `tools/kernelgen`, no command to record.
- **Overshoot:** the largest partial-sum overshoot over interpolated rows is 8.28 percent (throwaway script); four channels stepping together peaked at 28,944 of 26,880 across 75 ticks that sweep every row (7.6 percent), and 16,918 with the FM-profile gain (four channels at 3,928 each).
- **Timings:** `audio.spectral` takes 0.05 s native; the slowest audio test is `audio.cadence` at 0.32 s, so the 10 s floor is kept. The be32 and wasm32 legs were not run (qemu-ppc and wasmtime are not installed here); the 8 and 5 times scales leave 80 s and 50 s, over a thousand times the native cost. The first hosted run is the measurement and the timeout comment says so.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Plan error] The step test's "within 1 percent 16 samples after s" cannot hold**
- **Found during:** Task 1 (first probe of the step response)
- **Issue:** the plan's claim that the DC blocker decays under 1 percent in 16 samples is wrong: at a 256-sample time constant the output is still about 96 percent of the level 16 samples on (6,472 of 6,720).
- **Fix:** `audio.step` asserts the real shape: zero up to the sample, the peak under 9 percent over the level, 93 to 98 percent at s + 16, and the level 256 samples later at (255/256) to the 256th of that, within 1 percent of the level.
- **Files modified:** tests/audio/test_synth.c
- **Committed in:** `f93362a`

**2. [Rule 2 - Missing critical functionality] A hostile state must not overflow int64, and every reachable state must reload**
- **Found during:** Task 1 (state bounds)
- **Issue:** bounds alone do not stop a loaded slot or integrator from overflowing as a frame integrates them, and a state saved after a hostile load could exceed the bound it was loaded under.
- **Fix:** the integrator is clamped to 2^56 at every sample; static assertions show a legitimate slot is under 2^55 and that a loaded integrator plus 32 loaded slots stays under 2^62; the DC estimate is a convex combination bounded at 2^41; an event's sample index is capped at 975.
- **Files modified:** src/synth.c, src/internal.h, src/state.c
- **Committed in:** `f93362a`

**3. [Rule 3 - Blocking] Two small internal accessors the plan did not list**
- **Found during:** Task 1 and Task 2
- **Issue:** the kernel table is static const (abi.globals), so a test could not check its row sums or tap maxima, and state.c needed the per-source gained bound.
- **Fix:** `cogmaster__synth_kernel(row, tap)` and `cogmaster__synth_gained_max(m, source)`, both with the internal prefix.
- **Files modified:** src/synth.c, src/internal.h
- **Committed in:** `f93362a`

**4. [Rule 1 - Test bug] The first no_clip run added a fall that the plan's bound does not cover**
- **Found during:** Task 2
- **Issue:** a rise followed by a fall 900 ticks later reached 29,403 on the worst row, over 29,204: the one decibel figure is the kernel's overshoot for a simultaneous step, and a pulse can ring higher.
- **Fix:** the bound is asserted for the simultaneous step (28,944); the rise-and-fall run asserts no saturation (peak under 32,767, floor above -32,768, clamps 0). The docs say so.
- **Files modified:** tests/audio/test_synth.c, docs/audio.md
- **Committed in:** `cf28a21`

**5. [Rule 1 - Test bug] The inventory files are sorted**
- **Found during:** Task 2 (first build)
- **Issue:** the six new names were appended after `audio.step`; the inventory check wants alphabetical order.
- **Fix:** sorted in both inventories.
- **Files modified:** tests/inventory/ci.txt, tests/inventory/wasm32.txt
- **Committed in:** `cf28a21`

---

**Total deviations:** 5 auto-fixed (2 plan or test-bug corrections to the step and clip checks, 1 missing critical, 1 blocking, 1 inventory ordering). The planned D-04 shift deviation (<< 21 for 32 rows) is recorded under Decisions.
**Impact on plan:** none on scope.

## TDD Gate Compliance

The plan is type `execute` and `workflow.tdd_mode` is off. Task 1 is a tracer; Task 2 is `tdd="true"` but its cases characterise the Task 1 code (the same shape as plan 04-02's Task 2), so there is no failing RED commit.

- **Tracer:** `f93362a` committed, then its verify command (`ctest ... -R '^(audio\.|core\.state\.|libretro\.host\.|abi\.globals|abi\.undefined)'`) exited 0 and the full ctest passed before expansion.
- **Non-vacuity by mutation, each reverted and never committed:** removing the sub-sample row (`t = 0`) failed `audio.spectral` on both tones (peaks 4.7 and 14.4 dB under the reference) and `audio.step`; dropping the DC estimate from the state visitor made `audio.state_continue` fail on 6 of 10 frames.
- A `test(04-04)` commit follows the `feat(04-04)` commit; there is no refactor commit.

## Issues Encountered

None beyond the deviations. The GG no_clip pulse case is a design observation for later plans: the peak bound is a step figure, so `--hash-audio` runs of real chip output should be read for `clipped` and `peak` rather than assumed under 29,204.

## Verification

Run from the repository root; exit statuses only, no logs committed.

- Task 1 verify (`cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(audio\.|core\.state\.|libretro\.host\.|abi\.globals|abi\.undefined)'`): exit 0, 19 of 19.
- Task 2 verify (`ctest --test-dir build/ci --no-tests=error --output-on-failure -L audio`): exit 0, 9 of 9.
- `cmake --workflow --preset ci`: exit 0 (352 of 352 tests, then 4 of 4).
- `cmake --workflow --preset hygiene`: exit 0 (38 of 38; clang-format 21, hygiene.no_float and the analyzer included).
- `grep -Ec '\b(float|double)\b' tests/audio/test_synth.c`: prints 0.
- `grep -c nesturbator PROVENANCE.md`: 6 (was 5).

## Known Stubs

- No sound chip calls `cogmaster__synth_level` yet, so real games are still silent. The open ledger entry for `src/synth.c` frame end (recorded by plan 04-02) stays open until the PSG plan wires the chip; the synthesis itself is real and tested through the interface.

## Threat Flags

None. The state's new synth fields (ring, integrator, DC estimate, head, levels) are range-checked on load, a refused state changes nothing, and `core.state.record_sweep` refuses a value one past each bound.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for the PSG plan: it calls `cogmaster__synth_level(m, tick, source, level)` with sources 0 to 4 and the tests here already cover the placement, gaining, routing and state of those calls. The FM plan adds sources 5 and 6 to `synth_psg`'s sibling (levels are stored and bounded already; gained bounds for source 5 are in `cogmaster__synth_gained_max`). SND-05 stays open in the traceability table until the cost counters of the PSG and FM plans and the hash lines exist; SND-03 until the PCM hash lines do.

## Self-Check: PASSED

Modified files found: src/synth.c, src/internal.h, src/state.c, tests/audio/test_synth.c, tests/audio/tests.cmake, docs/audio.md, PROVENANCE.md. Commits found on the branch: f93362a, cf28a21.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
