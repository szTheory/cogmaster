---
phase: 04-sound
plan: "12"
subsystem: sound-fm-console
tags: [ym2413, opll, ports-f0-f2, audio-control, mixer, state, reset, cost]

requires:
  - phase: 04-sound
    provides: the integer YM2413 port with its lockstep and chip units (plans 04-10 and 04-11), the PSG, the band-limited synthesiser, the trace log and the state visitor
provides:
  - $F0/$F1/$F2 on the Japanese console (and any console with the unit), decoded ahead of the I/O chip, with exact 216-tick catch-up and the audio control mixed at the synthesiser
  - cogmaster__fm_catch_up and cogmaster__fm_reset, sources 5 and 6 in the mixer and in every load and reset snapshot
  - the YM2413 block in the saved state at a fixed size exactly when the unit is attached
  - eleven fm.* and one audio.* test, the FM cases of audio.no_clip and core.state
affects: [04-13, 04-14, 04-15]

actuals:
  tokens: 18223
  tasks: 2
  commits: 5
plan_head_before: 89cc3840f9784c8175c48f4aa6fefdf3b4469d8b
plan_head_after: d3347d48363c6869bec07d40a4d55a5b90900cfc

tech-stack:
  added: []
  patterns:
    - "A chip's lazy catch-up is the same shape for both chips: every sample or step due at or before a tick runs first, in order, then a write applies; $F2 catches both chips up before it gates them"
    - "A level that enters the mixer is made total in one place: unsigned bias, saturation to the sounding range, then the gain, so every 16-bit value lands inside the bound a saved state checks"
    - "Hostile-state coverage by one representative per distinct field name and bound, so the chip's 600 repeated fields cost 54 refusals, not 666"

key-files:
  created: []
  modified:
    - src/io.c
    - src/synth.c
    - src/state.c
    - src/internal.h
    - src/opll.h
    - tests/audio/test_opll.c
    - tests/audio/test_synth.c
    - tests/audio/tests.cmake
    - tests/core/test_state.c
    - tests/io/input_cases.h
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/audio.md
    - .planning/WINDOWS.md

key-decisions:
  - "The source-5 record stays the original's 16-bit wrap of the exact sum, and the mixer is a pure function of those 16 bits: bias by 32,768 (a top-bit flip), drop what is below the lowest sounding sum, saturate at twice COGMASTER__OPLL_SUM_MAX, scale to the PSG full scale, then gain 1402 over 4096. Every one of the 65,536 values gives a gained level between 0 and 11,216, the bound the state checks, so no int16 assumption is left (ledger row 10)"
  - "The chip's address latch lives in struct cogmaster__opll (a new byte), saved and reset with it, so $F0 needs no second copy in the I/O block"
  - "The muted source's stored gained level is 0, so power-on and every reset need no special value: a SOFT reset gates both sources off with the control and the step to silence enters the ring at tick 0"
  - "OPLL_UPDATE_ALL is 15, the four flags together (it was 255, with only the low four bits ever read), so a slot's pending requests have a bound narrower than their type; the lockstep digest is unchanged"
  - "The next-sample tick is bounded on load by three rules (a multiple of 216, past the last frame's deadline, at most one sample past the tick count) so a hostile state cannot make the next catch-up run unbounded samples"

patterns-established:
  - "Cross-block state checks use the values the visitor decoded, not the instance's: opll_slot_io returns the instrument index it read and synth_io and io_io return the levels and control, because VALIDATE never writes the instance"

requirements-completed: [SND-02, SND-05]

