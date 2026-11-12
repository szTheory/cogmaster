---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "06"
subsystem: ports
tags:
  - state-bounds
  - byte-mutation
  - fuzz
  - sports-pad
  - paddle
  - glasses
status: complete

requires:
  - 08-05
provides:
  - tests/ports/test_state.c, five ports.state cases that change every byte of every device field of a saved state against an independent bounds checker
  - tests/fuzz/fuzz_pad.c, the fifth fuzz target, with its own bounds checker and a saved-state leg
  - fuzz.regress.pad on every lane that registers the regress runs and fuzz.libfuzzer.pad on the fuzz lane
affects: [08-07, 08-17]

actuals:
  tokens: 16000
  tasks: 2
  commits: 2

plan_head_before: 0b694c5f68232ffd725fbd9a60b58ab4dab04663
plan_head_after: bd438ed5dd4ce3d5cb9529ff3ba543b8e4d92bb4
commits: 2

tech-stack:
  added: []
  patterns:
    - "The checker reads the instance or the bytes of a patched state through the field order and widths of the record and the format's own encoding (little-endian, a signed field with its top bit flipped), so a refusal and an acceptance are both compared with an answer that shares no code with src/state.c"
    - "A byte-mutation case loads into a second instance of the profile, takes a whole-struct snapshot only when the checker expects a refusal, and after an acceptance saves back, compares the bytes and runs a frame"
    - "The fuzz target names the saved fields to change in the ten bytes after the profile byte, so the operations that follow cannot use the stream up before the state leg"

key-files:
  created:
    - tests/ports/test_state.c
    - tests/fuzz/fuzz_pad.c
  modified:
    - tests/ports/tests.cmake
    - tests/fuzz/seeds.c
    - tests/fuzz/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/fuzz.txt
    - docs/perf-fuzz.md

key-decisions:
  - "The state test asserts that the loader and the checker agree in both directions on every patched state (refused if and only if the checker refuses), which also catches a rule or bound the core cannot reach, not only a loose bound"
  - "A single-byte edit of the first byte of a signed field changes the biased value, so the mutation values are applied to the raw byte and the checker decodes the patched bytes itself; the just-outside table sets values through the field's own width and bias"
  - "The pad fuzz target moves ticks only inside the frame window and calls the bus sync after each move, so every instance it reaches is one a run could have saved and the final save and load cannot fail for a harness reason"
  - "The libFuzzer lane and the other hosted lanes are the evidence for fuzz.libfuzzer.pad; locally the replay of the seeds and of 3000 structured inputs, also under the Apple sanitisers, is the stand-in"

requirements-completed:
  - PAD-07

coverage:
  - id: C1
    description: "Every byte of every device, axis, port block and eye field of a saved state, changed to 0x00, 0x01, 0x7F, 0x80, 0xFF and its complement on six instances (Sports Pads at step 2, a Paddle with a Sports Pad at step 1, a Game Gear, two eye pairs, a Japanese console with pads), is loaded after a reseal: 2299 are refused with the instance byte-identical, 653 are accepted, hold the checker's invariants, save back to the patched bytes and run a frame"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_state.c#test_mutation via ctest ports.state.mutation"
        status: pass
    human_judgment: false
  - id: C2
    description: "Each bound just outside (pending 32768, active 1, acc 1024, val -128, step 5, eyes 3, device 4, an edge past the tick count) and each cross-check violated once (a pad with a block or a pending axis, a Sports Pad with a position, a Paddle with a step, edge, acc, val or pending y, step 0 with a val, an eye of the frame with no eye, an eye on a Game Gear, a device the profile cannot take) is refused, and ten controls at the edge of a bound load; the checker and the loader agree on each"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_state.c#test_outside via ctest ports.state.outside"
        status: pass
    human_judgment: false
  - id: C3
    description: "Every field of the state one past each end of its bound is refused with the instance byte-identical on each of the six instances, all of the device fields and one of every other kind"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_state.c#test_sweep via ctest ports.state.sweep"
        status: pass
    human_judgment: false
  - id: C4
    description: "A state saved at each step of a Sports Pad read (step 1 with edge 0 after a soft reset and after a power reset, then steps 1 to 4 with a stored edge) loads into a second instance and saves byte-equal, both holding the checker's invariants"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_state.c#test_midsequence via ctest ports.state.midsequence"
        status: pass
    human_judgment: false
  - id: C5
    description: "A same-profile state patched to a Paddle on port 1 (pending, in force or both) is refused on an export console and on a Game Gear by the availability rule alone, and the same patch loads on a Japanese console"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_state.c#test_profile_device via ctest ports.state.profile_device"
        status: pass
    human_judgment: false
  - id: C6
    description: "The fifth fuzz target drives set_input with adversarial devices and axes (the answer must match the model exactly, a refusal changes nothing), $3F and $3E writes, $DC and $DD reads, $FFF8 to $FFFB writes, tick advances around the idle limit, both resets and frames on four profiles, checks the port bounds after every operation, then changes one to three saved fields of a copy and requires a clean refusal or a valid state that saves the same bytes, loads into another instance and runs a frame; its regress replay runs on every lane that registers the regress runs"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/fuzz/fuzz_pad.c via ctest fuzz.regress.pad; 44 seeds and 3000 structured inputs, both also under cc -fsanitize=address,undefined"
        status: pass
      - kind: integration
        ref: "fuzz.libfuzzer.pad on the hosted fuzz lane (macOS has no libFuzzer runtime)"
        status: pending
    human_judgment: false
  - id: C7
    description: "No existing state, io, audio or testcard test changed: core.state.* pass unchanged, tests/core/test_state.c, tests/io, tests/audio and tests/testcard_pins.cmake are byte-equal to origin/main, and the counted check code and the documents are within their budgets"
    requirement: PAD-07
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (494 tests); cmake --workflow --preset hygiene exit 0 (48 tests); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake tests/core/test_state.c exit 0"
        status: pass
    human_judgment: false

