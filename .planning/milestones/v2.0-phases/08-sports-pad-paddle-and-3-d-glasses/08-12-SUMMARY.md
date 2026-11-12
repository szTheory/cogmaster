---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "12"
subsystem: fixtures
tags:
  - fixtures
  - glasses
  - eyes
  - hosted-dispatch
status: complete

requires:
  - 08-11
provides:
  - "glasses.sms: a 120-line pure-assembly image that writes the shutter register from a per-frame table, holds one eye after a single write, then alternates from a 16-byte routine in work RAM with the cartridge off"
  - "eye-count cases on the export, Japan and Game Gear profiles computed from the table, with a control and a frame-hash-equality case"
  - "a glasses device row in hashes.cmake so the frame is compared across lanes"
  - "hosted dispatch 2 green on every lane, perf delta 0 basis points"
affects: [08-13, 08-17]

actuals:
  tokens: 3700
  tasks: 3
  commits: 5

plan_head_before: a9e4409994e0ead93004c1994b0a37bdc78d896f
plan_head_after: 60c97a6a21324d74b32cb6e0bf862866e9a07c2e
commits: 5

tech-stack:
  added: []
  patterns:
    - "The table in the assembly is the oracle: tests.cmake holds the same list, reads the one .DB row of the source at configure time and fails on any difference, then derives none, left, right, flips and last by simulating the frames"
    - "A RAM routine that has no absolute address is assembled in place in ROM between two labels and copied with LDIR, so one source line set is both the bytes and the listing"
    - "Exact-byte files in script-mode tests go through file(CONFIGURE) with a newline style, because file(WRITE) writes CRLF on Windows"

key-files:
  created:
    - tests/roms/src/glasses.asm
    - tests/roms/src/glasses.link
  modified:
    - tests/roms/manifest.txt
    - tests/built/expected.txt
    - tests/diag/expected.csv
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - src/state.c
    - tests/fuzz/fuzz_pad.c
    - tests/runner/tests.cmake

key-decisions:
  - "The run is 30 frames: the table's 24 plus six for the mailbox work (its CRC takes about three frames), so the held eye of the tail is part of the derived counts"
  - "The mailbox is judged in a second run inside each eyes case, not by stopping the eyes run at done: done lands at a frame that depends on CRC time, which is not in the table"
  - "The negative control that fails is the early-write path: an end-of-window snapshot cannot be told from the rule by a program whose every write is early, so that mutation passes here and is pinned by the library cases instead"
  - "The hash row uses a neutral 30-line script whose line count is the frame count, the form the row list already requires"

requirements-completed: []

coverage:
  - id: C1
    description: "The glasses image writes the shutter from a table of one entry a frame (two frames with no write, eight alternating, one write held for six, eight alternating from a 16-byte RAM routine run with the cartridge off), and the runner prints none 2, left 14, right 14, flips 15, last left over 30 frames on the export and Japan profiles, the counts computed from the same literal table that is checked against the assembly source"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest fixtures.glasses.eyes.export and fixtures.glasses.eyes.japan"
      status: pass
    human_judgment: false
  - id: C2
    description: "A Game Gear profile prints none for all 30 frames with no left, no right and no flip"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest fixtures.glasses.eyes.gg"
      status: pass
    human_judgment: false
  - id: C3
    description: "The same case with the expected left count one too high fails naming that count, so the counts are really compared"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest fixtures.glasses.eyes.control"
      status: pass
    human_judgment: false
  - id: C4
    description: "The last frame's hash is the same with and without the eye report, and the image's two mailbox rows (the routine's length and the mirror byte it left) hold on all three profiles"
    requirement: PAD-04
    verification:
      kind: integration
      ref: "ctest fixtures.glasses.hash_same and fixtures.glasses.eyes.export"
      status: pass
    human_judgment: false
  - id: C5
    description: "Hosted dispatch 2 passes on every lane (hygiene, asan, be32, wasm32, nofp on two architectures, six build legs including MSVC, perf, retroarch, hash-equality) with the glasses, Sports Pad, Paddle, runner grammar and device tests Passed on each, the cross-lane hash file byte-identical with the three device rows, and a perf delta of 0 basis points"
    requirement: PAD-04
    verification:
      kind: e2e
      ref: "https://github.com/szTheory/cogmaster/actions/runs/42973268415"
      status: pass
    human_judgment: false

