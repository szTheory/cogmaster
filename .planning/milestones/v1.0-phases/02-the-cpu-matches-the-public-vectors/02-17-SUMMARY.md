---
phase: 02-the-cpu-matches-the-public-vectors
plan: 17
subsystem: vectors
status: complete
tags:
  - vectors
  - nightly
  - timeouts
  - docs
requires:
  - "02-01: cogmaster-sst2bin, cogmaster-test-sst and the committed sample"
  - "02-05: the seven prefix-group shards and the allow list"
  - "02-16: the nightly fuzz job and the report job's needs list"
provides:
  - "Option COGMASTER_VECTORS_FULL, inventory tests/inventory/vectors-full.txt, preset vectors-full (configure, build, test, workflow)"
  - "vectors.full.base/cb/dd/ddcb/ed/fd/fdcb over all 1,604,000 vectors, and vectors.sample_equal against the committed sample"
  - "Nightly job vectors on ubuntu-26.04 in the report job's needs"
  - "Measured TIMEOUT values and run citations for every test the phase added; hygiene job 2 minutes"
  - "docs/conformance.md label table, nightly full run and fuzzing sections; docs/testing.md vectors-full lane and six new labels; docs/maintaining.md sst-z80 and wla-dx bump policy"
affects:
  - "Phase 3 brings the first representative perf subject (boot_to_play); until then perf benches the no-content path"
actuals:
  tokens: 11300
  tasks: 2
  commits: 2
plan_head_before: 723d871c9c5dbb3f36ab9c35e18a3c37498c3034
plan_head_after: de88bb9e9fac3470241af3eb59d61f070db81132
tech-stack:
  added: []
  patterns:
    - "A lane option that selects its own inventory and returns early from tests/tests.cmake, as hygiene, nofp and fuzz do"
    - "A test TIMEOUT carries a one-line comment: the 10 s floor or the value, 2 x the slowest Linux x64 time, the sample count, the run ID, provisional"
key-files:
  created:
    - tests/inventory/vectors-full.txt
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - tests/tests.cmake
    - tests/z80/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/bus/tests.cmake
    - tests/mapper/tests.cmake
    - tests/io/tests.cmake
    - tests/cart/tests.cmake
    - tests/core/tests.cmake
    - tests/runner/tests.cmake
    - tests/fuzz/tests.cmake
    - tests/abi/tests.cmake
    - .github/workflows/nightly.yml
    - .github/workflows/ci.yml
    - docs/conformance.md
    - docs/testing.md
    - docs/maintaining.md
    - .planning/preparation/ARCHITECTURE.md
decisions:
  - "vectors-full builds no libretro core and no fixtures; Release with warnings as errors"
  - "The full and regenerated-sample files are build outputs of cogmaster-sst2bin under build/vectors-full, rebuilt only when the converter changes"
  - "Every phase test's Linux x64 time is under 0.3 s, so each takes the 10 s floor; the libFuzzer runs take 123 s (2 x 61.12 s)"
  - "The be32 and wasm32 scales stay 8 and 5: the slowest phase test under qemu-ppc (cart.probe.pure, 2.42 s) uses 3 % of its 80 s budget, and under wasmtime (0.91 s) 2 % of 50 s"
  - "A change under tests/z80 or tests/vectors runs the nightly workflow on its pull request"
requirements-completed:
  - CPU-01
  - CPU-02
  - CPU-03
  - CPU-04
  - CPU-05
