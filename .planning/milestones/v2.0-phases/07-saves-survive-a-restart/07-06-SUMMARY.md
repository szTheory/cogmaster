---
phase: 07-saves-survive-a-restart
plan: "06"
subsystem: testing
tags:
  - eeprom
  - fuzz
  - runner
  - save
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-03 EEPROM board and plan 07-05 owned eeprom.gg fixture
provides:
  - a fuzz entry that clocks the 93C46 with hostile write streams and checks its bounds after every write
  - a generated eeprom-clocks seed that reaches every chip phase in the replay
  - a runner test that continues the owned EEPROM fixture across two fresh processes
affects: [07-10]

actuals:
  tokens: 4600
  tasks: 2
  commits: 2

plan_head_before: 650e236d3747af3d6e90af28361983780effdbd1
plan_head_after: 6388449d2f78e256aa6991a8626dfc70cf7d498f
commits: 2

tech-stack:
  added: []
  patterns:
    - "A fuzz entry checks a device's struct fields against bounds written out in the entry, not against the device's own macros"
    - "A generated seed gives the plain replay a meaningful input for an entry whose input format is not a ROM image"

key-files:
  created:
    - tests/fuzz/fuzz_eeprom.c
  modified:
    - tests/fuzz/tests.cmake
    - tests/fuzz/seeds.c
    - tests/inventory/fuzz.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/runner/tests.cmake
    - docs/perf-fuzz.md
    - docs/saves.md

key-decisions:
  - "The fuzz entry opens the chip window itself before the first pair, so every $8000 write in the input reaches the chip; a $FFFC write in the input may still close it"
  - "The bounds are tighter than the state loader's: a command counts at most 7 clocks while it is taken in, since the eighth completes it"
  - "A generated seed, not a committed regression file, gives the replay a clock stream (no bytes enter the tree)"

requirements-completed: [SAVE-01, SAVE-02]

coverage:
  - id: F1
    description: "fuzz.regress.eeprom replays every seed through the EEPROM entry on every portable lane and fuzz.libfuzzer.eeprom is registered on the fuzz lane"
    requirement: SAVE-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fuzz\\.regress\\.'"
        status: pass
    human_judgment: false
  - id: F2
    description: "The chip's phase, count and flag fields stay inside their bounds after every write of a hostile stream, and a saved state loads into a second instance and saves byte-equal"
    requirement: SAVE-02
    verification:
      - kind: unit
        ref: "tests/fuzz/fuzz_eeprom.c via fuzz.regress.eeprom; negative controls exit 8 under ctest"
        status: pass
    human_judgment: false
  - id: F3
    description: "The runner continues eeprom.gg across two fresh processes: the 128-byte save starts 34 12 01 00 then 34 12 02 00 and the mailbox reports read-back equal and the post-EWDS write refused"
    requirement: SAVE-01
    verification:
      - kind: e2e
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.continuation\\.eeprom$'"
        status: pass
    human_judgment: false
  - id: F4
    description: "No audio, frame or built pin line changes: tests/audio, tests/testcard_pins.cmake and tests/built carry no removed line against origin/main"
    requirement: SAVE-01
    verification:
      - kind: other
        ref: "git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake; grep -c '^-[^-]' of git diff origin/main -- tests/built"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-11
---

# Phase 7 Plan 06: The EEPROM fuzz entry and the runner restart Summary

The 93C46 bit-banger is fuzzed with random `$8000` and `$FFFC` traffic and bounds-checked after every write, and the owned EEPROM fixture's 128-byte save now continues across two fresh runner processes on every portable lane.

## What was built