duration: 45min
completed: 2026-11-11
---

# Phase 8 Plan 6: Adversarial proof of the device state and the pad fuzz target Summary

**Every byte of every saved device field is mutated and loaded against a checker that shares no code with state.c (2299 refusals that leave the instance byte-identical, 653 acceptances that save back and run), and every device is in a fifth fuzz target whose set_input answers must match the model, whose bounds are checked after every operation and whose state leg changes saved fields and requires a clean refusal or a valid state.**

## Performance

- **Duration:** about 45 min
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 9 (2 new)

## Accomplishments

- `tests/ports/test_state.c` (one executable, five cases). The checker reads the values of a patched state straight from its bytes by the record's offsets and widths and from an instance, holds the bounds of the port model and its five cross-checks, and is compared with the loader in both directions: a state is accepted if and only if the checker accepts it, so a loose bound and a rule no run can reach are both caught. An acceptance must also decode to the values the checker read, save back to the patched bytes and run a frame; a refusal must leave the whole instance struct memcmp-equal.
- `mutation` changes every byte of 82 device field bytes six ways on six instances (2952 loads in about 5 s). `outside` is a table of 48 patched states with the intent of each. `sweep` is the out-of-range sweep of `state.record_sweep` over all device fields and one field of each other kind on the same six instances (about 165 values per instance). `midsequence` saves at each step of a read, including step 1 with edge 0 after both reset kinds. `profile_device` refuses a Paddle on an export console and a Game Gear by the availability rule alone and accepts it on a Japanese console.
- `tests/fuzz/fuzz_pad.c`, the fifth target in the shape of `fuzz_eeprom.c`, not folded into `fuzz_run.c`. A second checker (`check_pad`) is written afresh from the port model. A set_input must answer OK, ERR_ARGUMENT or ERR_PROFILE_REJECTED exactly as the model says and a refusal must leave the pending input unchanged, so an unexpected OK on an illegal device aborts.
- `tests/fuzz/seeds.c` writes `pad-ops.bin` (Japan: a Sports Pad on port 1 and a Paddle on port 2), `pad-ops-export.bin` and `pad-ops-gg.bin` (the same program on the other two profile kinds): a full read, a read abandoned one tick inside the idle limit that keeps its place, one abandoned at the limit that starts over, a glasses write, both resets, an unknown device and a hostile axis. The tick constants are named only in `fuzz_pad.c`; no file of the plan repeats 299, 598 or 1536.
- `tests/fuzz/tests.cmake` adds `pad` to both target loops, so `fuzz.regress.pad` registers wherever the regress runs do and `fuzz.libfuzzer.pad` on the fuzz lane; the inventories carry both lines and `docs/perf-fuzz.md` says five targets.

## Negative controls

Each edit was confirmed with `git diff --stat` (1 or 2 files) and built before the run, then reverted with `git checkout -- <file>`, rebuilt and re-run.

| Control | Command | Exit | Result |
|---|---|---|---|
| port_acc bound widened to the type's range in src/state.c | `ctest --test-dir build/ci -R '^ports\.state\.mutation$'` | 8 | mutation fails (outside fails too; the sweep, which only goes one past the type's end, still passes) |
| reverted | `ctest --test-dir build/ci -R '^ports\.state\.'` | 0 | 5 of 5 pass |
| set_input range widened to 255 and device_ok accepting any code (src/io.c, src/ports.c) | `ctest --test-dir build/ci -R '^fuzz\.regress\.pad$'` | 8 | fuzz.regress.pad aborts on the seeds |
| reverted | `ctest --test-dir build/ci -R '^fuzz\.regress\.pad$'` | 0 | passes |
| port_acc bound widened again, the state leg alone | `build/ci/tests/cogmaster-fuzz-replay-pad` over 3000 structured inputs | 134 | abort (the seeds alone and a first set of inputs did not reach it, see deviations) |
| reverted | the same command | 0 | replayed 3000 |