coverage:
  - id: D1
    description: "After fetch-pin, the vectors-full workflow converts every vector, runs seven full shards each asserting its executed count, and requires the regenerated sample to equal the committed one"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "scripts/fetch-pin.sh sst-z80 build/pins/sst-z80 && cmake --workflow --preset vectors-full (exit 0); ctest --test-dir build/vectors-full --show-only=json-v1 | jq -e '.tests | length == 8' (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Configure of vectors-full fails naming the fetch-pin command when build/pins/sst-z80/v1 is missing"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "configure with COGMASTER_VECTORS_FULL=ON and the fetched tree moved aside; output matches 'scripts/fetch-pin.sh sst-z80 build/pins/sst-z80'"
        status: pass
    human_judgment: false
  - id: D3
    description: "The nightly vectors job fetches the pin, runs the workflow, finds all eight tests in the JUnit file, and joins the report job's needs"
    requirement: CPU-01
    verification:
      - kind: e2e
        ref: "gh run view 39377102899 (nightly.yml on the phase branch, conclusion success, job vectors success); first run 39376646150 also success"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every phase test's TIMEOUT comes from hosted per-test times with its run cited, and each fits its be32 and wasm32 budget"
    requirement: CPU-02
    verification:
      - kind: e2e
        ref: "gh run view 39377100295 (ci.yml on the phase branch after the timeout change, conclusion success on every leg)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The mapper, ports, fixture and fuzz-replay tests keep passing on all six build legs, be32 and wasm32 under the new timeouts"
    requirement: CPU-03
    verification:
      - kind: e2e
        ref: "gh run view 39377100295 (conclusion success)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ports fixture and the io and vdp tests run within measured timeouts on every leg"
    requirement: CPU-04
    verification:
      - kind: e2e
        ref: "gh run view 39377100295 (conclusion success)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The nightly fuzz job passes with its libFuzzer runs under the measured 123 s timeout, and the replay tests pass under 10 s on every lane"
    requirement: CPU-05
    verification:
      - kind: e2e
        ref: "gh run view 39377102899 (job fuzz success) and gh run view 39377100295 (conclusion success)"
        status: pass
    human_judgment: false
  - id: D8
    description: "The local ci workflow, the JUnit inventory check, the docs budget and name checks, the hygiene lane and the planning-ID scan all pass"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci; cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake; ctest --test-dir build/ci --no-tests=error -R '^(docs\\.budgets|docs\\.api_names)$'; cmake --workflow --preset hygiene (all exit 0)"
        status: pass
    human_judgment: false
metrics:
  duration: 14 min
  completed: 2026-10-20
---

# Phase 2 Plan 17: Nightly full vectors, measured timeouts and maintainer docs Summary

The new `vectors-full` preset runs all 1,604,000 SingleStepTests Z80 vectors from the pinned archive. It also proves the committed sample is what the converter writes from that archive. The preset runs nightly, and it takes about 4 s locally and 67 s for the whole hosted job. Every test this phase added now has a timeout measured on the hosted runners. The two final dispatched runs, CI and nightly, both concluded `success`.

## What was built

- **Lane.** `COGMASTER_VECTORS_FULL` selects the `vectors-full` inventory. If `build/pins/sst-z80/v1` is missing, configure stops and names `scripts/fetch-pin.sh sst-z80 build/pins/sst-z80`. In `tests/z80/tests.cmake`, the build runs `cogmaster-sst2bin --count 1000` into `build/vectors-full/sst-full.bin` (130 MB) and `--count 100` into `build/vectors-full/sst-sample.bin`. It registers seven `vectors.full.*` shards (`--expect-count` 252,000 / 256,000 / 252,000 / 256,000 / 80,000 / 252,000 / 256,000, with the allow list) and `vectors.sample_equal` (`cmake -E compare_files`), then returns.
- **Preset.** `vectors-full` has configure, build, test and workflow presets. It is Release with warnings as errors, and builds no fixtures and no libretro core.
- **Nightly.** The `vectors` job runs on `ubuntu-26.04`: fetch-pin, the workflow, then the JUnit check against `vectors-full.txt`. The `report` job now needs it. A change under `tests/z80/` or `tests/vectors/` runs the nightly workflow on its pull request.
- **Timeouts.** Every test the phase added runs in under 0.3 s on Linux x64, so each takes the 10 s floor. Before, many sat at 30 or 60 s. Each registration site has a comment like `# Timeout: 10 s floor; 2 x 0.18 s, slowest of 2 runs, run 39376644025, provisional.` The libFuzzer runs get 123 s, which is 2 x 61.12 s from run 39375850717. The hygiene job goes to 2 minutes (2 x 33 s, run 39376644025). The nightly `fuzz` and `vectors` jobs keep the 5-minute floor and now cite their runs.
- **Docs.** `docs/conformance.md` gets three additions:
  - a table with the command for each label and preset;
  - "The nightly full run";
  - a Fuzzing section.

  The allow-line and CSV-row instructions were already there. `docs/testing.md` gets the `vectors-full` lane row and the fifth inventory, plus label rows for `z80`, `vectors`, `vdp`, `mapper`, `cart` and `io` (now 26,605 B). `docs/maintaining.md` adds the bump policy for the `sst-z80` and `wla-dx` pins. ARCHITECTURE section 12 notes that the perf subject stays the no-content path until phase 3.