coverage:
  - id: D1
    description: "A note written through $F0 and $F1 is heard on the Japanese console: the registers hold it, the log has a source-5 record at every change on the 216-tick grid, and the level at each of 1,659 samples equals the chip run on the cadence with each write applied after the samples due at its tick (a write just after a sample is the case that fails if the catch-up follows it)"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "tests/audio/test_opll.c#through_bus (ctest fm.through_bus)"
        status: pass
    human_judgment: false
  - id: D2
    description: "$F2 stores v and fm_mask, bits 7 to 2 ignored and a mask of 1 keeping bit 0, and gates the mixer: the drained PCM carries the PSG alone for 0, the FM alone for 1, neither for 2 and both for 3 (four instances per value), and a mute at a late tick keeps what both chips did before it"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "tests/audio/test_opll.c#f2_write_mask and #f2_catches_up (ctest fm.f2_write_mask, fm.f2_catches_up)"
        status: pass
    human_judgment: false
  - id: D3
    description: "$F2 reads return the stored value with bits 7, 6 and 5 following bits 11, 7 and 3 of the line index of the tick count, bits 4 to 2 zero, and no read of $F0 to $F2 counts as input"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#f2_read_bits (ctest fm.f2_read_bits)"
        status: pass
    human_judgment: false
  - id: D4
    description: "With $3E bit 2 set the unit still answers: $F2 stores, mutes and reads back with its counter bits, and instrument, frequency and key-on written through $F0 and $F1 reach the chip and sound; the pad ports read open bus meanwhile and as before afterwards"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "tests/audio/test_opll.c#chip_off (ctest fm.chip_off)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Without the unit the three ports change nothing and record no fault: the export console reads them as $DC and $DD without counting input, the Japanese console and the Game Gear read open bus; io.lag_decode and io.decode_mirrors keep passing"
    requirement: SND-02
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#no_unit_ports (ctest fm.no_unit_ports)"
        status: pass
      - kind: unit
        ref: "tests/io/input_cases.h#test_lag_decode (ctest io.lag_decode, io.decode_mirrors)"
        status: pass
    human_judgment: false
  - id: D6
    description: "A load and both resets leave $F2 at 0, every YM register 0, the PSG silent, the next sample at tick 216 and the tick-0 snapshot of sources 0 to 3, 5 and 6; after a SOFT reset the synth's FM and mute levels equal the chip's, the sample count stays on the cadence, the note written again is silent until $F2 is written and then audible; a reload leaves nothing of the unit or its pending audio"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "tests/audio/test_opll.c#reset_kinds and #reload_zeroes (ctest fm.reset_kinds, fm.reload_zeroes)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The chip's block is saved field by field at a fixed size exactly when the unit is attached; a state saved mid-note and loaded into a second instance gives identical samples, records and bytes over ten frames; hostile controls, sample ticks, sums, envelope levels, instrument indices, phase indices and noise values, a control with no unit and a chip block without the unit are refused with nothing changed"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "tests/audio/test_opll.c#state_round_trip (ctest fm.state_round_trip)"
        status: pass
      - kind: integration
        ref: "tests/core/test_state.c (ctest core.state.round_trip, core.state.record_sweep, core.state.rejects_header)"
        status: pass
    human_judgment: false
  - id: D8
    description: "Under a hammer of OUT writes 33 ticks apart (register writes, $F2 cycling 0 to 3, PSG tone and attenuation writes) one frame takes at most ceil(frame ticks / 216) FM samples, exactly 18 slot evaluations a sample, at most ceil(frame ticks / 48) PSG steps, and at most 16 x rings x (4 x steps + 5 x writes + FM samples) kernel applications"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "tests/audio/test_opll.c#cost (ctest fm.cost)"
        status: pass
    human_judgment: false
  - id: D9
    description: "The FM mix is total and bounded: all 65,536 sums give a gained level between 0 and 11,216 that never falls as the signed sum rises, the lowest and highest sounding sums give 0 and the maximum, an out-of-bound sum counts as its low 16 bits; four PSG channels at 0 dB with a full-scale FM step peak at 28,996, under 29,204, with no clamp"
    requirement: SND-05
    verification:
      - kind: unit
        ref: "tests/audio/test_synth.c#test_fm_level (ctest audio.fm_level)"
        status: pass
      - kind: unit
        ref: "tests/audio/test_synth.c#test_no_clip_fm (ctest audio.no_clip)"
        status: pass
    human_judgment: false
  - id: D10
    description: "The whole ci workflow and the hygiene workflow exit 0 with the new tests registered in both inventories"
    requirement: SND-02
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci: exit 0 (389 of 389 plus 4 of 4); cmake --workflow --preset hygiene: exit 0 (38 of 38)"
        status: pass
    human_judgment: false

