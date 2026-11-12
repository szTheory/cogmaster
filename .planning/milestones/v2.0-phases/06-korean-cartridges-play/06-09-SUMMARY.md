---
phase: 06-korean-cartridges-play
plan: "09"
subsystem: runner
tags:
  - runner
  - probe
  - refusal
  - board-detection
status: complete

requires:
  - phase: 06-korean-cartridges-play
    provides: plan 06-02 the cart_info fields; plan 06-08 guess_board and the scan that fill them
provides:
  - seven new --probe keys after title (board_source, board_candidate, scan_sega, scan_candidate, scan_runner_up, unsupported_id, unsupported_reason)
  - mapper_guess names for every board the catalog names
  - a refusal message that names the board and the reason, exit 77
  - janggun-board.sms, a synthetic seed that exercises the named-unsupported path end to end
affects: [06-11]

actuals:
  tokens: 1700
  tasks: 2
  commits: 3

plan_head_before: 906a25ddfa3a76c6a6c759cd1feaf28d17047a62
plan_head_after: 99317e34bbcf3f7ef5ea06eac5aa5f1f88d6074d
commits: 3

tech-stack:
  added: []
  patterns:
    - "The runner probes the refused image a second time to explain the refusal; the core stays the only place that knows why"

key-files:
  created: []
  modified:
    - runner/probe.c
    - runner/probe.h
    - runner/main.c
    - tests/runner/tests.cmake
    - tests/fuzz/seeds.c
    - docs/runner.md

key-decisions:
  - "The refusal text lives in probe.c beside the board names (cogmaster__refusal_board) so main.c keeps one call; it returns 0 when there is nothing to add and the old message stays"
  - "No test was added: the existing runner.probe.keys, runner.probe.unsupported and runner.refuse.profile cases were extended, so tests/inventory is untouched"

requirements-completed:
  - MAP-05
  - MAP-06

coverage:
  - id: C1
    description: "--probe prints board_source, board_candidate, scan_sega, scan_candidate, scan_runner_up, unsupported_id and unsupported_reason after title in a documented form, and mapper_guess names every board"
    requirement: MAP-06
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^runner\\.probe\\.' exit 0 (expect_probe_form regex over header-export-128k, size-16k-512, unsupported-board and janggun-board)"
      status: pass
    human_judgment: false
  - id: C2
    description: "A header seed probes with board_source header, and a headerless image with two Korean-looking stores probes with board_source heuristic and board_candidate korean, exit 77"
    requirement: MAP-06
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^runner\\.probe\\.(keys|unsupported)$' exit 0"
      status: pass
    human_judgment: false
  - id: C3
    description: "A Janggun-looking image probes with exit 77 and an unsupported_reason naming janggun and its reason, and a run of it exits 77 with a message containing the reason; the doubtful scan seed exits 77 with a message naming the candidate"
    requirement: MAP-05
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^runner\\.probe\\.unsupported$' exit 0 (expect_exit 77 over unsupported-board.sms and janggun-board.sms)"
      status: pass
    human_judgment: false
  - id: C4
    description: "The new keys and refusal message are in docs/runner.md and the new seed replays under the fuzz targets"
    requirement: MAP-06
    verification:
      kind: other
      ref: "grep -q board_source docs/runner.md exit 0; ctest --test-dir build/ci -R '^(runner\\.|fuzz\\.)' exit 0, 66 of 66 passed"
      status: pass
    human_judgment: false
  - id: C5
    description: "The whole plan passes the full local workflow and hygiene, stays inside the check-line budget, adds no test name and leaves tests/audio, tests/testcard_pins.cmake and tests/built untouched"
    requirement: MAP-05
    verification:
      kind: other
      ref: "cmake --workflow --preset ci exit 0 (427 tests passed); cmake --workflow --preset hygiene exit 0 (48 of 48); cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines of 7953 src lines); git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake exit 0; grep -c '^-[^-]' over the tests/built diff prints 0; git log for tests/inventory lists none of this plan's subjects"
      status: pass
    human_judgment: false
---

# Phase 6 Plan 09: Runner shows board source and refusal reason Summary

`cogmaster-run --probe` now says where a board came from (`board_source`, the scan counts and the unsupported slot after `title`), `mapper_guess` names all nine boards, and a refused load exits 77 with a message naming the board and the reason or, for a doubtful scan, the candidate and the counts.

## What was built

- `runner/probe.c`: `cogmaster__board_name` (unsupported, none, sega, codemasters, korean, korean_nb, hicom, zemina, nemesis, hes_4pak; out of range reads unsupported) replaces the four-name list; the probe prints the seven new keys after `title=`, so readers of the older keys see no change in order. `cogmaster__refusal_board` probes the same image and prints `unsupported: board <slot>: <reason>`, or `unsupported: the board scan is not sure about <name> (sega <s>, <name> <x>, runner-up <y>)`, and returns 0 when neither applies.
- `runner/main.c`: `load_status` takes the image, and for the unsupported-board status calls `cogmaster__refusal_board`, keeping the old `the cartridge's board` message when it has nothing to add. The exit stays 77.
- `tests/fuzz/seeds.c`: `janggun-board.sms`, 64 KiB, four `LD A,n; LD ($6000),A` pairs then DI and HALT; synthetic bytes, no ROM data. The core's named row for Janggun is therefore exercised end to end without a committed ROM byte.
- `tests/runner/tests.cmake`: the probe form regex lists the new `mapper_guess` words and keys; `runner.probe.keys` checks `board_source=header` on a header seed; `runner.probe.unsupported` checks `heuristic` and `korean` for the two-store seed, its refusal text, and the Janggun seed's probe and run. No test name was added.
- `docs/runner.md`: the five key rows, the new `mapper_guess` words and one sentence on the refusal message.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] clang-format was not on PATH, so the first commit was unformatted**
- **Found during:** Task 2 hygiene run (`hygiene.format` failed on `runner/probe.c`)
- **Issue:** the plan's task commits were made before the repository's own clang-format 21 was run on the new code
- **Fix:** ran `/opt/homebrew/opt/llvm@21/bin/clang-format -i` over the touched C files; committed as its own style commit
- **Files modified:** runner/probe.c
- **Commit:** 2d5c19e

No other deviations; the plan's two tasks were executed as written (three commits: feature, formatting, tests and docs).

## Known Stubs

None.

## Threat Flags

None. The refusal path reads only the image the runner already holds and prints fixed-size fields; the reason string is at most 127 bytes and NUL-terminated by the library.

## Evidence

- `cmake --preset ci && cmake --build --preset ci && build/ci/cogmaster-run --probe build/ci/tests/seeds/unsupported-board.sms` exit 77 with `board_source=heuristic`, `board_candidate=korean`, `scan_candidate=2`
- `ctest --test-dir build/ci -R '^(runner\.|fuzz\.)'` exit 0 (66 of 66)
- `cmake --workflow --preset hygiene` exit 0
- `cmake --workflow --preset ci` exit 0 (427 tests passed); no hosted run yet (local only)

## Self-Check: PASSED
