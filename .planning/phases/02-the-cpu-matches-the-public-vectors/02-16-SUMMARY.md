---
phase: 02-the-cpu-matches-the-public-vectors
plan: 16
subsystem: fuzz
status: complete
tags:
  - fuzz
  - libfuzzer
  - replay
  - nightly
requires:
  - "02-11: cogmaster-test-seeds writes build/<preset>/tests/seeds; the oversize file sits apart in tests/oversize"
  - "02-09: cogmaster_probe; 02-06: cogmaster_load with fast boot"
provides:
  - "tests/fuzz/fuzz_loader.c and fuzz_run.c: one LLVMFuzzerTestOneInput each (fuzz.h declares it)"
  - "tests/fuzz/replay.c: a plain C main replaying files and directories through either entry, printing 'replayed N'"
  - "fuzz.regress.loader and fuzz.regress.run (labels fuzz, portable) on the ci, ci-msvc, asan, be32 and wasm32 lanes"
  - "Option COGMASTER_LIBFUZZER, inventory tests/inventory/fuzz.txt, preset fuzz (configure, build, test, workflow)"
  - "fuzz.libfuzzer.loader and fuzz.libfuzzer.run: 60 s each, -timeout=2 -max_len=1049089, over a fresh copy of the seeds"
  - "Nightly job fuzz on ubuntu-26.04, continue-on-error until ten green runs, in the report job's needs"
  - "docs.budgets keeps tests/fuzz at most 524,288 bytes"
affects:
  - "Plan 17 measures the replay tests' timeouts and documents the fuzz lane for maintainers"
  - "A finding's minimised input goes to tests/fuzz/regress with a tests/roms/manifest.txt row"
actuals:
  tokens: 7770
  tasks: 2
  commits: 3
plan_head_before: 1bc70d3358bebd59964d6659409edb177d6a5e53
plan_head_after: a3d32d969c6ef8ce4e7be1af94702be86132fc0f
tech-stack:
  added: []
  patterns:
    - "One entry per fuzz target, linked either with libFuzzer (nightly) or with a plain replay main (every pull request)"
    - "tests/fuzz/tests.cmake doubles as the -P driver of one timed libFuzzer run, as tests/runner/tests.cmake does for runner checks"
    - "The fuzz preset compiles everything with -fsanitize=fuzzer-no-link and links -fsanitize=fuzzer on the two targets only"
key-files:
  created:
    - tests/fuzz/fuzz.h
    - tests/fuzz/fuzz_loader.c
    - tests/fuzz/fuzz_run.c
    - tests/fuzz/replay.c
    - tests/inventory/fuzz.txt
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - tests/tests.cmake
    - tests/fuzz/tests.cmake
    - tests/cmake/budgets.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - .github/workflows/nightly.yml
    - docs/testing.md
    - .planning/preparation/ENGINEERING.md
decisions:
  - "The loader entry loads on the probe's machine and region guess, as cogmaster-run --machine auto does, and aborts when the probe and the load disagree on accepting the image"
  - "The replay copies each input into a buffer of exactly its length, so the asan lane sees any read past it, and treats an input longer than -max_len as an error"
  - "libFuzzer grows a fresh copy of the seeds in build/<preset>/tests/corpus-<target>; crash inputs go to build/<preset>/tests/fuzz-artifacts, uploaded on failure"
  - "The fuzz preset builds no libretro core and no fixtures; Debug -O1 like asan"
  - "A change under tests/fuzz runs the nightly workflow on its pull request, as a bench change does"
requirements-completed:
  - CPU-05
coverage:
  - id: D1
    description: "fuzz.regress.loader and fuzz.regress.run each replay every generated seed (31) and pass; the fuzz label selects exactly these two tests"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -L '^fuzz$' --output-on-failure -V | grep -Eq 'replayed [1-9][0-9]*' && ctest --test-dir build/ci -L '^fuzz$' --show-only=json-v1 | jq -e '.tests | length == 2'"
        status: pass
    human_judgment: false
  - id: D2
    description: "The replay tests pass on every pull-request leg, including Windows MSVC, be32 under qemu-ppc and wasm32 under wasmtime"
    requirement: CPU-05
    verification:
      - kind: e2e
        ref: "gh run view 39376066109 (ci.yml on the phase branch, conclusion success)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The nightly fuzz job builds the fuzz preset with clang-21 and ASan and UBSan, runs each libFuzzer target 60 s, and the JUnit check finds both runs"
    requirement: CPU-05
    verification:
      - kind: e2e
        ref: "gh run view 39375850717 (nightly.yml on the phase branch, job fuzz success, 77 s)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The ci inventory still matches what ran once each, and docs.budgets holds tests/fuzz at most 512 KiB"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^docs\\.budgets$'"
        status: pass
    human_judgment: false
  - id: D5
    description: "The fuzz preset sets COGMASTER_LIBFUZZER, nightly runs the fuzz workflow, tests/fuzz/tests.cmake passes -max_len=1049089, and ENGINEERING section 3 holds 1049089 and not the old value"
    requirement: CPU-05
    verification:
      - kind: other
        ref: "jq -e '.configurePresets[] | select(.name==\"fuzz\") | .cacheVariables.COGMASTER_LIBFUZZER == \"ON\"' CMakePresets.json && grep -Eq 'preset fuzz' .github/workflows/nightly.yml && grep -Eq 'max_len=1049089' tests/fuzz/tests.cmake"
        status: pass
    human_judgment: false
  - id: D6
    description: "The hygiene lane accepts the edited workflow, formatting and timeout comments"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
