---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "09"
subsystem: runner
tags:
  - runner
  - input-script
  - deltas
  - eyes
status: complete

requires:
  - 08-08
provides:
  - "the input script's 75-byte line: the 35 bytes then x1 y1 x2 y2 with a mandatory sign and five digits, range -32767..32767"
  - "validation against the resolved devices before frame 1, naming the port and the device when a pad or an empty port is given motion"
  - "--eyes: one line after the last frame counting the 3-D glasses eye per frame (none, left, right, flips, last)"
affects: [08-10, 08-13, 08-17]

actuals:
  tokens: 6400
  tasks: 2
  commits: 2

plan_head_before: 67063a08cc81b47cc5dd0f5b1fcff2a8ed39b416
plan_head_after: cc871fdea6725c34330cc3cfca8ef11e53a812bd
commits: 2

tech-stack:
  added: []
  patterns:
    - "Fixed-offset field parser for the axes (no locale, no strtol, no allocation); a line is exactly 35 or 75 bytes"
    - "validate takes the two resolved devices and an out parameter that names the refused port, so main.c prints the message and every other refusal keeps its old text"

key-files:
  created: []
  modified:
    - runner/input.c
    - runner/input.h
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - tests/runner/tests.cmake
    - docs/runner.md
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "A Paddle and a Sports Pad accept any axis values; a pad or an empty port accepts only zero on its two axes (port 1 reads x1 and y1, port 2 reads x2 and y2); the core ignores a Paddle's y"
  - "The refusal text is its own line, input file error: port N is DEVICE, which takes no motion, so malformed-grammar cases keep matching input file error (status N) and the device cases name the port and device"
  - "The eyes line prints last, after the expect block, so --hash, --png, --expect and lag_frames= keep their place and form"

requirements-completed: []

coverage:
  - id: R1
    description: "A 35-byte line plays as before with all four axes 0; a 75-byte line carries x1 y1 x2 y2 with sign and five digits; every length from 36 to 74, 76 bytes, a missing field, a wrong order, a 4-digit width, a missing sign, +32768, -32768 and +99999 are exit 2 before frame 1 with no frame line printed; the range edges, CRLF and a missing final newline are accepted"
    requirement: PAD-03
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#axes_grammar via ctest runner.input.axes_grammar"
        status: pass
    human_judgment: false
  - id: R2
    description: "A non-zero axis on a port whose resolved device is a pad or none is exit 2 before frame 1 naming the port and the device, on either port and either axis and on a later record; Sports Pad and Paddle accept motion; an all-zero 75-byte line is accepted for any device"
    requirement: PAD-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#axes_device via ctest runner.input.axes_device"
        status: pass
    human_judgment: false
  - id: R3
    description: "The 35-byte script and the same script in the all-zero 75-byte form give identical output (frame hash and lag count) on a pad and a Sports Pad; scripts one digit apart give different script digests in the execution line and a pad-only image keeps one frame hash"
    requirement: PAD-03
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#axes_equivalent via ctest runner.input.axes_equivalent"
        status: pass
    human_judgment: false
  - id: R4
    description: "--eyes prints eyes frames=F none=N left=L right=R flips=X last=left|right|none after the last frame, the counts add up to the frames run, the other output is byte-equal with and without it, it follows lag_frames=, and with --probe in either argument order it is a usage error (exit 2)"
    requirement: PAD-04
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#eyes_line and eyes_probe via ctest runner.eyes.line and runner.eyes.probe"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-11
---

# Phase 8 Plan 9: Runner delta lines and the eyes report Summary

**The runner's input script takes a 75-byte line carrying the four axis deltas (sign and five digits each, range 32767), refuses a delta a port's resolved device cannot take before frame 1 with a message naming the port and the device, and `--eyes` counts the 3-D glasses eye per frame into one line with no composition.**

## Accomplishments

