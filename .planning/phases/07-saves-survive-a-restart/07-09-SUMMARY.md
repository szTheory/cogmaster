---
phase: 07-saves-survive-a-restart
plan: "09"
subsystem: runner
tags:
  - saves
  - runner
  - durability
  - sweep
  - directory-sync
status: complete

requires:
  - phase: 07-saves-survive-a-restart
    provides: plan 07-08 periodic flush (--save-interval) and the flush path the new tests exercise
provides:
  - at load the runner deletes exactly <target>.tmp-0 to .tmp-1023 of its own save, with no directory listing, and a temporary created after load is never overwritten
  - one temporary name pattern, <target>.tmp-<i>, on every platform (the Windows process id is gone)
  - on POSIX other than WASI, a successful rename is followed by open, fsync and close of the directory; EINVAL and ENOTSUP are tolerated and any other error fails the flush without acknowledging the generation
  - runner.save.process.dirsync, runner.save.sweep, runner.save.next_index and runner.save.kill are registered under the same names on every lane
affects: [07-10]

actuals:
  tokens: 4200
  tasks: 3
  commits: 3

plan_head_before: 3fb975b6406a72ad619a6323a0a9828d91df5102
plan_head_after: 56857589687f11f744aac14febecd1fae2335047
commits: 3

tech-stack:
  added: []
  patterns:
    - "A fault test registered on every lane picks its expected exit at configure time (code 0 and the normal frame line where no directory sync exists, code 1 and the error where one does) instead of being registered per platform"
    - "A killed process loses unflushed standard output, so a kill test locates the kill with a file the run writes inside its frame loop (--png) rather than with a frame line"

key-files:
  created: []
  modified:
    - runner/save.c
    - runner/save.h
    - tests/runner/test_save.c
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/runner.md

key-decisions:
  - "The same test names are registered on every lane; only the asserted behaviour differs per platform (supersedes the platform-gated registration wording in CONTEXT)"
  - "The directory-sync fault point sits inside the POSIX-only sync function, so on Windows and WASI the name is accepted and the point is never reached"
  - "The sweep is a bounded unlink loop over the 1024 names of the save's own target, ignoring failures, so a temporary held open on Windows stays"
  - "A failed directory sync leaves the renamed bytes in place and the generation unacknowledged; the next flush writes them again"

requirements-completed:
  - SAVE-05

coverage:
  - id: R1
    description: "A load deletes exactly the .tmp-0 to .tmp-1023 temporaries of its own target and leaves .tmp-1024, a non-numeric temporary and another save's temporary"
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.sweep$'"
        status: pass
    human_judgment: false
  - id: R2
    description: "A temporary that appears after load keeps its bytes and the flush takes the next index"
    requirement: SAVE-05
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.next_index$'"
        status: pass
    human_judgment: false
  - id: R3
    description: "Where a directory sync exists the dirsync fault exits 1 with the rename done and the generation unacknowledged; where none exists the same test name completes the save normally"
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.(process\\.dirsync|failures)$'"
        status: pass
    human_judgment: false
  - id: R4
    description: "A process ended at the first periodic flush, temporary complete and target not replaced, leaves the previous save byte-equal; the next production run removes the temporary and continues from that save"
    requirement: SAVE-05
    verification:
      - kind: e2e
        ref: "ctest --test-dir build/ci -R '^runner\\.save\\.kill$'"
        status: pass
    human_judgment: false
  - id: R5
    description: "The durability section of docs/runner.md states the periodic flush, the POSIX directory sync, the macOS and Windows limits, the load-time sweep and the unsupported simultaneous writers"
    requirement: SAVE-05
    verification:
      - kind: other
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
---

# Phase 7 Plan 09: Sweep, directory sync and the kill test Summary

The runner now deletes its own stale `<target>.tmp-0` to `.tmp-1023` at load, names temporaries the same way on every platform, fsyncs the directory after the rename on POSIX, and proves with a test build that a process ended between flushes leaves the last complete save readable.

## Accomplishments

