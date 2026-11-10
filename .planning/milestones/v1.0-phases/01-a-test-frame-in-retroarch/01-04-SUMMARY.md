---
phase: 01-a-test-frame-in-retroarch
plan: 04
subsystem: runner
tags:
  - bench
  - perf
  - signals
  - runner
  - docs
requires:
  - "01-01: cogmaster-run, cogmaster_add_test and the inventory check"
  - "01-02: cogmaster__state_save and cogmaster__state_load, the internal visitor the no-allocation test drives"
  - "01-03: --png N:FILE, runner/png.c and the runner_check script pattern in tests/runner/tests.cmake"
provides:
  - "cogmaster-run --bench N (1 to 1073741823): frames, p50_us, p90_us, p99_us, p99.9_us, p99.99_us, p99.999_us, max_us and max_rss_kib lines"
  - "runner/bench.h, bench.c: cogmaster__bench_rank, cogmaster__bench_bytes, cogmaster__bench_us, cogmaster__bench_stats, cogmaster__bench_format, COGMASTER__BENCH_MAX_FRAMES"
  - "runner/clock.h, clock.c: cogmaster__clock_ns (CLOCK_MONOTONIC, QueryPerformanceCounter)"
  - "runner/stop.h, stop.c: cogmaster__stop_install, cogmaster__stop_requested (SIGINT and SIGTERM)"
  - "runner/rss.h, rss.c: cogmaster__max_rss_kib, the only per-OS branches for peak RSS"
  - "runner/exit_status.h: cogmaster__exit_statuses, the table --help prints"
  - "cogmaster-run --help and --version, each given alone"
  - "tests/cmake/perf_compare.cmake with MODE gate|report, for the CI perf job"
  - "tests/cmake/exit_table.cmake and docs/runner.md"
  - "COGMASTER_RUNNER_SOURCES in CMakeLists.txt, the runner's absolute source list"
affects:
  - "CI perf job (calls perf_compare.cmake with MODE report)"
  - "nightly bench trend run (parses the bench lines)"
  - "wasm32 lane (stop.c uses signal(); rss.c prints 0 under WASI)"
  - "README and docs index, which do not yet link docs/runner.md"
actuals:
  tokens: 15700
  tasks: 2
  commits: 3
plan_head_before: dce711cea62300724ca44771dd0e6817fa4ec337
plan_head_after: 8c5b5c28bcd2e1f0a8585e93ea41b433d39ae09c
tech-stack:
  added: []
  patterns:
    - "A second runner build with a compile-time hook (COGMASTER_RUN_RAISE_AT) proves signal handling end to end; the installed runner has no hook"
    - "Script-mode runner cases run a unit binary case first, then the CLI checks, under one test name"
    - "The doc-versus-help exit table is checked in both directions"
key-files:
  created:
    - runner/bench.h
    - runner/bench.c
    - runner/clock.h
    - runner/clock.c
    - runner/stop.h
    - runner/stop.c
    - runner/rss.h
    - runner/rss.c
    - tests/runner/test_runner.c
    - tests/perf/tests.cmake
    - tests/perf/allow.txt
    - tests/cmake/perf_compare.cmake
    - tests/cmake/exit_table.cmake
    - docs/runner.md
  modified:
    - CMakeLists.txt
    - runner/main.c
    - runner/exit_status.h
    - tests/tests.cmake
    - tests/runner/tests.cmake
    - tests/core/test_core.c
    - tests/core/tests.cmake
    - tests/inventory/ci.txt
decisions:
  - "The bench report has exactly the plan's nine lines; the seed's fps_milli line is dropped"
  - "--bench caps N at 1073741823 so the 4-byte-per-frame sample buffer fits a 32-bit size_t; a larger N is a usage error (exit 2)"
  - "The clock reads nanoseconds (cogmaster__clock_ns, as in the seed) and each frame's difference is truncated to microseconds"
  - "The signal test uses a second runner build, cogmaster-test-run-stop, compiled with COGMASTER_RUN_RAISE_AT=3; no hidden option or environment hook exists in cogmaster-run"
  - "rss.c defines _DARWIN_C_SOURCE on macOS because _POSIX_C_SOURCE hides ru_maxrss there"
  - "Exit table rows are 0 ok, 1 failure, 2 usage"
patterns-established:
  - "cogmaster-test-bench <case>: percentile, bytes, format, stop"
  - "perf.gate.<case> tests feed fixed Ir/frame numbers through expect_fail.cmake, label perf"
requirements-completed:
  - FRAME-09
  - FRAME-02
