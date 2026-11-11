---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "04"
subsystem: ports
tags:
  - glasses
  - bus-hot-path
  - state
  - perf-gate
  - hosted-dispatch
status: complete

requires:
  - 08-03
provides:
  - the 3-D glasses eye recorded from writes to $FFF8-$FFFB on every Master System profile (never on a Game Gear) and reported as run_result.glasses by the window cut rule
  - io_glasses and io_glasses_frame saved with bounds 0..2 and two cross-checks, the io struct at 11 bytes
  - six ports.glasses cases on the ci and wasm32 inventories
  - hosted dispatch 1 green on every lane with delta_bp 0
affects: [08-05, 08-12, 08-15, 08-17]

actuals:
  tokens: 6800
  tasks: 3
  commits: 2

plan_head_before: 8a52acf38201030b1d132d3978a0880c709adf59
plan_head_after: b05c7b4ffd8bab67342a42367c9f466912404fef
commits: 2

tech-stack:
  added: []
  patterns:
    - "A rare-path helper takes the whole $FFF8-$FFFF store, so the work RAM write branch keeps one address compare and the mapper registers, the eye and the RAM-off fault each stay one arm"
    - "A frame window is anchored by the deadline, so a write's window tick is one unsigned subtract of the previous deadline and the cut is a per-TV constant"

key-files:
  created:
    - tests/ports/test_glasses.c
  modified:
    - src/bus.c
    - src/io.c
    - src/frame.c
    - src/internal.h
    - src/state.c
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The cut is a function-like macro over the frame length in lines, (lines - 96) * 684 - 607, anchored by the window start at line 192 tick 607; the test writes the same boundary again from line and tick numbers and also asserts the two agree"
  - "The helper also owns the $FFFC-$FFFF arm (cart on goes to the mapper, otherwise a RAM-off fault) so bus.c has one compare, and a test models the old branch to prove every fault is unchanged"
  - "A one-instruction program writes the eye at a chosen tick by setting the tick before the frame (the store lands 18 ticks after the instruction starts); a write at window tick 0 goes through the bus before the frame because no instruction can land there"
  - "A Game Gear is excluded by machine, not by video mode, so a Game Gear running in Master System mode still records no eye"

requirements-completed: []

coverage:
  - id: C1
    description: "A store to $FFF8-$FFFB records the eye (bit 0 clear right, set left) on SMS1 and SMS2 in any region whatever $3E says, never on a Game Gear, and a write with work RAM off faults exactly as the old branch did"
    requirement: PAD-04
    verification:
      - kind: unit
        ref: "tests/ports/test_glasses.c#gate and #profiles via ctest ports.glasses.gate and ports.glasses.profiles; the gate case models the old branch for every address, $3E state and machine"
        status: pass
    human_judgment: false
  - id: C2
    description: "The frame reports the eye in force at its start unless a write before the cut overrode it: writes at active lines 20 and 150, window ticks 0, CUT-1, CUT and line 192 tick 606 on NTSC, PAL and the 224-line mode, with the next frame reporting the eye in force"
    requirement: PAD-04
    verification:
      - kind: unit
        ref: "tests/ports/test_glasses.c#rule via ctest ports.glasses.rule against a closed form written from line and tick numbers"
        status: pass
    human_judgment: false
  - id: C3
    description: "An end-of-window snapshot disagrees with the core on exactly the late writes, so a rule that is secretly a snapshot fails"
    requirement: PAD-04
    verification:
      - kind: unit
        ref: "tests/ports/test_glasses.c#control via ctest ports.glasses.control; replacing the rule by a snapshot made it fail with ctest exit 8"
        status: pass
    human_judgment: false
  - id: C4
    description: "The eye write is never a lag poll (a frame holding it is a lag frame, a read of $DC is the control) and the frame hash is the same with the eye store and with the same store aimed at work RAM"
    requirement: PAD-04
    verification:
      - kind: unit
        ref: "tests/ports/test_glasses.c#nolag via ctest ports.glasses.nolag on all six profiles"
        status: pass
    human_judgment: false
  - id: C5
    description: "io_glasses and io_glasses_frame round-trip save, load, save byte-equal; glasses 3, glasses_frame 3, a frame eye with no held eye and any eye on a Game Gear are refused with the instance unchanged"
    requirement: PAD-07
    verification:
      - kind: unit
        ref: "tests/ports/test_glasses.c#state via ctest ports.glasses.state on SMS2 and Game Gear"
        status: pass
    human_judgment: false
  - id: C6
    description: "The bus.c hot-path change, the struct growth and the six new tests are green on the hosted asan, nofp, wasm32, be32 and Windows lanes, and boot_to_play is within the perf limit"
    requirement: PAD-04
    verification:
      - kind: integration
        ref: "https://github.com/szTheory/cogmaster/actions/runs/42963275959 (success); main control run https://github.com/szTheory/cogmaster/actions/runs/42963279510; H=12302997 B=12302854 delta_bp=0"
        status: pass
    human_judgment: false