metrics:
  duration: 23 min
  completed: 2026-10-20
---

# Phase 2 Plan 16: Fuzz the loader and a short run Summary

The loader and a two-frame run each have one `LLVMFuzzerTestOneInput`. A plain C replay runs every generated seed through both entries on every pull-request lane. The `fuzz` preset runs them under libFuzzer, ASan and UBSan nightly, with `-max_len=1049089`.

## What was built

- **Fuzz entries.** `fuzz_loader.c` probes the bytes, then loads them on the profile the probe guesses. It aborts if the probe and the load disagree on accepting the image. `fuzz_run.c` loads with fast boot and runs two frames. Both use a malloc-backed allocator and destroy the instance afterwards.
- **Replay.** `replay.c` takes files or directories. It lists directories with `dirent` on POSIX and WASI and with `FindFirstFileA` on Windows. It prints `replayed N` and exits 1 when N is 0 or an input cannot be read. `fuzz.regress.*` read `build/<preset>/tests/seeds` and, when it exists, `tests/fuzz/regress`. They never read the oversize directory.
- **libFuzzer lane.** `COGMASTER_LIBFUZZER` selects inventory `fuzz`, and `tests/tests.cmake` returns after the fuzz file. In script mode, `tests/fuzz/tests.cmake` copies the seeds into a fresh corpus. It then runs the target with `-max_total_time=60 -timeout=2 -max_len=1049089 -print_final_stats=1`, writing artifacts into the build tree.
- **Nightly.** A `fuzz` job runs on `ubuntu-26.04` with `continue-on-error: true` until ten green runs. It installs `libclang-rt-21-dev` as the asan job does, then runs the JUnit check against `fuzz.txt` and uploads the artifacts on failure. `report` now needs `fuzz`.
- **Budget and docs.** `docs.budgets` sums `tests/fuzz` (currently 22 KB) against 524,288 bytes. `docs/testing.md` gains a lanes row, a label row and a Fuzzing section covering replay, nightly runs and adding a regression. ENGINEERING section 3 now reads `-max_len=1049089`, with the reason.

## Evidence

- `cmake --workflow --preset ci`: exit 0. `cmake --workflow --preset hygiene`: exit 0.
- CI on the phase branch: https://github.com/szTheory/cogmaster/actions/runs/39376066109. Conclusion: success. `fuzz.regress.*` passed on linux, macOS, windows-x64, be32 and wasm32.
- Nightly on the phase branch: https://github.com/szTheory/cogmaster/actions/runs/39375850717. Conclusion: success. `fuzz.libfuzzer.loader` took 61.1 s and `fuzz.libfuzzer.run` 61.1 s, and `check_junit: 2 tests ran once each`.
- A local run with Homebrew clang-21 and `-fsanitize=fuzzer-no-link` only: the loader ran 409,823 executions and the run target 73,704 in 60 s each, with no finding. Locally, ASan hangs at start-up on this macOS (in `__asan::InitializeShadowMemory`). That is a local toolchain problem, so the sanitizer lane is proven only by the hosted run above.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] MSVC C4701 in replay.c**
- **Found during:** CI dispatch after Task 2 (run 39375857405, both Windows legs)
- **Issue:** `WIN32_FIND_DATAA fd` was passed to `FindFirstFileA` through a conditional, so MSVC flagged a possibly uninitialised read. `/WX` turned it into an error.
- **Fix:** `memset` the find data first.
- **Files modified:** tests/fuzz/replay.c
- **Commit:** a3d32d9

**2. [Rule 2 - Missing] Added `tests/fuzz/fuzz.h`**
- A shared prototype for `LLVMFuzzerTestOneInput`. Without it, `-Wmissing-prototypes` fails the build under warnings-as-errors. The file is not in the plan's file list.

**3. [Rule 2 - Missing] Artifact upload and pull-request trigger**
- On failure, the nightly job uploads `build/fuzz/tests/fuzz-artifacts`, so a finding's input can be minimised.
- `tests/fuzz/**` joins the nightly workflow's `pull_request` paths, so a fuzz change gets a libFuzzer run before it merges.

## Known Stubs

None.

## Threat Flags

None. The new code is test-only and adds no network, auth or file surface to the library, runner or core.

## Self-Check: PASSED