duration: 29min
completed: 2026-11-09
status: complete
---

# Phase 04 Plan 12: The FM Unit on the Japanese Console Summary

**$F0/$F1 write the YM2413 on an exact 216-tick catch-up, $F2 mutes PSG or FM at the mixer, the 16-bit FM sum is mixed through a total saturating bias so no int16 assumption remains, and the chip's block, reset snapshot and cost bound are saved, tested and documented.**

## Performance

- **Duration:** 29 min by the system clock (the start time was not captured; taken from the end of the previous plan)
- **Completed:** 2026-11-09T22:02:00Z
- **Tasks:** 2
- **Files modified:** 14 (13 source, test and doc files, and the ledger)

## Accomplishments

- On a console with the unit, ports $F0 to $F2 answer ahead of the I/O chip, so they work with $3E bit 2 set. A write to $F1 completes every sample due at or before its tick, then applies; $F2 catches both chips up, stores `v & fm_mask` and records source 6. The chip runs on the 216-tick grid from tick 216 and to the frame deadline after the PSG. `fm.through_bus` compares the level at all 1,659 samples of two frames against the chip run on that cadence, including a register write landing 10 ticks after a sample.
- The mixer treats source 5 as a pure function of its 16 bits: top-bit flip, drop below the lowest sounding sum, saturate at twice 32,688, scale to 32,767, gain 1402 over 4096. `audio.fm_level` runs all 65,536 values; no value leaves the 11,216 the state bounds it by, the mapping never falls as the signed sum rises, and the held-output sums that leave int16 (38,817) wrap as the original does and land inside the range. This resolves ledger row 10.
- The FM profile gates both sources by the control (PSG for 0 and 3, FM for 1 and 3, the PSG at 491 beside the unit). Four instances per $F2 value show PSG only, FM only, neither and both in the drained PCM.
- Every load and reset snapshots sources 5 and 6 at tick 0 after the PSG; `fm.reset_kinds` shows the synth's stored levels equal the chip's, the mute applied after a SOFT reset, and the note audible only after $F2 is written again.
- The state gains the chip's block (every field but the evaluation counter, the user instrument, derived per-slot values, pending requests, the next sample tick) with bounds from the tables they index and cross-checks that use the decoded values.
- `fm.cost` shows 829 FM samples a frame, exactly 18 slot evaluations each, and the kernel-application bound with the $F2 re-feeds holding under the hammer.

## Task Commits

1. **Task 1: a note the Z80 writes is heard on the Japanese console and $F2 gates it (tracer)** - `33fe0de` (feat)
2. **Task 2, red: control, reset, state and cost contracts, the FM level and the FM headroom** - `14ca4b5` (test)
3. **Task 2, green: the YM2413 block joins the saved state** - `e21757d` (feat)
4. **Task 2: the unit's ports, the $F2 control, the mix and the reset documented** - `6738188` (docs)
5. **Reloading content leaves nothing of the unit behind** - `d3347d4` (test)

**Plan metadata:** the commit that adds this summary and the ledger entry, then the state and roadmap commit.

## Measured values and commands

Every command ran from the repository root.