- `runner/save.c`: `save_sweep` unlinks the 1024 names `<target>.tmp-<i>` at load, with no directory listing and failures ignored. The Windows temporary name drops the process id, so one pattern serves both platforms; `<process.h>` and `_getpid` are gone. After a successful rename on POSIX other than WASI, `save_sync_directory` opens the directory part of the save path, calls `fsync` and closes it. EINVAL and ENOTSUP from `fsync` count as success, any other error (or the dirsync fault) makes the flush return the existing error without acknowledging the generation. Comments cite POSIX.1 `fsync` and `rename`, and say that macOS `fsync` does not flush the drive cache with `F_FULLFSYNC` unused, and that Windows has no directory sync (`MoveFileExW` with write-through is kept, `ReplaceFileW` avoided).
- `runner/save.h`: `COGMASTER_SAVE_DIRSYNC` appended to the test fault enum, on every platform.
- `tests/runner/test_save.c`: a `next_index` case; a `remove_file` helper shared with `failures`; the `failures` case now covers the dirsync stage (skipping the interrupt stage, which ends the process), with the platform branch chosen by the same preprocessor condition as `save.c`.
- `tests/runner/tests.cmake`: `next_index` in the unit foreach; the `save_unit` script keeps `other.srm.tmp-0` and asserts `round.trip.srm.tmp-0` swept (except in `excluded`, where the cartridge has no save and nothing is swept); new script cases `save_sweep` and `save_kill`; `dirsync` in the process-fault foreach with a configure-time expectation (`WIN32` or system name `WASI`: exit 0 and the frame line; otherwise exit 1 and `save: cannot`) and a `RENAMED` flag that makes `save_process` require the new bytes. No `if(WIN32)` or `if(NOT WIN32)` was added and `SKIP_RETURN_CODE` stays at 0.
- `tests/inventory/ci.txt`, `wasm32.txt`: `runner.save.next_index`, `runner.save.sweep`, `runner.save.process.dirsync`, `runner.save.kill`, each `portable,runner`, sorted, in both.
- `docs/runner.md`: the durability paragraph rewritten (directory sync, `F_FULLFSYNC`, Windows, sweep, next free index, simultaneous writers unsupported); "not synchronized" is gone and the periodic-save text no longer says deferred, with the SIGUSR1 and SIGBREAK flush still deferred. The file is 28,595 bytes.

## Task Commits

1. **Task 1: sweep, unified temporary names, next_index** - `87abd04` (feat)
2. **Task 2: POSIX directory sync and the dirsync fault test on every lane** - `8415047` (feat)
3. **Task 3: kill test and durability docs** - `5685758` (test)

## Evidence

All run from the repository root on macOS arm64; hosted runs belong to plan 07-10. Exit 0 unless stated.