- `tests/fuzz/fuzz_eeprom.c`: builds one Game Gear instance on the EEPROM board (64 KiB synthetic image, 128-byte save), opens the chip window, then reads the input as pairs, at most 4096 of them. The first byte's low bit picks `$FFFC` or `$8000`, the second is the value. After every write it reads `$8000` and checks the chip's fields directly, with the bounds written out in the entry: phase at most 4, count 0 when idle or complete, at most 7 while a command is taken in, at most 15 in data or read; reserved flag bits clear; chip select low means idle with count 0; the output pin is driven low only while reading; a pending program only after a complete command. Any violation calls `abort()`. At the end the state is saved, loaded into a second instance of the same image and saved again; the two saves and the 128 bytes of cart RAM must be byte-equal. Both instances and both buffers are freed before it returns.
- `tests/fuzz/tests.cmake`: `eeprom` joins both `foreach` lists (libFuzzer lane and plain replay) and both "Fault prevented" comments name the EEPROM clock stream.
- `tests/fuzz/seeds.c`: one more generated seed, `eeprom-clocks.bin`, an EWEN, a WRITE of word 5, a READ of word 5 with 20 further clocks, an abandoned ERAL, a start bit and the reset bit. Nothing is committed that is not source.
- `tests/inventory/fuzz.txt`, `ci.txt`, `wasm32.txt`: `fuzz.libfuzzer.eeprom` and `fuzz.regress.eeprom` (labels `fuzz` and `fuzz,portable`), `runner.save.continuation.eeprom` (labels `portable,runner`), sorted.
- `tests/runner/tests.cmake`: script case `save_eeprom` runs the production `cogmaster-run` on `fixtures/eeprom.gg` with `--frames 60 --mailbox --save-dir` twice in an emptied directory and requires exit 0, a 128-byte `eeprom.srm` starting `34120100` then `34120200`, the mailbox records `01=0001` then `01=0002`, `02=0001` (read-back equal) and `03=0001` (write after EWDS refused), and no leaked temporary. `SECOND` overrides the second expected prefix for the negative control. Registered as `runner.save.continuation.eeprom` under `COGMASTER_FIXTURES`.
- `docs/perf-fuzz.md`: the entry, the four targets, the new replay test. `docs/saves.md`: one sentence naming the EEPROM cases.

## Evidence

Every command is from the repository root; there is no hosted run URL in this plan (the branch was not pushed here and the plan names no dispatch). The fuzz lane of plan 07-10's hosted run is the libFuzzer evidence.