- `runner/input.c`: `cogmaster__input_read` accepts exactly 35 or 75 bytes. A small fixed-offset helper parses ` x1=+DDDDD` fields at offsets 35, 45, 55 and 65, checks the sign, the five digits and the 32767 bound, and fills the record's four axes; any other byte or length is exit 2. `cogmaster__input_validate` gains the two resolved devices and an out parameter; a non-zero axis on a pad or an empty port stops validation with `*bad_port` set, so the whole body is checked before frame 1 and then rewound as before.
- `runner/main.c`: the order is load, resolve the devices (plan 08-08), validate against them, run. Each frame's deltas ride the same `cogmaster_set_input` call as its buttons, once. Per frame the eye from `cogmaster_run_result.glasses` is counted; `eyes frames=F none=N left=L right=R flips=X last=...` prints after every other output line. `flips` counts frames whose non-zero eye differs from the last non-zero eye.
- `runner/args.c`, `runner/args.h`: `--eyes` with a help line; with `--probe` it joins the existing "runs nothing" usage error.
- `tests/runner/tests.cmake`: `runner.input.axes_grammar`, `runner.input.axes_device`, `runner.input.axes_equivalent`, `runner.eyes.line` and `runner.eyes.probe`, labels `portable runner`, TIMEOUT 30, under the "Fault prevented" comment. All assert on exit status and tolerant regexes. Inventories ci and wasm32 list the five names.
- `docs/runner.md`: the 75-byte form with an example, the sign and width rule, the refusals, the per-port axis mapping and the `--eyes` option row (about +1.2 KB; the file is 30,487 bytes against a 38,912 ceiling).

## Negative control

The edit let the reader accept a 74-byte line (`n != 74u` added to the length test). `git diff --stat runner/input.c` showed it changed and the build exited 0 before the test ran.

| Control | Command | Exit | Result |
|---|---|---|---|
| 74-byte line accepted | `ctest --test-dir build/ci -R '^runner\.input\.axes_grammar$'` | 8 | fails: "expected exit 2" |
| reverted (`git checkout runner/input.c`) | `ctest --test-dir build/ci -R '^runner\.input\.axes_grammar$'` | 0 | passes |

## Verification run

- Task 1 verify: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(runner\.|fixtures\.|library\.)'` exit 0 (122 of 122 before the new cases); `-N` over the same regex shows 127 after them (at least 118 required); `grep -c 'eyes frames=' runner/main.c` prints 1; `grep -c '"--eyes"' runner/args.c` prints 1; `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0; `cmake --workflow --preset hygiene` exit 0.
- Task 2 verify: `ctest --test-dir build/ci -R '^(runner\.|docs\.)'` exit 0 (92 of 92); `-N -R '^runner\.(input\.axes_|eyes\.)'` lists the five names; five matching lines in each of tests/inventory/ci.txt and wasm32.txt; the Fault-prevented comment appears once; `grep -c 'eyes frames=' docs/runner.md` is at least 1; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8793 src lines).
- Plan: `cmake --workflow --preset ci` exit 0 (507 tests plus 4 release tests); `cmake --workflow --preset hygiene` exit 0 (48 tests); clang-format 21 dry run on runner/*.c and runner/*.h exit 0; `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0.
- No push and no hosted dispatch; the plan names none. Run URLs: none for this plan.

## Task Commits

1. **Task 1: The 75-byte line, the per-device check and --eyes** - `b5d6a00` (feat)
2. **Task 2: Grammar negatives, the replay-equivalence cases and the docs** - `cc871fd` (test; also rewords the device refusal line in `runner/main.c`)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The device refusal read "whose none takes none"**
- **Found during:** Task 2, writing `runner.input.axes_device`
- **Issue:** the Task 1 message "motion for port N, whose DEVICE takes none" read badly for the device word `none`.
- **Fix:** the line is now `input file error: port N is DEVICE, which takes no motion`; the cases pin that form.
- **Files modified:** runner/main.c
- **Committed in:** cc871fd

**Total deviations:** 1 auto-fixed. **Impact:** none on scope.

## Phase notes

- The delta-to-hash replay differential needs a guest that reads the device and lands in plan 08-10. Here the proof is grammar, per-device refusal, script identity (digest) and 35 versus all-zero 75-byte equality; the test card and a pad-only image never read a controller, so scripts one digit apart legitimately share a frame hash.
- The `--eyes` counts here are pinned on the test card (all none). The table-derived counts, `flips` and `last` on real alternation come with the glasses fixture in plan 08-12.

## Known Stubs

None.

## Threat Flags

None. The new reader is fixed-size, allocation-free and bounded by the 96-byte line buffer; it adds no file access beyond the existing `--input` file.

## Issues Encountered

None.

## Next Phase Readiness

Ready for plan 08-10. The branch has the 08-05 to 08-09 commits locally (pushed through `b05c7b4`).

## Self-Check: PASSED

- All nine modified files exist; commits `b5d6a00` and `cc871fd` are ancestors of HEAD.