coverage:
  - id: D1
    description: "Nearest-rank ranks and the seven reported values (p50 to p99.999 and max) are exact for 1, 2, 100 descending, 300,000 equal and 300,000 descending samples; microseconds truncate and saturate"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/runner/test_runner.c#runner.bench.percentile"
        status: pass
    human_judgment: false
  - id: D2
    description: "The formatter writes the nine lines in order from fixed samples; --bench 30 prints every line as a whole number; --bench 1 prints seven equal times"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.bench.format"
        status: pass
      - kind: other
        ref: "build/ci/cogmaster-run --bench 300 > build/ci/bench.txt and a grep for each of the nine lines (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The sample buffer size is exact up to the cap and 0 above it; --bench 0, 1073741824 and 4294967295 exit 2"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.bench.bytes"
        status: pass
    human_judgment: false
  - id: D4
    description: "max_rss_kib is in KiB on macOS, where ru_maxrss is bytes; it read 1584 on arm64 macOS for --bench 300"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "grep -Eq '^max_rss_kib [1-9][0-9]*$' build/ci/bench.txt (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "A SIGTERM raised inside frame 3 ends the run after that frame with exit 0, prints frame 3's hash, equal to the pinned SMS card hash, and a bench report of 3 frames; the stop flag is clear until a signal arrives"
    requirement: FRAME-09
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.stop"
        status: pass
      - kind: other
        ref: "a real kill -TERM sent to build/ci/cogmaster-run --frames 4294967295 --hash after 0.5 s (exit 0, one complete hash line)"
        status: pass
    human_judgment: false
  - id: D6
    description: "After load, 600 frames with a frame copy each and an internal state save and load into caller buffers make 0 alloc and 0 free calls"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/core/test_core.c#core.no_alloc_after_load"
        status: pass
    human_judgment: false
  - id: D7
    description: "The Ir/frame script passes identical numbers and +1.00 %, fails +1.01 % in gate mode and only reports it in report mode, honours a current allow line and not an expired one, reports UNSTABLE for unequal base runs, reports a missing base without failing in report mode, and errors on an empty measurement"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/perf/tests.cmake#perf.gate.identical, perf.gate.plus_1_00, perf.gate.plus_1_01, perf.gate.allowed, perf.gate.unstable, perf.gate.no_base, perf.gate.missing"
        status: pass
    human_judgment: false
  - id: D8
    description: "--help prints the exit table 0 ok, 1 failure, 2 usage, and docs/runner.md holds exactly the same rows"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/cmake/exit_table.cmake#runner.exit.table"
        status: pass
    human_judgment: false
  - id: D9
    description: "No arguments, every deferred option (--probe, --trace, --region, --expect, --port1, --port2, --input, --hash-audio, --capabilities, --boot, --bios), --help or --version with other arguments, a file name, and --frames 0, -1, 1x, 4294967296, missing or twice all exit 2; --help mentions none of the deferred options"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.no_args, runner.usage.unknown, runner.usage.positional, runner.usage.frame_range"
        status: pass
    human_judgment: false
  - id: D10
    description: "An unwritable --png path exits 1; --frames 1 --hash and --frames 60 --hash print the same digest with their own frame numbers; --version prints cogmaster-run X.Y.Z"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.exit.failure, runner.hash.repeat, runner.version"
        status: pass
    human_judgment: false
  - id: D11
    description: "docs/runner.md documents options, the hash and PNG definitions, bench lines and units including max_rss_kib 0 under WASI, signals, the exit table and the test card element by element, in under 40 KB"
    requirement: FRAME-02
    verification:
      - kind: other
        ref: "grep -q 0x0F0F docs/runner.md && grep -q 4,096 docs/runner.md (exit 0); wc -c < docs/runner.md printed 8690"
        status: pass
    human_judgment: false
duration: 8min
completed: 2026-10-19
status: complete
---

# Phase 1 Plan 04: Bench, signals, help and the runner guide Summary

**`cogmaster-run --bench N` prints nearest-rank frame-time percentiles in whole microseconds and peak RSS in KiB. SIGINT and SIGTERM stop the run cleanly after the current frame. `--help` and `--version` work, and `docs/runner.md` holds an exit table that a test keeps equal to `--help`. A test proves the core makes no allocator call after load. The Ir/frame comparison script is ready in report mode.**

## Performance

- **Duration:** about 8 min
- **Started:** 2026-10-19T21:30Z
- **Completed:** 2026-10-19T21:38Z
- **Tasks:** 2
- **Files created:** 14, modified: 8

## Accomplishments