duration: 55min
completed: 2026-11-11
---

# Phase 8 Plan 4: The 3-D glasses eye and hosted dispatch 1 Summary

**A store to $FFF8-$FFFB now records the 3-D glasses eye on every Master System profile and `run_result.glasses` reports it by a window-relative cut at active line 96; the eye pair is saved with bounds, six `ports.glasses.*` cases prove the rule against a closed form and an end-of-window snapshot, and the first hosted dispatch of the phase is green on every lane with `delta_bp=0`.**

## Performance

- **Duration:** about 55 min, including two hosted runs of about 5 min
- **Completed:** 2026-11-11
- **Tasks:** 3
- **Files:** 9 (1 new)

## Accomplishments

- `src/bus.c`: the work RAM write branch keeps its RAM write and one address compare; `addr >= 0xFFF8u` replaces `addr >= 0xFFFCu && cart_on` and calls `cogmaster__io_glasses_write`. The helper in `src/io.c` keeps the $FFFC-$FFFF behaviour (cart on goes to the mapper, otherwise the RAM-off fault) and for $FFF8-$FFFB records the eye unless the machine is a Game Gear, then faults with RAM off as before. The write never sets `input_read`.
- The cut: `COGMASTER__GLASSES_CUT_TICKS(frame_lines)` is `(lines - 96) * 684 - 607`, the window tick at which the beam reaches active line 96 of the picture the window ends in. A write with `window < CUT` also sets `glasses_frame`; later ones are reported next frame. `src/frame.c` copies `glasses` into `glasses_frame` at frame start and reports it in `run_result.glasses`.
- `src/state.c`: the io size assert is 11, `io_glasses` and `io_glasses_frame` are u8 0..2 using the value `cogmaster__sio_u8` returns, with the two cross-checks (`glasses_frame != 0` implies `glasses != 0`; both 0 on a Game Gear). `cogmaster__io_reset` zeroes the pair, and boot hand-off calls it on load and both resets.
- `tests/ports/test_glasses.c`: six cases (`rule`, `control`, `gate`, `profiles`, `nolag`, `state`), registered on the ci and wasm32 inventories with the same names on every lane. The Z80 stores at an exact tick by setting `m->ticks` before the frame (a store lands 18 ticks after its instruction starts, from the UM0080 cycle layout).
- Hosted dispatch 1, below.

## Negative controls

Each was run once locally with the edit confirmed by `git diff --stat src/io.c` (1 file changed) before the run, then reverted.

| Control | Command | Exit | Result |
|---|---|---|---|
| `window <= CUT` in the helper | `ctest --test-dir build/ci -R '^ports\.glasses\.rule$'` | 8 | rule fails |
| snapshot instead of the cut (`glasses_frame = eye` always) | `ctest --test-dir build/ci -R '^ports\.glasses\.control$'` | 8 | control fails (rule and state also fail) |
| reverted `src/io.c` | `ctest --test-dir build/ci -R '^ports\.glasses\.'` | 0 | 6 of 6 pass |

The same six cases also ran clean under `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` over `src/*.c` and the test (all exit 0), since the Homebrew clang-21 asan preset is not available here; the hosted asan job is the lane evidence.

## Hosted dispatch 1

Commands, from the repository root, no `cd`:

1. `git push origin phase/08-sports-pad-paddle-and-3-d-glasses` exit 0 (head `b05c7b4ffd8bab67342a42367c9f466912404fef`; the 08-01 to 08-03 commits went with it).
2. `gh workflow run ci.yml --ref phase/08-sports-pad-paddle-and-3-d-glasses` exit 0; `gh workflow run ci.yml --ref main` exit 0. Runs found by head SHA with `jq` over `gh run list --event workflow_dispatch --json databaseId,headSha,url`.
3. `gh run watch 42963275959 --exit-status` exit 0 and `gh run watch 42963279510 --exit-status` exit 0. Neither needed a second attempt.

Runs:

