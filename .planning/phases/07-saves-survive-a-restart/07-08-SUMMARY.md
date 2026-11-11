---
phase: 07-saves-survive-a-restart
plan: "08"
subsystem: runner
tags:
  - saves
  - runner
  - save-interval
  - eeprom
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-07 EEPROM import rule (0 or 128 bytes) and the existing runner flush
provides:
  - cogmaster-run --save-interval N flushes after the outputs of every frame n with n % N == 0; 0 or absent is off
  - a periodic flush failure exits 1 at once with no later frame and no hash line; N above --frames writes only at exit
  - usage refusals (exit 2) for a non-number, a value over 4294967295, a missing value, the option without --save-dir and the option with --probe
  - an EEPROM save of the wrong length is refused with exit 4 and a message naming the length wanted
affects: [07-09, 07-10]

actuals:
  tokens: 2242
  tasks: 3
  commits: 3

plan_head_before: 290402254a9d802f323f91f15147848ea9756cc7
plan_head_after: ef5b4599e4f56e8c1d116665c145c64538a42913
commits: 3

tech-stack:
  added: []
  patterns:
    - "A usage-only script case needs no fixtures, so its runner.usage test sits outside the fixtures guard; the fixture cases sit inside it"
    - "A periodic-failure test runs the normal_write test build, where every write of the save fails and no signal interferes"

key-files:
  created: []
  modified:
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - runner/save.c
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/runner.md

key-decisions:
  - "--save-interval 0 is accepted without --save-dir because 0 is off, the same as no option; only a non-zero interval needs --save-dir"
  - "parse_count is now parse_number (0 allowed) plus a non-zero wrapper, so the other numeric options are untouched"
  - "the periodic flush sits after the frame's outputs and before the stop check; the exit flush is unchanged and so prints a second error line after a periodic failure"
  - "the EEPROM refusal tells an oversize file (n > span) from any other refused length by comparing the read length with the span"

requirements-completed: []

coverage:
  - id: R1
    description: "--save-interval N flushes every Nth frame without changing the frame hash or the save bytes; 0 is off and N above --frames writes only at exit"
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.interval$'"
        status: pass
    human_judgment: false
  - id: R2
    description: "A non-number, a value over 4294967295, a missing value, the option without --save-dir and the option with --probe each exit 2 with the usage message"
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.usage\\.save_interval$'"
        status: pass
    human_judgment: false
  - id: R3
    description: "A periodic flush failure exits 1 at once with no frame 10 line; an interval above --frames prints the frame 10 line and fails only at the exit flush"
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.interval_fault$'"
        status: pass
    human_judgment: false
  - id: R4
    description: "An EEPROM save of a length other than 0 or 128 is refused before running with exit 4 and a message naming the expected length, the file unchanged; a 128-byte file runs"
    requirement: SAVE-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.import$'"
        status: pass
    human_judgment: false
---

# Phase 7 Plan 08: Periodic flush and the EEPROM length message Summary

`cogmaster-run --save-interval N` writes the save every N frames (frames, never time) and ends the run at once when that flush fails, and a mis-sized EEPROM save is refused with exit 4 and a message naming the 128-byte length.

## Accomplishments

- `runner/args.c`, `runner/args.h`: a `uint32_t save_interval`, parsed digits-only and range-checked before narrowing, with a usage line. A non-zero interval without `--save-dir`, and any interval with `--probe`, is a usage error.
- `runner/main.c`: after frame n's outputs and before the stop check, a non-zero interval with `n % interval == 0` calls the existing `cogmaster__save_flush`. A non-OK result sets the failure status and ends the loop, so no later frame runs and no hash line prints. The exit flush is untouched.
- `runner/save.c`: a size refusal prints `oversized file` when the file is longer than the span and `file length <n> does not match the <length>-byte save` otherwise; both exit 4.
- Tests: `runner.usage.save_interval`, `runner.save.interval` and `runner.save.interval_fault` are registered with `portable,runner` in `tests/inventory/ci.txt` and `wasm32.txt`; `runner.save.import` gained the EEPROM length checks (100 bytes, 129 bytes, 128 bytes) with no new test name.
- `docs/runner.md`: the option row, a paragraph on the periodic flush and its failure, and the corrected "short files pad with FF" sentence (battery saves only; an EEPROM save is 0 or 128 bytes). The file is 27,637 bytes against the 38,912 cap.

## Task Commits