- **Bench.** `--bench N` times only `cogmaster_run_frame`, using the host's monotonic clock, and keeps one 4-byte sample per frame on the heap. It prints `frames`, the six percentiles, `max_us` and `max_rss_kib`. Each percentile is the sample at rank ⌈n × p⌉, so every value printed was actually measured.
- **RSS.** `runner/rss.c` is the only file with a branch per operating system. Linux reports `ru_maxrss` in KiB. macOS reports bytes, which the runner divides by 1024, rounding up. Windows uses `K32GetProcessMemoryInfo` and its `PeakWorkingSetSize`. WASI prints 0. On arm64 macOS, `--bench 300` reported `max_rss_kib 1584`, a plausible size. If the bytes-versus-KiB unit were wrong, the value would be about 1024 times larger.
- **Stop.** The frame loop checks the signal flag only between whole frames, after that frame's PNG is written. A run that is stopped prints `frame <frames run> sha256 …` and a bench report for the frames that ran, then exits 0.
- **No allocation after load.** `core.no_alloc_after_load` counts allocator calls across 600 frames with a frame copy each, plus one internal state save and load. Both the allocation count and the free count are 0.
- **Ir/frame gate.** The seed's `perf_compare.cmake` now takes `MODE=gate|report` instead of `ARMED`. Seven synthetic `perf.gate.*` tests cover the +1.00 % and +1.01 % boundary, the allow list, UNSTABLE, a missing base and a missing measurement. None of them needs Valgrind.
- **Surface.** The option set is exactly `--machine`, `--frames`, `--bench`, `--hash`, `--png`, `--help` and `--version`. Every option deferred to a later release exits 2, and `--help` does not mention any of them.
- **Guide.** `docs/runner.md` is 8,690 bytes. It covers the options, the hash and PNG definitions, the bench lines and their units, signals, the exit table and the two test cards rule by rule. It ends with a table of the fault each element exposes.

## Task Commits

1. **Task 1: --bench, max RSS, signal stop, zero allocations after load and the Ir/frame gate script**: `1d4f5e1` (test, RED), `9155509` (feat, GREEN)
2. **Task 2: --help, --version, the exit table and the runner guide**: `8c5b5c2` (feat)

## Evidence (command, exit status)

- `cmake --workflow --preset ci`: exit 0, 47 tests.
- `ctest --test-dir build/ci --no-tests=error -R '^(runner\.bench\.|runner\.stop$|core\.no_alloc_after_load$|perf\.gate\.)' --output-on-failure && ctest … --show-only=json-v1 | jq -e '.tests | length == 12'`: exit 0.
- `build/ci/cogmaster-run --bench 300 > build/ci/bench.txt` followed by a grep for each of the nine lines: exit 0.
- `build/ci/cogmaster-run --bench 0`: exit 2.
- `grep -Eq '^max_rss_kib [1-9][0-9]*$' build/ci/bench.txt`: exit 0 (value 1584 on arm64 macOS).
- `ctest --test-dir build/ci --no-tests=error -L '^runner$' --output-on-failure && ctest … -L '^runner$' --show-only=json-v1 | jq -e '.tests | length == 18'`: exit 0.
- `build/ci/cogmaster-run --help | grep -Eq '^ *2 +usage'`: exit 0. `build/ci/cogmaster-run --help --version`: exit 2.
- `build/ci/cogmaster-run --frames 1 --expect x`: exit 2. `build/ci/cogmaster-run --frames 1 --hash-audio`: exit 2.
- `grep -q '0x0F0F' docs/runner.md && grep -q '4,096' docs/runner.md`: exit 0. `wc -c < docs/runner.md` printed 8690.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `grep -rEn '\b(D-[0-9]{2}|FRAME-[0-9]{2})\b' runner host tests docs`: exit 1, nothing printed.
- `sh scripts/hygiene.sh --tree`: exit 0.
- One-off, real signal: `build/ci/cogmaster-run --frames 4294967295 --hash` was sent `kill -TERM` after 0.5 s. It exited 0 and printed one complete line, `frame 7174 sha256 e14d77ce…`, which is the pinned SMS card hash.
- One-off negative controls for `exit_table.cmake`: with the `2 usage` row deleted from a copy of the doc, the script exited 1. With an extra `3 extra` row added, it also exited 1.
- One-off build outside the tree with `-fsanitize=address,undefined -fno-sanitize-recover=all` (Apple clang, Debug): `ctest` exited 0, 47 of 47 tests passed.

## TDD Gate Compliance