## Measurements

Per-test times come from the ctest lines in the job logs of CI runs 39376066109 and 39376644025 (Linux x64, be32, wasm32). The full shards come from nightly run 39376646150.

| Site | Linux x64 slowest | be32 slowest | wasm32 slowest | TIMEOUT |
|---|---|---|---|---|
| `cart.*` | 0.18 s | 2.42 s | 0.91 s | 10 |
| `core.state.*` (incl. `state.mapper`) | 0.26 s | 1.48 s | 0.59 s | 10 |
| `vectors.*` shards | 0.06 s | 0.35 s | 0.11 s | 10 (was 60) |
| `fuzz.regress.*` | 0.11 s | 0.33 s | 0.16 s | 10 (was 60) |
| `vectors.full.*` (nightly) | 0.28 s | - | - | 10 |
| `fuzz.libfuzzer.*` (nightly) | 61.12 s | - | - | 123 (was 120) |
| every other phase site | at most 0.05 s | at most 0.45 s | at most 0.36 s | 10 |

**Scales.** The be32 scale stays 8 and the wasm32 scale stays 5. Every phase test fits its emulated budget with a wide margin: the slowest under qemu-ppc uses 2.42 s of 80 s, and under wasmtime 0.91 s of 50 s. The whole-suite be32/x64 ratio varied between 4.9 and 17.5 across the two runs. Linux x64 timings varied by two times between them, so the ratio is noisy. The libretro host tests are the slowest under be32 (18.69 s against a 60 x 8 = 480 s budget). Re-deriving the scale stays a job for when a test approaches its budget.

## Evidence

- Local: `scripts/fetch-pin.sh sst-z80 build/pins/sst-z80 && cmake --workflow --preset vectors-full`, exit 0 (`/usr/bin/time` real 3.89 s). `jq -e '.tests | length == 8'`, exit 0. `check_junit: 8 tests ran once each`.
- Local: `cmake --workflow --preset ci` exit 0. `check_junit: 180 tests ran once each`. `docs.budgets` and `docs.api_names` exit 0. `cmake --workflow --preset hygiene` exit 0. The planning-ID grep exits 1, so the negated check passes.
- CI before the timeout change: https://github.com/szTheory/cogmaster/actions/runs/39376644025, conclusion success.
- CI after it, the final run: https://github.com/szTheory/cogmaster/actions/runs/39377100295, conclusion `success` on every leg (`gh run view 39377100295 --json conclusion --jq .conclusion` prints `success`).
- Nightly, both runs: https://github.com/szTheory/cogmaster/actions/runs/39376646150 and https://github.com/szTheory/cogmaster/actions/runs/39377102899, conclusion success. Jobs `vectors`, `fuzz`, the benches and `report` all succeeded.
- Task 1 acceptance: `grep -c 'fetch-pin.sh sst-z80' .github/workflows/nightly.yml` prints 1. The inventory grep prints 8. `grep -c 'Fault prevented:' tests/z80/tests.cmake` went from 5 to 7. The configure check's `Fault prevented:` line sits in the `COGMASTER_VECTORS_FULL` block of `CMakeLists.txt`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Hygiene job timeout below twice its run time**
- **Found during:** Task 2 (job durations of runs 39376066109 and 39376644025)
- **Issue:** The hygiene job took 33 s against a 1-minute timeout, and twice that is 66 s.
- **Fix:** `timeout-minutes: 2`, citing run 39376644025.
- **Files modified:** .github/workflows/ci.yml
- **Commit:** de88bb9

**2. [Rule 2 - Missing] Nightly pull-request paths**
- `tests/z80/**` and `tests/vectors/**` were added to `nightly.yml`'s `pull_request` paths, so a change to the converter, harness or sample gets a full-set run before merge. This matches how 02-16 handled `tests/fuzz/**`.

**3. [Rule 1 - Bug] Stale comment**
- The vectors shard comment said "Timeouts are provisional until the emulated lanes are measured". They now are, so the sentence went and each site carries its own measured comment.

No other deviation. `CMakePresets.json` changed only by gaining the `vectors-full` presets; the scales were left alone because every test fits.

## Known Stubs

None.

## Threat Flags

None. The new job downloads only the pinned, digest-checked archive through `fetch-pin.sh`, and runs with the workflow's read-only token.

## Self-Check: PASSED