## Verification run

- Task 1 verify: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(ports\.state\.|core\.state\.)'` exit 0 (10 of 10); `ctest -N -R '^ports\.state\.'` lists the five names; both inventories carry five `portable,ports` lines; `Fault prevented: a loader that accepts a device state` once in tests/ports/tests.cmake; `git diff --quiet origin/main -- tests/core/test_state.c` exit 0; the five `core.state.*` tests exist and passed unchanged.
- Task 2 verify: `ctest --test-dir build/ci -R '^(fuzz\.regress\.|docs\.)'` exit 0 and `-N` prints `Total Tests: 17`; `fuzz.regress.pad` is on ci.txt and wasm32.txt once each and `fuzz.libfuzzer.pad` on fuzz.txt once; `Five fuzz targets` once and `Four fuzz targets` zero times; zero matches of 299, 598 or 1536 in fuzz_pad.c and seeds.c.
- Plan: `cmake --workflow --preset ci` exit 0 (494 tests plus 4 release tests); `cmake --workflow --preset hygiene` exit 0 (48 tests); `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8763 src lines); `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0.
- `cmake --workflow --preset fuzz` was not run: the libFuzzer runtime is not on this Mac. The hosted fuzz lane carries it at the next dispatch (the plan names none and nothing was pushed since `b05c7b4`).
- Sanitisers: `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` over src/*.c, fuzz_pad.c and replay.c, replaying the seeds and 3000 structured inputs, exit 0.
- Run times (this Mac, ci preset): mutation 4.7 s, sweep 1.5 s, outside 0.15 s, midsequence and profile_device under 0.1 s each; `fuzz.regress.pad` about 0.2 s. The registered timeouts (60 s, and the common 10 s) are provisional until a hosted run measures them, as the plan states.

## Task Commits

1. **Task 1: Byte-mutation, just-outside and mid-sequence state tests** - `526b2e9` (test)
2. **Task 2: The pad fuzz target, its seeds and registrations** - `bd438ed` (test)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The state leg of the fuzz target got no stream to mutate with**
- **Found during:** Task 2, the first state-leg negative control (a 2000-input replay did not abort with the acc bound widened)
- **Issue:** the operations consumed the whole input before the leg that changes saved fields read its parameters, so every input mutated the same byte of the same field.
- **Fix:** the ten bytes after the profile byte now name the fields to change and the operations start after them; the seeds write ten zero bytes there; a value byte that is not a multiple of four is applied directly and the others complement the byte, so a field leaves its range more often. The control then aborted (exit 134) on 3000 structured inputs generated with a Sports Pad in force.
- **Files modified:** tests/fuzz/fuzz_pad.c, tests/fuzz/seeds.c
- **Commit:** bd438ed (before it was made)

### Notes that are not deviations

- The plan puts the replayed seed through "the local replay runner"; the build has no separate runner, so `build/ci/tests/cogmaster-fuzz-replay-pad build/ci/tests/seeds` (44 files) exit 0 is that run, and the seed was also stepped through with a temporary print to confirm it reaches steps 1 to 4, the idle restart, the glasses writes and both resets (the temporary copy is not in the tree).
- The just-outside table loads each case into the instance of its profile with the checker's verdict recorded beside it, so a case that would be refused for a reason other than the one named is caught as a disagreement with the checker rather than passing silently.
- `ports.state.sweep` still passes with the acc bound widened to the type's range because its out-of-range value is one past the end of the type; the mutation and outside cases are the ones that catch that fault, as the plan expects of the mutation test.

**Total deviations:** 1 auto-fixed (a harness bug found by its own control). **Impact:** none on the core.

## Known Stubs

None.

## Threat Flags

None. The plan adds tests and a fuzz target; it adds no endpoint, file access or trust boundary.

## Issues Encountered

libFuzzer, the nofp lane and the Homebrew clang-21 asan preset cannot run on this Mac; the hosted fuzz, nofp, asan, wasm32 and be32 lanes are the evidence for them at the next dispatch.

## Next Phase Readiness

Ready for plan 08-07. The branch has the 08-05 and 08-06 commits locally; the next hosted dispatch should run on the branch head so the new tests run on the wasm32 and fuzz lanes.

## Self-Check: PASSED

- `tests/ports/test_state.c` and `tests/fuzz/fuzz_pad.c` exist; commits `526b2e9` and `bd438ed` are ancestors of HEAD.