- `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^fuzz\.regress\.' --output-on-failure && cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0 (`fuzz.regress.eeprom` replays 44 inputs, 0.24 s; 723 check lines of 8268 src lines).
- `ctest --test-dir build/ci -R '^runner\.save\.' --output-on-failure` exit 0 (23 tests, `runner.save.continuation.eeprom` included).
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0.
- Negative control 1, the final comparison made against a zeroed buffer (state load and the second save replaced by a `memset` to zero): `ctest --test-dir build/ci -R '^fuzz\.regress\.eeprom$'` exit 8 (test failed); source restored, same command exit 0.
- Negative control 2, the per-write bound check tightened to demand count 0 in every phase: `ctest --test-dir build/ci -R '^fuzz\.regress\.eeprom$'` exit 8 (the generated seed clocks a start bit, so it trips); source restored, same command exit 0. No device edit was needed.
- Negative control for the runner case: `cmake "-DCMD=<cmake -DRUN=build/ci/cogmaster-run -DEMULATOR= -DSEEDS=build/ci/tests/seeds -DCASE=save_eeprom -DOUT=... -DEEPROM=build/ci/fixtures/eeprom.gg -DSECOND=34120100 -P tests/runner/tests.cmake>" -DCODE=1 "-DREGEX=expected 128 starting 34120100" -P tests/cmake/expect_fail.cmake` exit 0, meaning the case exited 1 with the byte-mismatch message; the same case with no override exits 0.
- `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` over `tests/fuzz/replay.c`, `tests/fuzz/fuzz_eeprom.c` and `src/*.c` replays the seeds, exit 0 (leak detection is not supported on this platform; the entry frees both instances and buffers).
- `cmake --preset fuzz` with the Homebrew clang-21 configures (exit 0), so the inventory and registrations agree.
- `grep -c 'fuzz.libfuzzer.eeprom' tests/inventory/fuzz.txt` prints 1; `grep -c 'fuzz.regress.eeprom'` on `ci.txt` and `wasm32.txt` prints 1 each; `grep -cE 'ITEMS loader run save eeprom' tests/fuzz/tests.cmake` prints 2; `grep -c 'runner.save.continuation.eeprom'` prints 1 for each inventory; `grep -c 'Fault prevented: an EEPROM save' tests/runner/tests.cmake` prints 1; `grep -cE 'abort\(|fuzz_fail' tests/fuzz/fuzz_eeprom.c` prints 5.
- `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` exit 0; `git diff origin/main -- tests/built` exit 0 with no removed line.
- Local libFuzzer was not exercised: `cmake --workflow --preset fuzz` needs a clang-21 libFuzzer runtime (Apple clang has none, as in earlier plans). A Homebrew clang-21 build of `fuzz_eeprom.c` with `-fsanitize=fuzzer,address,undefined` stalled in dyld initialisers before `main` on this machine, while a trivial libFuzzer target ran, so that is a local-host fault and not a finding; the fuzz lane of plan 07-10's hosted run is the libFuzzer evidence.

## Task Commits

1. **Task 1: The EEPROM fuzz entry on the fuzz lane and in the regression replay** - `8e75c0b` (test)
2. **Task 2: The EEPROM fixture continues across two runner processes** - `6388449` (test)

**Plan metadata:** the docs commit that holds this file.

## Decisions Made

- The harness opens the chip window before the first pair so the input's `$8000` writes reach the chip instead of recording a ROM-write fault; the input can still close it with a `$FFFC` write.
- The command-phase bound is 7 clocks, one tighter than the plan's 8: the eighth clock completes the command and resets the count, and the state loader already refuses a count of 8 in that phase.
- The replay is made meaningful with a generated seed rather than a committed regression file, so no new bytes need a manifest row.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] A generated EEPROM clock stream seed**
- **Found during:** Task 1 (negative control 2)
- **Issue:** The existing seeds are cartridge images and save blobs; read as byte pairs they hold no start bit with the window open, so the plain replay could pass a bound check that was wrong or never reached. The plan allowed for this case only by editing the device.
- **Fix:** Added `eeprom-clocks.bin` to `tests/fuzz/seeds.c`, generated at build time into the build tree. It also seeds the libFuzzer corpus.
- **Files modified:** tests/fuzz/seeds.c (not in the plan's file list)
- **Verification:** negative control 2 fails with it and passes without the tightening; `fuzz.regress.eeprom` replays 44 inputs.
- **Committed in:** 8e75c0b

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** One extra file in the task's own directory; no budget, label or inventory effect beyond the planned lines.

## Issues Encountered

- The two task commits were made from a working tree that held both tasks' edits; the shared inventories were committed in two steps (the runner line removed for the first commit, restored for the second), so each commit is complete and the full verification ran after the second.
- A local libFuzzer build could not be exercised (see Evidence).

## Known Stubs

None.

## Threat Flags

None. The change adds a test entry, a generated seed and a runner test; no network, auth or trust-boundary surface.

## Next Phase Readiness

Ready for plan 07-07. Plan 07-10's hosted run is where the fuzz lane and the other portable lanes confirm `fuzz.libfuzzer.eeprom`, `fuzz.regress.eeprom` and `runner.save.continuation.eeprom`.

## Self-Check: PASSED

- FOUND: tests/fuzz/fuzz_eeprom.c, tests/fuzz/tests.cmake, tests/fuzz/seeds.c, tests/inventory/fuzz.txt, tests/inventory/ci.txt, tests/inventory/wasm32.txt, tests/runner/tests.cmake, docs/perf-fuzz.md, docs/saves.md
- FOUND commits: 8e75c0b, 6388449