duration: 14min
completed: 2026-11-11
---

# Phase 8 Plan 12: The 3-D glasses from the Z80 side and hosted dispatch 2 Summary

**A pure-assembly program writes the shutter register from a frame table, from ROM, as one held write, and from a 16-byte routine in work RAM with the cartridge off the bus; the runner's eye counts equal the ones simulated from that table on the export and Japan profiles and read none on a Game Gear; and the second hosted dispatch, after two GCC-only errors and four Windows-only failures were fixed, is green on every lane with the perf delta at 0.**

## Accomplishments

- `tests/roms/src/glasses.asm` (120 lines): plain assembly from the hardware document. A table of one byte a frame (0 or 1 is the byte written, `$FF` no write) drives frames 1 to 16 from ROM: two frames with no write, eight alternating, one write and five held. Frames 17 to 24 run from a 16-byte routine copied to `$C300`: it takes `$3E` with bit 6 set (cartridge off), writes the counter at `$FFF8`, waits for the frame flag, repeats eight times and writes `$AB` back. Two mailbox records: the routine's length (16) and the byte the work RAM mirror holds afterwards (7, the eighth write counting up from 0, bit 0 set).
- `tests/fixtures/tests.cmake`: the literal table, a configure-time comparison with the `.DB` row of the source, the simulation of 30 frames (a write lands in its own frame, the eye carries into a frame with no write, flips count a change of the non-zero eye), the `eyes` and `eyes_hash` cases, the five registrations, and a neutral 30-line script for the cross-lane row. The eyes case runs the runner twice: `--frames 30 --eyes` for the counts and `--frames 120 --mailbox` for the rows, which are judged by the existing mailbox judge.
- `tests/cmake/hashes.cmake`: one entry in the device row list, `glasses.sms|glasses|` (the row's script field names the file that gives the frame count; no extra arguments). `build/ci/hashes.txt` now holds `playing glasses.sms sms ntsc-j device frame 30 sha256 ...` (baseline 0).
- Data files: one line each in `tests/built/expected.txt` and `tests/roms/manifest.txt`, two rows in `tests/diag/expected.csv`, five names in `tests/inventory/ci.txt` and `wasm32.txt`. No existing line moved (`grep -c '^-[^-]'` over the diff against origin/main prints 0).

## Counts the table predicts (and the runner prints)

Frames 1 and 2 none; 3 to 10 right, left alternating (4 each); 11 right (one write) held through 16; 17 to 24 right, left alternating (4 each, the first the same eye as the held one, so no flip); 25 to 30 the held left. So none 2, right 14, left 14, flips 15, last left. `cogmaster-run glasses.sms --frames 30 --eyes` prints `eyes frames=30 none=2 left=14 right=14 flips=15 last=left` for ntsc-u and ntsc-j (exit 0), and `none=30 left=0 right=0 flips=0 last=none` with `--machine gg`.

## Negative controls (core mutations, run once locally)

| Mutation | Command | Exit | Result |
|---|---|---|---|
| plan's control: report the end-of-window eye (`r.glasses = m->io.glasses` in `src/frame.c`; `git diff --stat src` showed one file) | `ctest --test-dir build/ci -R '^fixtures\.glasses\.eyes\.export$'` | 0 | passes: every write in this program is early in its frame, so an end-of-window snapshot reports the same eye; the late side of the rule is pinned by `ports.glasses.*` |
| early writes no longer count for their frame (the `glasses_frame = eye` line in `src/io.c` replaced; `git diff --stat src` showed one file) | same command | 8 | fails: `eyes: none=3, expected 2` |
| both reverted with `git checkout -- src/frame.c` and `src/io.c` | `ctest --test-dir build/ci -R '^fixtures\.glasses\.'` | 0 | 5 of 5 pass |

## Hosted dispatch 2

Commands, from the repository root, no `cd`:

1. `git push origin phase/08-sports-pad-paddle-and-3-d-glasses` exit 0, then `gh workflow run ci.yml --ref phase/08-sports-pad-paddle-and-3-d-glasses` exit 0. The run was found by head SHA with `jq` over `gh run list --workflow ci.yml --branch ... --event workflow_dispatch --json databaseId,headSha,url`.
2. Attempt 1, head `c1b1e3a3169337af112aee63980b8f566e7e16b4`: https://github.com/szTheory/cogmaster/actions/runs/42972569805, `gh run watch 42972569805 --exit-status` exit 1. Jobs that failed, each first seen on this run:
   - be32, both nofp legs, linux-x64 and linux-arm64 builds: `src/state.c:710` `invalid use of pointers to arrays with different qualifiers in ISO C before C23 [-Werror=pedantic]`: `ports_io` took `const uint32_t device[2][2]` and was called with the plain array. Apple clang accepts it, GCC does not. Fix `26a54c8`: the parameter loses its `const` (the function only reads it, the call site is unchanged).
   - windows-x64 and windows-arm64 builds: `runner.device.names`, `runner.device.auto`, `runner.input.axes_equivalent` ("expectation file error (status 2)") and `runner.input.axes_grammar` (the CRLF case). `file(WRITE)` writes each LF as CRLF on Windows, so the expectation reader refused the `.expect` files and the "CRLF" case became `\r\r\n`. Fix `c93de45`: a `write_bytes` helper in `tests/runner/tests.cmake` writes through `file(CONFIGURE ... NEWLINE_STYLE UNIX|CRLF @ONLY)` in the new device and axes cases; the CRLF case writes its first record with CRLF and appends the last record with `file(APPEND)` so the missing final newline is still tested.
   - Not reached on attempt 1 and found beforehand by a GCC 13 syntax check in a container over `src`, `runner`, `host`, `tools`, `tests`: `tests/fuzz/fuzz_pad.c:181` `comparison is always true due to limited range of data type [-Werror=type-limits]` (`val` is an `int8_t`, so `<= 127` cannot fail). Fix `60c97a6`: only the low bound is tested.
3. Attempt 2, head `60c97a6a21324d74b32cb6e0bf862866e9a07c2e`: https://github.com/szTheory/cogmaster/actions/runs/42973268415, `gh run watch 42973268415 --exit-status` was moved to the background and the run was awaited with `gh run view 42973268415 --json status`; its conclusion is `success`. It did not need a third attempt.
4. Main control: `git fetch origin main` then `git rev-parse origin/main` prints `bb7d15874cb357a366521e72046ad279441decb1`, the head of dispatch 1's main run, so B is reused from https://github.com/szTheory/cogmaster/actions/runs/42963279510 (success, head `bb7d15874cb357a366521e72046ad279441decb1`).

Control lines, verbatim:

```
perf control boot_to_play runs=12303147,12303147,12303147 spread=0
perf control boot_to_play runs=12302854,12302854,12302854 spread=0
```

The first is attempt 2's branch run (`gh run view 42973268415 --log`), the second the reused main run. Both spreads are 0, so H is 12303147 and B is 12302854.

```
perf dispatch boot_to_play H=12303147 B=12302854 delta_bp=0
```

`delta_bp=$(( (H - B) * 10000 / B ))` is 0 (the exact ratio is 2.3 hundredths of a percent). `git diff --quiet origin/main -- tests/perf/allow.txt` exits 0, so no allow line.

Per-job conclusions of 42973268415 (`gh run view --json jobs`): hygiene, changes, perf, asan, be32, wasm32, nofp (ubuntu-26.04), nofp (ubuntu-26.04-arm), build on linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64 and windows-arm64, retroarch, hash-equality ("Hashes are byte-identical") and CI required all `success`; title, rehearsal and cleanup `skipped` as for any dispatch. In the run's log each of asan, be32, wasm32, build (windows-x64), build (windows-arm64), build (linux-x64) and build (macos-arm64) lists as Passed: `ports.devices.*` (11), `ports.state.*` (5), `fuzz.regress.pad`, `cart.device.*` (4), `runner.device.*` (4), `runner.input.axes_*` (3), `fixtures.sports_pad.*` (6), `fixtures.paddle.*` (7) and `fixtures.glasses.*` (5), none Failed. `git diff --quiet 60c97a6a21324d74b32cb6e0bf862866e9a07c2e HEAD -- . ':(exclude).planning'` exits 0.

## Verification run

- Task 1: `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(fixtures\.|policy\.)'` exit 0 (82 tests, at least 69); `test -s build/ci/fixtures/glasses.sms` exit 0; one `glasses` line in `tests/built/expected.txt`; 2 `glasses,` rows in the CSV; `wc -l < tests/roms/src/glasses.asm` prints 120; no removed line against origin/main in the three data files; `cmake --workflow --preset hygiene` exit 0.
- Task 2: `ctest --test-dir build/ci -R '^(fixtures\.glasses\.|runner\.hashes_file)'` exit 0 (6 of 6); five `fixtures.glasses.*` names registered and five lines in each inventory; `grep -c 'Fault prevented: a 3-D eye reported in the wrong frame' tests/fixtures/tests.cmake` prints 1; `grep -c '^playing glasses.sms sms ntsc-j device frame ' build/ci/hashes.txt` prints 1.
- Task 3: the SUMMARY section above; the acceptance checks `jq -e` over the run's jobs exit 0 for hash-equality, for wasm32 or be32 or asan or nofp or perf (at least five, all success) and for the build legs; `gh run view 42973268415 --json conclusion --jq .conclusion` prints `success`.
- Plan: `cmake --workflow --preset ci` exit 0 (525 tests plus 4 release tests, before the three fix commits; the fix commits were run against `ctest -R '^(runner\.|fuzz\.|core\.state|ports\.|policy\.)'` and the hosted run); `cmake --workflow --preset hygiene` exit 0 (48 tests); `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0; `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0 (723 check lines, 8793 src lines, unchanged); `clang-format --dry-run -Werror` exit 0 on `src/state.c` and `tests/fuzz/fuzz_pad.c`.

## Task Commits

1. **Task 1: The glasses program, its build entry and its rows** - `2e7145d` (feat)
2. **Task 2: Eye-count cases on three profiles and the control** - `c1b1e3a` (test)
3. **Task 3: Hosted dispatch 2** - evidence above; it needed three fix commits: `26a54c8` (fix, state.c), `c93de45` (test, runner tests) and `60c97a6` (fix, fuzz_pad.c)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] `ports_io` parameter qualifier rejected by GCC**
- **Found during:** Task 3 (hosted dispatch 2, attempt 1)
- **Issue:** `const uint32_t device[2][2]` receiving a `uint32_t [2][2]` is invalid before C23 under `-Wpedantic`; be32, both nofp legs and the Linux builds failed to compile `src/state.c`.
- **Fix:** removed the `const`; reproduced the error and its absence with GCC 13 in a container before and after.
- **Files modified:** src/state.c
- **Commit:** 26a54c8