- RED `1d4f5e1` came before GREEN `9155509`. RED was recorded as `ctest --test-dir build/ci -R '^(runner\.bench\.|runner\.stop$)' --output-junit red-junit.xml`, which exited 8 with 4 of 4 tests failing. `gsd-tools check tdd-red-evidence build/ci/red-record.json` returned `RED_EVIDENCE_OK` (target_test_failed, target `runner.bench.percentile`). The classifier ran on the unchanged RED JUnit report after the GREEN commit, not before it. The report was written in the RED state and was not regenerated.
- Semantic assessment: in RED, the bench, clock, stop and RSS bodies were stubs that returned 0, so everything compiled. `runner.bench.percentile` and `runner.bench.bytes` failed on their rank, statistics and byte assertions. `runner.bench.format` failed on the formatter's text. `runner.stop` failed because the flag stayed 0 after `raise(SIGINT)`. No test failed because of a build or fixture error.
- Two kinds of test already passed in RED, by design. The `perf.gate.*` tests passed because the script was copied complete from the seed and its tests are fixed-number checks. `core.no_alloc_after_load` passed because it proves a property the core already has, so it guards against a regression rather than driving new code.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] macOS hides `ru_maxrss` under `_POSIX_C_SOURCE`**
- **Found during:** Task 1 GREEN
- **Issue:** The runner is built with `_POSIX_C_SOURCE=200809L`, and under that macro Apple's `struct rusage` has no `ru_maxrss` member. The build failed.
- **Fix:** `rss.c` defines `_DARWIN_C_SOURCE` on `__APPLE__` before any system header. glibc and musl always declare the field.
- **Files modified:** runner/rss.c
- **Commit:** `9155509`

### Interpretations and additions

- **Bench lines.** The output matches the plan's list exactly. The seed's `fps_milli` line is dropped, and percentile keys use dots (`p99.9_us`), not the seed's underscores.
- **Clock name.** The plan's artifact list names `cogmaster__clock_us`. The code keeps the seed's `cogmaster__clock_ns`, and the frame-time difference is truncated to microseconds. This avoids rounding each reading separately.
- **Sample cap.** The cap is `COGMASTER__BENCH_MAX_FRAMES` = 1073741823. Above it, `cogmaster__bench_bytes` returns 0 on every platform and the runner exits 2. The seed instead exited 1, and only on 32-bit hosts.
- **Signal test hook.** The plan says "test hook" without saying how. It is a second build of the runner sources, `cogmaster-test-run-stop`, compiled with `COGMASTER_RUN_RAISE_AT=3`. The installed `cogmaster-run` has no hidden option or environment hook. `COGMASTER_RUNNER_SOURCES` in `CMakeLists.txt` keeps the two builds in step.
- **Test layout.** The plan places `runner.bench.format`, `runner.bench.bytes` and `runner.stop` in `test_runner.c`. Each of these is a script case that runs one `cogmaster-test-bench` unit case and then the CLI checks. `runner.bench.percentile` runs the unit binary directly. The unit binary is named `cogmaster-test-bench` because `cogmaster-test-runner` is already the PNG test binary from plan 03.
- **Exit table check.** `exit_table.cmake` also fails when the doc has an exit-status row that `--help` does not print. The seed checked only the other direction.
- **Exit table names.** The rows are `ok`, `failure` and `usage`, as the plan says, not the seed's `done`.
- **More usage cases.** `runner.usage.frame_range` also covers `--frames 4294967296`, a missing value, `--frames` given twice, and `--frames` together with `--bench`. `runner.usage.unknown` checks each deferred option with and without a value.

## Requirement status

`requirements-completed` lists FRAME-09 and FRAME-02 because this plan delivers its share of each. Neither box in `.planning/REQUIREMENTS.md` is ticked, because later plans in this phase still carry both: the CI `perf` job that calls `perf_compare.cmake`, `abi.undefined` for the no-clock proof, and the cross-platform runs.

## Known Stubs

None.

## Issues Encountered

- **wasm32 signals, for the wasm32 plan.** `stop.c` uses ISO C `signal()`. wasi-libc may need its emulated-signal option for `signal()` and `raise()`. The lane that first builds the runner for wasm32 confirms this. The seed's identical `stop.c` shipped on its wasm32 lane.

## Next Phase Readiness

The CI `perf` job can call `cmake -DROM=testcard -DHEAD=… -DBASE1=… -DBASE2=… -DALLOW=tests/perf/allow.txt -DMODE=report -DTODAY=… -P tests/cmake/perf_compare.cmake`. The nightly trend run can parse the nine `--bench` lines. Every runner option and exit status is documented in `docs/runner.md`, but no README or docs index links to it yet.

## Self-Check: PASSED

- All 14 created files exist on disk.
- Commits 1d4f5e1, 9155509 and 8c5b5c2 are ancestors of HEAD.