- Branch run: https://github.com/szTheory/cogmaster/actions/runs/42963275959 (success), head `b05c7b4ffd8bab67342a42367c9f466912404fef`.
- Main run: https://github.com/szTheory/cogmaster/actions/runs/42963279510 (success), head `bb7d15874cb357a366521e72046ad279441decb1`. `git rev-parse origin/main` and `git merge-base origin/main HEAD` both print that SHA, so the base is the branch's own merge base and origin/main has not moved.

Branch run jobs: hygiene, changes, nofp (ubuntu-26.04), nofp (ubuntu-26.04-arm), perf, asan, be32, wasm32, hash-equality, `build` on linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64 and windows-arm64, retroarch and CI required all success; title, rehearsal and cleanup skipped as for any dispatch. In the run's log the asan job lists `ports.input.exact_malloc` and the six `ports.glasses.*` cases as Passed; the be32, wasm32, windows-x64 and windows-arm64 legs list `abi.api_v12`, `ports.input.exact_malloc` and the six glasses cases Passed; the nofp legs list `abi.api_v12` Passed (the nofp preset registers no `ports.*` case).

Control lines, verbatim:

```
perf control boot_to_play runs=12302997,12302997,12302997 spread=0
perf control boot_to_play runs=12302854,12302854,12302854 spread=0
```

The first is the branch run, the second the main run. Both spreads are 0, so H is the first value and B is the first value.

```
perf dispatch boot_to_play H=12302997 B=12302854 delta_bp=0
```

`delta_bp=$(( (H - B) * 10000 / B ))` in shell integer arithmetic is 0 (the exact ratio is 1.16 hundredths of a percent). `git diff --quiet origin/main -- tests/perf/allow.txt` exits 0, so no allow line was added.

## Task Commits

1. **Task 1: The glasses hook, the cut rule, the state bytes and the cross-checks** - `fd9c236` (feat)
2. **Task 2: Boundary, control, gate, profile, lag and state cases** - `b05c7b4` (test)
3. **Task 3: Hosted dispatch 1** - no source change; evidence in this file

## Verification run

- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8424 src lines).
- `ctest --test-dir build/ci -R '^(io\.|bus\.|audio\.|core\.|vdp\.|fixtures\.|abi\.(globals|undefined|symbols)|ports\.)'` passed 181 of 181 after Task 1 (the plan's figure of 172 was the count before plan 08-03's six cases and this plan's six); the full `ctest --test-dir build/ci` passed 481 of 481.
- `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0: no existing pin, hash or io case moved.
- Acceptance greps: `GLASSES_CUT_TICKS` appears in `src/internal.h` and `src/io.c`; `addr >= 0xFFF8u` appears once and `addr >= 0xFFFCu` zero times in `src/bus.c`; `sizeof(struct cogmaster__io) == 11u` once in `src/state.c`; six `ports.glasses.*` names registered and in both inventories; `Fault prevented: a 3-D eye` once; five `core.state.*` tests; none of 299, 598 or 1536 in the new test.

## Deviations from Plan

### Auto-fixed Issues

None - the plan executed as written. Notes that are not deviations:

- The Task 1 regex count is 181, not 172 (see above); the criterion's floor of 170 holds.
- The window-relative `win` in the helper is a `uint64_t` expression `m->ticks - (m->bus.deadline - frame_ticks)`, the form the plan gives; the CPU is always past the previous deadline, so it never underflows.
- The state case reaches the pair `(glasses 2, glasses_frame 1)` by a real run (a right eye written before the frame, a left-eye store after the cut) rather than by patching fields.

**Total deviations:** 0. **Impact:** none.

## Known Stubs

None.

## Threat Flags

None. The new write path adds no endpoint, file access or trust boundary; the eye is write-only to the guest and never reaches a hashed byte.

## Issues Encountered

None. Local libFuzzer, nofp (GCC) and the clang-21 asan preset cannot run on this Mac; the hosted runs above are their evidence, and an Apple clang address and undefined sanitizer build of the six new cases was the local stand-in.

## Next Phase Readiness

Ready for 08-05. Hosted dispatch 1 is done and green, so the device plans stack on a measured baseline (boot_to_play 12302997 Ir/frame on the branch against 12302854 on main). Plan 08-12 needs the `--eyes` runner flag and the fixture; the host composition in 08-14 and 08-15 reads `run_result.glasses`.

## Self-Check: PASSED

- `tests/ports/test_glasses.c` exists; commits `fd9c236` and `b05c7b4` are ancestors of HEAD.
- Both run URLs answer success through `gh run view <id> --json conclusion`, the branch run's head SHA has no non-`.planning` difference from HEAD, and the main run's head SHA equals `git rev-parse origin/main`.