| Check | Command | Result |
|---|---|---|
| Task 1 verify | `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fm\.\|audio\.\|psg\.\|io\.\|fault\.\|core\.\|runner\.hashes_file$)'` | exit 0 (79 portable tests, 4.9 s) |
| Task 2 verify | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fm\.\|audio\.no_clip\|core\.state\.\|io\.)'` | exit 0 (34 tests) |
| Workflows | `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` | both exit 0 (389 of 389 plus 4 of 4; 38 of 38) |
| Red evidence | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^(fm\.\|audio\.\|core\.state)'` at `14ca4b5` | exit 8: fm.state_round_trip (b->fm_next 179,280 != 537,840, chip fields differ, PCM and 306-record logs differ), core.state.round_trip (saved bytes differ after the note) and core.state.rejects_header (size > target_size) fail |
| Sanitizers | `cc -std=c17 -O1 -g -fsanitize=undefined,address -fno-sanitize-recover=undefined` over src/*.c with test_opll.c, test_synth.c and test_state.c | every new case and core.state.round_trip, record_sweep, rejects_header exit 0 |
| Full-scale headroom | `build/ci/tests/cogmaster-test-audio no_clip` | prints peak 28,996 against the 29,204 line, no clamp |
| Timings | `ctest -R '^(fm\.\|audio\.fm_level)'` | slowest new test fm.state_round_trip 0.09 s; the 10 s floor stays |

The be32 and wasm32 legs were not run here (`qemu-ppc` and the pinned wasmtime are not installed); the comment in `tests/audio/tests.cmake` marks the timeouts provisional.

Mutation checks (a scratch mutation of the source, a rebuild, the audio, fm, io, psg and state tests, then a restore): the unit's ports behind the I/O gate, FM always or PSG audible at 1 or 2, the write before the catch-up, no catch-up at $F2 for either chip, the reset keeping the control or not snapshotting, no FM catch-up at the frame end, a wrong counter bit, the mask ignored, ports decoded without the unit, the export mirror counting as input, a missing saturation, a state that drops a slot field, a reload that keeps the chip, the next-sample tick restarting at 0, and the PSG gains swapped each made at least one test fail. Two mutants first survived (the state field and the PSG catch-up at $F2); they led to the stronger state comparison and `fm.f2_catches_up`.

## Files Created/Modified

- `src/io.c` - the unit's ports ahead of the I/O chip, $F2 read bits, the no-unit mirrors no longer counting input
- `src/synth.c` - fm_catch_up, fm_reset, the source 5 and 6 snapshot, the total FM level, the gate by the control, source 5 and 6 in synth_level
- `src/state.c` - io_io audio_ctrl, the per-instance level bounds, the YM2413 block and its checks, the chip tripwire
- `src/internal.h`, `src/opll.h` - the chip and next-sample tick on the instance, audio_ctrl, the latch byte, OPLL_UPDATE_ALL as 15, COGMASTER__OPLL_SUM_ALL
- `tests/audio/test_opll.c` - the unit's cases (1,333 lines, its chip units unchanged)
- `tests/audio/test_synth.c` - audio.fm_level and the FM part of audio.no_clip; the parked-chip helper parks the FM sample too
- `tests/core/test_state.c`, `tests/io/input_cases.h` - sizes of the two Japanese states, a Japanese sweep and a note in the image round trip, the export mirror no longer input
- `tests/audio/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - ten fm rows and audio.fm_level
- `docs/audio.md` - ports, control, mix, reset, records
- `.planning/WINDOWS.md` - row 10 fixed

## Decisions Made

See key-decisions. In short: the record stays the original's 16-bit wrap, the mixer makes that total, the address latch is the chip's, the muted source holds 0 so no reset needs a special level, and the next-sample tick is checked on load.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] The chip had no address latch**
- **Found during:** Task 1
- **Issue:** the plan says $F0 latches the register and the state saves the latch, but the chip struct from plan 04-10 had no such field.
- **Fix:** a `latch` byte in `struct cogmaster__opll`, cleared by its reset and saved in its block.
- **Files modified:** src/opll.h, src/io.c, src/state.c
- **Committed in:** 33fe0de, e21757d

**2. [Rule 2 - Missing critical] A request mask with no bound**
- **Found during:** Task 2
- **Issue:** `OPLL_UPDATE_ALL` was 255 though only the four low flags are read, so the pending requests of a slot could not be given a bound narrower than their type.
- **Fix:** it is 15, the four flags together; the lockstep digest and every chip test are unchanged.
- **Files modified:** src/opll.h
- **Committed in:** 33fe0de

**3. [Rule 3 - Blocking] Existing tests assumed no FM block**
- **Found during:** Tasks 1 and 2
- **Issue:** a Japanese console now carries the unit by default, so its state is a different size from an export console's, and the export mirror of $F0 to $F2 no longer counts as input. `core.state.round_trip`, `core.state.rejects_header` and `io.lag_decode` failed.
- **Fix:** `tests/core/test_state.c` builds the profile-mismatch pair with the unit off so the sizes stay equal, and checks that the states of a console with and without the unit differ in size and refuse each other; `tests/io/input_cases.h` (the file `tests/io/test_io.c` includes) excludes $F0 to $F2 from the export console's decoded input ports.
- **Files modified:** tests/core/test_state.c, tests/io/input_cases.h
- **Committed in:** 33fe0de, 14ca4b5

**4. [Plan text] The mixer's normalisation is by the sounding bound, not the returned sum**
- **Found during:** Task 1, from the ledger finding of plan 04-11
- **Issue:** the plan text biases by 32,768 and normalises by `COGMASTER__OPLL_SUM_MAX`, which assumes the sum is inside int16. It is not always (held outputs reach 38,817).
- **Fix:** the record keeps the original's wrap (the lockstep digest and D-18's `(uint16_t)` are unaffected); the mixer is made total over the 16 bits with a saturating span of twice the bound, so out-of-bound sums land inside the range or at its ends; `audio.fm_level` proves it for all 65,536 values. Ledger row 10 is marked fixed.
- **Files modified:** src/synth.c, tests/audio/test_synth.c, .planning/WINDOWS.md
- **Committed in:** 33fe0de, 14ca4b5

**5. [Additions beyond the plan's case list]** `fm.f2_catches_up` (the survivor of a mutation of the $F2 catch-up) and `fm.reload_zeroes` (the plan's idempotency edge had no case) were added and registered in both inventories; `audio.fm_level` is the test the ledger finding asked for.

**TDD note.** The plan is not a TDD plan (`workflow.tdd_mode` is off). Task 1 is a tracer and was committed with its test after the implementation; `fm.through_bus` was then proved non-vacuous by mutation. Task 2's state work has a true red commit: `14ca4b5` fails on the planned assertions (the chip's fields, sample tick, PCM and records differ after a load) and `e21757d` makes it pass. The other Task 2 cases test code that Task 1 wrote and were green on the first run; each was shown to fail under a mutation.

---

**Total deviations:** 4 auto-fixed (2 Rule 2, 1 Rule 3, 1 plan text) and one set of additions
**Impact on plan:** no scope change; the ledger finding is resolved and the extra cases close gaps the plan's list left.

## Issues Encountered

`clang-format` 21.1.8 is at `/opt/homebrew/opt/llvm@21/bin/clang-format` (the previous summary said it was absent); `hygiene.format` failed on the first workflow run and the changed C files were formatted with it. The local `asan` preset still needs clang-21 and was replaced by `cc` with the same sanitizers over the same sources, as in the previous plan.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The Japanese profile plays FM through the bus with $F2 control, a bounded cost and a saved state, and the trace log carries sources 5 and 6 at tick 0 and at every change; plan 04-13 can drive the unit from a fixture and pin its hashes.
- Not run here: the be32 and wasm32 legs; the hosted lanes run every new test.
- No stubs, no skipped tests; ledger row 10 is fixed and the ledger has no open entry.

## Self-Check: PASSED

`src/io.c`, `src/synth.c`, `src/state.c`, `tests/audio/test_opll.c`, `tests/audio/test_synth.c` and `docs/audio.md` exist; commits 33fe0de, 14ca4b5, e21757d, 6738188 and d3347d4 are ancestors of HEAD; `ctest -R '^fm\.'` lists 18 passing tests; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