1. **Task 1: --save-interval end to end, with its usage refusals** - `8f51c49` (feat)
2. **Task 2: periodic failure exits at once; N above --frames writes only at exit** - `cf11e4e` (test)
3. **Task 3: the EEPROM length message** - `ef5b459` (feat)

## Evidence

All run from the repository root. Local runs only; the hosted round trip belongs to plan 07-09.

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(runner\.\|policy\.inventory)'` (66 tests; the inventory check runs at configure) | 0 |
| `ctest --test-dir build/ci -R '^(runner\.save\.\|policy\.inventory)'` (25 tests) | 0 |
| `ctest --test-dir build/ci -R '^runner\.'` (67 tests, after Task 3) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` (723 check lines, 8270 src lines, docs under cap) | 0 |
| `cmake --workflow --preset hygiene` | 0 (the first run timed out `hygiene.tree` at 60 s under load; it passed alone and the full workflow then passed) |
| `cmake --workflow --preset ci` | 0 |
| `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` | 0 |

Acceptance greps: `grep -c 'save-interval' runner/args.c` prints 4 and `docs/runner.md` 1; the inventory rows for `runner.save.interval`, `runner.usage.save_interval` and `runner.save.interval_fault` print 2 and 1 in `ci.txt` and `wasm32.txt`; `grep -c 'Fault prevented: a periodic save failure' tests/runner/tests.cmake` prints 1; `grep -cE 'does not match the' runner/save.c` prints 1.

Negative controls, each reverted afterwards (ctest exit 8 means a test failed):

| Control | Result |
| --- | --- |
| range check removed from the number parser in `runner/args.c`, `ctest -R '^runner\.usage\.save_interval'` | exit 8; restored, exit 0 |
| a non-digit ends the parse instead of failing it (accepts `1x`), same command | exit 8; restored, exit 0 |
| `runner/main.c` ignores a failed periodic flush and carries on, `ctest -R '^runner\.save\.interval_fault'` | exit 8; restored, `-R '^runner\.save\.interval'` exit 0 |
| `runner/main.c` flushes on every frame whatever the interval, same command | exit 8; restored, exit 0 |
| `runner/save.c` prints the oversize message for every size refusal, `ctest -R '^runner\.save\.import$'` | exit 8; restored, `-R '^runner\.'` exit 0 |

Battery first-write check (Task 2): `build/ci/cogmaster-run --frames 1 --save-dir <new dir> build/ci/fixtures/battery.sms` exit 0 and wrote `battery.srm`; the same with `--frames 10 --hash` exit 0, hash `3a3ed164...e6e4`. The generation therefore moves at frame 1, before the first flush at frame 1 and well before frame 10, so interval 1 fails at frame 1 and interval 20 over 10 frames fails only at the exit flush. The tests use the planned values (1 and 20 against 10); no value was raised. By hand with the `normal_write` build: interval 1 exits 1 with two error lines and no frame line; interval 20 prints the frame 10 line and exits 1.

## Deviations from Plan

### Auto-fixed Issues

None - no deviation rule applied.

### Notes on how the plan was followed

- The runner plans follow the import plan (CONTEXT listed them first), so the EEPROM length message lands with the runner change, and the runner durability work is split between this plan and 07-09 (sweep, directory sync, kill tests).
- Task 2 case (b) checks standard output and standard error apart. The runner flushes standard output after the exit flush's message, so a merged capture orders the failure message first; checking the two streams separately keeps the test independent of that order.
- `--save-interval 0` without `--save-dir` is accepted: 0 is off, the same as no option.
- The periodic failure path leaves the exit flush as it was, so a failed periodic flush is followed by a second failure message from the exit flush before the exit status 1.
- `requirements-completed` is empty by plan: SAVE-05 completes with plan 07-09; SAVE-03 is listed by the plan but its import half was recorded in 07-07 and the length message here is a runner-side addition.

## Issues Encountered

`hygiene.tree` timed out at 60 s inside the full hygiene workflow when run straight after a build; alone it passed (38 s) and a second full workflow run exited 0.

## Known Stubs

None.

## Threat Flags

None. The only new input is a numeric command-line option parsed with an explicit range check before narrowing, and no new file or network surface.

## Next Phase Readiness

Plan 07-09 can add the sweep, the directory sync and the kill tests on top of the flush that `--save-interval` now calls.

## Self-Check: PASSED

- `runner/args.c`, `runner/args.h`, `runner/main.c`, `runner/save.c`, `tests/runner/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`, `docs/runner.md` all exist and are modified in the three task commits.
- Commits `8f51c49`, `cf11e4e`, `ef5b459` are ancestors of HEAD.