**2. [Rule 1 - Bug] Windows wrote CRLF into the runner tests' exact-byte files**
- **Found during:** Task 3 (hosted dispatch 2, attempt 1)
- **Issue:** `file(WRITE)` converts LF to CRLF on Windows; four of the plan 08-08 and 08-09 runner cases failed on both Windows legs, and the CRLF case of the axes grammar did not test what it says there.
- **Fix:** `write_bytes` helper over `file(CONFIGURE ... NEWLINE_STYLE)`, used by the new device, axes and eyes cases; the final-newline-missing case is appended with `file(APPEND)`.
- **Files modified:** tests/runner/tests.cmake (not in the plan's file list)
- **Commit:** c93de45

**3. [Rule 1 - Bug] A comparison GCC proves always true in the pad fuzz target**
- **Found during:** Task 3 (found by a container GCC check while diagnosing attempt 1; it would have failed next)
- **Issue:** `val` is an `int8_t`, so `val <= 127` is rejected by `-Wtype-limits` under `-Werror`.
- **Fix:** test only `val >= -127`, with a comment.
- **Files modified:** tests/fuzz/fuzz_pad.c
- **Commit:** 60c97a6

### Plan wording not followed literally

- **The cross-lane row has a script.** The plan writes `glasses.sms|-|-`; the row list takes a script name whose line count is the frame count (plans 08-10 and 08-11 noted this), so the entry is `glasses.sms|glasses|` with a neutral 30-line script `tests/glasses.input` written by `device_script`, and 30 frames instead of 14.
- **A 30-frame run, not 14.** The table is 24 frames and the mailbox work adds a few, so the counts are taken over 30 and the held eye of the tail is counted.
- **The plan's negative control passes.** An end-of-window snapshot reports the same eye as the rule when every write is early, which is how the plan has the program write. The control that fails is the loss of the early-write path; the end-of-window case is pinned by `ports.glasses.control`, which the plan 08-04 summary records failing against exactly that mutation.
- **The mailbox is judged in a second run.** The plan has the program record one record and the CSV hold the row; two records (the routine length and the mirror byte it left) are judged by the existing mailbox judge inside each eyes case.

**Total deviations:** 3 auto-fixed (3 bugs), 4 wording notes. **Impact:** three small fixes outside the plan's file list, all needed for the lanes to run; no budget moved.

## Phase notes

- `glasses.asm` is final here. Any later edit changes its digest in `tests/built/expected.txt` and re-runs the cross-lane rows. Plans 08-10, 08-11 and 08-12 were the last `.asm` edits, as planned.
- Hosted timing for the closeout: the slowest new test on any lane is `ports.state.mutation` at 38.44 s on the be32 lane against its `TIMEOUT 60` (the rule of twice the slowest would give 80); the new fixture and runner tests are all under 1 s. The provisional timeout comments of this phase are for a later plan to settle; the 60 s limit has not been exceeded.
- The three device rows of `hashes.txt` (sports_pad, paddle, glasses) agree on all lanes (hash-equality success), which is the first time the lanes compared them.
- The GCC check done in a container (`gcc` 13 with the project's warning set and `-Werror`, `-fsyntax-only` over `src`, `runner`, `host`, `tools` and the tests) found the third error before a hosted run did; it covers none of be32's 32-bit-only warnings, which the hosted lane covers.

## Known Stubs

None.

## Threat Flags

None. The plan adds a self-authored test image and test data; the only source change in `src/` removes a qualifier.

## Issues Encountered

Two hosted attempts: the first failed on GCC and Windows lanes as above and was fixed on the branch; the second is green.

## Next Phase Readiness

Ready for plan 08-13 (the table identity row for the two device fixtures and the Paddle degrade case). Dispatch 3 and the `nightly.yml` dispatch follow on the final head. The branch is pushed through `60c97a6`; no pull request has been opened.

## Self-Check: PASSED

- `tests/roms/src/glasses.asm`, `tests/roms/src/glasses.link` exist; commits `2e7145d`, `c1b1e3a`, `26a54c8`, `c93de45` and `60c97a6` are ancestors of HEAD.
- Run 42973268415 answers `success` through `gh run view`, and its head SHA has no non-`.planning` difference from HEAD; run 42963279510 (the main control) answers `success` with the head of `origin/main`.