| Command | Exit |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(runner\.save\.\|policy\.inventory)'` (after Tasks 1 and 2) | 0 |
| `ctest --test-dir build/ci -R '^(runner\.save\.\|policy\.\|docs\.)'` (64 tests) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` (723 check lines, 8270 src lines) | 0 |
| `cmake --workflow --preset hygiene` | 0 |
| `cmake --workflow --preset ci` (461 tests) | 0 |
| `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` | 0 |
| `cc -fsanitize=address,undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -c runner/save.c` (with `COGMASTER_SAVE_TEST`) | 0 |
| `clang-format --dry-run runner/save.c runner/save.h tests/runner/test_save.c` | 0 |

Acceptance greps: `grep -c '_getpid' runner/save.c` prints 0; the inventory rows for `next_index` and `sweep` print 2 in each inventory and the six-name grep prints 6 in each; `grep -c O_DIRECTORY runner/save.c` prints 1; `grep -c COGMASTER_SAVE_DIRSYNC runner/save.h` prints 1; `grep -c SKIP_RETURN_CODE tests/runner/tests.cmake` prints 0; `grep -cE 'if\((NOT )?WIN32\)' tests/runner/tests.cmake` prints 0, equal to the count in `git show origin/main:tests/runner/tests.cmake`; `grep -ciE 'not synchronized' docs/runner.md` prints 0 and `grep -ciE 'F_FULLFSYNC' docs/runner.md` prints 1.

Wasm32 and Windows lanes: `wasmtime` is installed locally but the wasi-sdk with clang-21 that `cmake/toolchains/wasm32-wasi.cmake` needs is not, so `cmake --workflow --preset wasm32` was not run, and no Windows host is available. Both are covered by the registration (the same names on every lane, checked by the configure-time inventory check on the ci lane) and are confirmed by the wasm32 and ci-msvc lanes of plan 07-10's first hosted run.

Negative controls, each reverted afterwards (ctest exit 8 means a test failed):

| Control | Result |
| --- | --- |
| sweep stops at index 0 (`i < 1u`), `ctest -R '^runner\.save\.(sweep\|next_index\|failures)$'` | exit 8: `runner.save.sweep` failed because `tmp-1023` survived; the other two passed; restored, exit 0 |
| `save_temporary` opens with `O_TRUNC` instead of `O_EXCL`, same command | exit 8: `runner.save.next_index` failed; the other two passed; restored, exit 0 |
| every `fsync` error ignored (`fsync(fd) == 0 \|\| 1`), `ctest -R '^runner\.save\.(process\.dirsync\|failures)$'` | exit 0, still passing (see Deviations) |
| the directory-sync result ignored (`(void)save_sync_directory(save)`), same command | exit 8: `runner.save.failures` and `runner.save.process.dirsync` failed; restored, exit 0 |
| `save_sweep` call removed from the load, `ctest -R '^runner\.save\.(kill\|sweep)$'` | exit 8: both failed; restored, exit 0 |
| `runner.save.kill` run with the periodic interval dropped (the kill falls at the end of the run) | exit 8 with "kill at the first periodic flush ... expected 93 before frame 2"; restored, exit 0 |

First-write check for the kill test (Task 3): `build/ci/cogmaster-run --frames N --save-dir <new dir> build/ci/fixtures/battery.sms` exits 0 and writes `battery.srm` for N = 1, 2 and 3, so the guest changes its battery at frame 1, before the interrupt build's stop frame 3. The first periodic flush at interval 1 therefore sees a dirty battery and the kill comes from the periodic flush, not the signal flush. The kill-only target was not needed. By hand: save A by a 60-frame production run, then `build/ci/tests/cogmaster-test-save-interrupt --save-interval 1 --frames 10 --hash --save-dir <dir> battery.sms` exits 93, one `battery.srm.tmp-0` of 32768 bytes remains, and `cmake -E compare_files` of `battery.srm` against the copy of A exits 0.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug in the plan's control] The kill test's frame-line control cannot discriminate**
- **Found during:** Task 3
- **Issue:** The plan has the control (the interrupt build without `--save-interval`) print its frame line before exiting 93, so that "no `frame N sha256` line" in the kill run can fail. The runner prints the hash line and then calls the exit flush, whose `_Exit(93)` skips the final `fflush(stdout)`; with a captured (pipe) standard output the control prints nothing either, so the assertion would pass whether the kill falls at frame 1 or at the end.
- **Fix:** Both runs pass `--png 2:<file>`. The PNG is written inside the frame loop, so it exists in the control (killed at the exit flush, after frame 2) and must not exist in the kill run (killed at the first periodic flush, frame 1). The "no frame line" assertion is kept, as the plan asks, but the PNG is what makes the test discriminate; the negative control with the interval dropped fails on it.
- **Files modified:** `tests/runner/tests.cmake`
- **Commit:** `5685758`

**2. [Rule 1 - Bug in the plan's control] "Ignore every fsync error" does not fail the dirsync test**
- **Found during:** Task 2
- **Issue:** The dirsync fault is injected by `SAVE_FAULT` before any `open` or `fsync`, so ignoring `fsync` errors cannot change the test's result, and the control as worded still passes. A real `fsync` failure cannot be induced portably in a test.
- **Fix:** Recorded the literal control (exit 0, passing) and added the control that does discriminate: ignoring the whole `save_sync_directory` result fails both `runner.save.process.dirsync` and `runner.save.failures`. The `EINVAL` and `ENOTSUP` tolerance and the real `fsync` path are covered only by review and by the sanitizer compile.
- **Files modified:** none
- **Commit:** none

### Notes on how the plan was followed

- CONTEXT's platform-gated registration of the dirsync and O_EXCL tests is superseded, as the plan's objective says: the same names are registered on every lane, and only the asserted behaviour differs per platform. On Windows and WASI `runner.save.process.dirsync` expects exit 0 and `frame 3 sha256`; elsewhere exit 1, `save: cannot` and the new bytes in place.
- The `save_unit` script cannot expect the swept temporary for the `excluded` case, because that case has no battery save and so no sweep; it asserts the temporary survives there, which also shows the sweep runs only for a cartridge with a save.
- The plan's `plan_head_before` ledger file was written after the first task commit, from the known previous HEAD (`3fb975b`), since the ledger step ran after the commit; the measured count is 3 commits.

## Issues Encountered

None. `hygiene.tree` passed in the full hygiene workflow on the first run.

## Known Stubs

None.

## Threat Flags

None. The sweep deletes only names built from the runner's own save path and the fixed suffix `.tmp-<0..1023>`, so it cannot reach another file; the directory handle is opened read-only and closed.

## Next Phase Readiness

Plan 07-10 can run the hosted round trip. Its first hosted run is also the first evidence for the wasm32 lane (no directory sync, so `runner.save.process.dirsync` must complete normally there) and for the ci-msvc lanes (the unified `.tmp-<i>` names and the `CREATE_NEW` path under `runner.save.next_index`).

## Self-Check: PASSED

- `runner/save.c`, `runner/save.h`, `tests/runner/test_save.c`, `tests/runner/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`, `docs/runner.md` all exist and are modified in the task commits.
- Commits `87abd04`, `8415047`, `5685758` are ancestors of HEAD.
