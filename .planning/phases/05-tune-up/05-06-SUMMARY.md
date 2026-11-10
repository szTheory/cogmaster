---
phase: 05-tune-up
plan: "06"
subsystem: ci
tags: [quality, ratchet, size, ceilings, ci]
status: complete

requires:
  - phase: 05-tune-up
    provides: quality.json and its checker (plan 05-05), the eight-file hash equality and re-derived timeouts (plan 05-04)
provides:
  - a base-file ratchet in tests/cmake/quality.cmake
  - measured size.zip.library, size.zip.runner, size.zip.libretro and size.wasm ceilings in quality.json, each with a reasons entry naming its source run
  - a Size ceilings step in every build leg and a Size ceiling step in the wasm32 lane, each with a negative control
  - quality.json in the trusted base copy
affects: [05-14]

actuals:
  tokens: 3745
  tasks: 3
  commits: 4

plan_head_before: 73d895ab44098c32c132c78642323f00d428b600
plan_head_after: 59aaf53af22dfd95bfa1c1756df42475c3bc48fe
commits: 4

tech-stack:
  added: []
  patterns:
    - "A loosened limit needs a reasons entry for its key whose text the base file does not already hold; an absent base passes"
    - "A fixture measured in bytes differs on Windows, where file(WRITE) writes CRLF; measure fixtures in lines"

key-files:
  created: []
  modified:
    - quality.json
    - tests/cmake/quality.cmake
    - tests/policy/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - .github/workflows/ci.yml
    - docs/maintaining.md
    - docs/testing.md

key-decisions:
  - "Size ceilings are 15 percent above the largest value measured, rounded up to a whole kibibyte: library 228352, libretro 198656, runner 253952, wasm 1037312"
  - "Two controls beyond the planned three: tightened and new_reason prove the two passing paths of the ratchet"
  - "Enforcement split: the ci preset covers tests.* and docs.max_bytes; each build leg's step covers the three zips; the wasm32 lane's step covers size.wasm"

requirements-completed: [TUNE-04]

coverage:
  - id: R1
    description: "quality.cmake with -DBASE passes an absent base, fails a ceiling or floor loosened with no new reason or with a reason text the base already holds, and passes a tightening or a loosening with a new reason"
    requirement: TUNE-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.quality' exit 0, 10 of 10 tests; RED run before the ratchet failed base_absent, loosened and reused_reason; cmake -DQUALITY=quality.json with -DBASE=quality.json -P tests/cmake/quality.cmake exit 0"
        status: pass
    human_judgment: false
  - id: R2
    description: "quality.json holds measured size.zip.library, size.zip.runner, size.zip.libretro and size.wasm ceilings with a reasons entry per key naming the run it came from"
    requirement: TUNE-04
    verification:
      - kind: other
        ref: "jq -e '.ceilings | has(\"size.zip.library\") and has(\"size.zip.runner\") and has(\"size.zip.libretro\") and has(\"size.wasm\")' quality.json exit 0; main push run https://github.com/szTheory/cogmaster/actions/runs/42741490263 for the zips, phase branch run https://github.com/szTheory/cogmaster/actions/runs/42758855565 for the wasm module"
        status: pass
    human_judgment: false
  - id: R3
    description: "Each build leg enforces its three zip ceilings after packaging and the wasm32 lane enforces size.wasm, each with a negative control that exits 1 naming the key"
    requirement: TUNE-04
    verification:
      - kind: e2e
        ref: "gh run view 42759595082 --exit-status exit 0, run https://github.com/szTheory/cogmaster/actions/runs/42759595082; six Size ceilings steps and the wasm32 Size ceiling step passed; both negative control commands exit 0 locally"
        status: pass
    human_judgment: false
  - id: R4
    description: "The trusted base copy carries quality.json so the Linux x64 leg runs the ratchet against the base branch file"
    requirement: TUNE-04
    verification:
      - kind: e2e
        ref: "grep -q 'compat-base/quality.json' .github/workflows/ci.yml exit 0; build (linux-x64) Size ceilings step passed in run 42759595082; actionlint -ignore 'label \"ubuntu-26.04' .github/workflows/ci.yml exit 0"
        status: pass
    human_judgment: false
  - id: R5
    description: "The maintainer docs describe the keys, where each is enforced, hand tightening and the new-reason rule, and the testing guide names policy.quality, policy.lessons_unique and the quality.json docs cap"
    requirement: TUNE-04
    verification:
      - kind: other
        ref: "for f in docs/*.md; do s=$(wc -c < $f); [ $s -lt 38912 ] || exit 1; done exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 printing 667 check lines / 7455 src lines; docs/maintaining.md read against ci.yml and quality.cmake"
        status: pass
    human_judgment: false
  - id: R6
    description: "A first hosted workflow_dispatch run of the phase branch is green with the size steps, the eight-file hash equality and the re-derived timeouts in place"
    requirement: TUNE-04
    verification:
      - kind: e2e
        ref: "gh run view 42759595082 --exit-status exit 0; all jobs including hash-equality, wasm32, perf and CI required succeeded; https://github.com/szTheory/cogmaster/actions/runs/42759595082"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 06: Size ceilings and the quality ratchet Summary

**`quality.cmake` now compares `quality.json` with the base branch's copy and refuses a loosened limit without a new reason, the three zips per build leg and the wasm module are held to measured ceilings with negative controls, and a first phase-branch run is green.**

## Performance

- **Tasks:** 3 of 3, 4 commits (one a fix found by the first hosted run)
- **Files:** 8 modified

## Accomplishments

- Task 1, commit `d3cc725`. `-DBASE=<file>` makes `quality.cmake` fail with `quality: <key>: loosened from <b> to <h> with no new reason` for any base ceiling that rises or goes and any base floor that falls or goes, unless the head's `reasons` holds an entry for that key whose text the base does not hold. An absent base prints `quality: no base file` and passes. Five controls in `tests/policy/tests.cmake`: `base_absent`, `tightened`, `new_reason` (passing) and `loosened`, `reused_reason` (failing through `expect_fail.cmake`). All five are in `ci.txt` and `wasm32.txt`.
- Task 2, commit `3f76c2e`. `quality.json` gains `size.zip.library` 228352, `size.zip.libretro` 198656 and `size.zip.runner` 253952. Each build leg has a `Size ceilings` step after packaging that checks its three zips and runs an oversize control; on Linux x64 it also passes `-DBASE` from the trusted copy, which `Preserve baseline judge` now fills with `quality.json`. The wasm32 lane has a `Size ceiling` step for `size.wasm` with its own control. `docs/maintaining.md` has a "Quality ceilings and floors" section and `docs/testing.md` "Budgets" names `docs.max_bytes`, `policy.quality` and `policy.lessons_unique`.
- Task 3, commits `328c405` and `59aaf53`. The first dispatch failed on a fixture bug (see Deviations); the second measured `size.wasm` as 901343 bytes (the key was absent, so the step failed and printed it); `size.wasm` 1037312 was set with a reasons entry naming that run; the third dispatch is green.

## Measurements

Zips: main push run https://github.com/szTheory/cogmaster/actions/runs/42741490263, downloaded with `gh run download 42741490263 -p 'archives-*' -D build/quality-measure` (exit 0). Sizes in bytes per leg:

| Leg | library | runner | libretro |
|-----|---------|--------|----------|
| linux-x64 | 164533 | 152523 | 133739 |
| linux-arm64 | 166024 | 153557 | 134185 |
| macos-arm64 | 158209 | 141878 | 127632 |
| macos-x64 | 157246 | 137500 | 123661 |
| windows-x64 | 198021 | 220520 | 172530 |
| windows-arm64 | 192648 | 209903 | 164199 |
| maximum | 198021 | 220520 | 172530 |

Each ceiling is the maximum plus 15 percent, rounded up to a whole kibibyte. Wasm: 901343 bytes from the wasm32 `Size ceiling` step of https://github.com/szTheory/cogmaster/actions/runs/42758855565 (no WASI toolchain is installed locally, so it was not built here). Windows is the largest leg for every component; on the final dispatch its library zip was 198219 bytes.

## Evidence

All commands run from the repository root. No log is committed; scratch output went to the session scratchpad.

| Command | Exit |
|---------|------|
| `ctest --test-dir build/ci -R '^policy\.quality'` before the ratchet (RED) | 8, base_absent, loosened and reused_reason failed |
| the same ctest after the ratchet (GREEN) | 0, 10 of 10 |
| `cmake -DQUALITY=quality.json "-DMEASURED=tests.ci=lines:tests/inventory/ci.txt" -DBASE=quality.json -P tests/cmake/quality.cmake` | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0, 667 check lines / 7455 src lines |
| `actionlint -ignore 'label "ubuntu-26.04' .github/workflows/ci.yml` | 0 |
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` | 0 |
| `gh run view 42757934566` (dispatch 1, `3f76c2e`) | failure, both Windows legs: two new controls measured bytes |
| `gh run view 42758855565` (dispatch 2, `328c405`) | failure, wasm32 `size.wasm` absent, measured 901343; `perf` cancelled at its 6 minute limit during a slow apt install |
| `gh run view 42759595082 --exit-status` (dispatch 3, `59aaf53`) | 0, all jobs green including hash-equality (eight files), wasm32, perf and CI required |
| `jq -e '.ceilings \| has("size.wasm")' quality.json` | 0 |
| `grep -Eq 'each .?build.? leg checks its zip sizes.*wasm32 lane checks the wasm size' .planning/ROADMAP.md` | 0 |

Final phase-branch dispatch run: https://github.com/szTheory/cogmaster/actions/runs/42759595082. It is an intermediate check; the pull_request run on the final phase head is plan 05-14 Task 3.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Ratchet fixtures measured bytes, which differ on Windows**
- **Found during:** Task 3, first hosted run
- **Issue:** `three.txt` is 15 bytes on Unix and 19 on Windows (CRLF), so `tightened` and `new_reason` expected `measured=15` and failed on both Windows legs, which skipped wasm32 and hash-equality
- **Fix:** the five ratchet controls measure `lines:` (3 on every platform) against `tests.fixture` limits 4 and 5
- **Files modified:** tests/policy/tests.cmake
- **Commit:** 328c405

**2. [Rule 2 - Missing critical] Two passing-path controls added**
- **Found during:** Task 1
- **Issue:** the plan listed three controls, all but one failing paths; the tightened and new-reason passes (the behaviours "tightening is free" and "a new reason permits a rise") had no test
- **Fix:** added `policy.quality.tightened` and `policy.quality.new_reason`
- **Files modified:** tests/policy/tests.cmake, tests/inventory/ci.txt, tests/inventory/wasm32.txt
- **Commit:** d3cc725

**Total deviations:** 2 auto-fixed. **Impact:** two extra tests per lane, one extra dispatch.

## Deferred Issues

- The `perf` job's 6 minute limit was hit once in dispatch 2: `apt-get install valgrind` took about 5 minutes on that runner, and the job was cancelled at its Cachegrind step. It passed in 80 seconds on dispatch 3. The limit was derived in an earlier plan from 26 runs (2 x 151 s); one slow-mirror run is not evidence to change it here, but plan 05-14's final run should show whether it recurs.

## Known Stubs

None.

## Threat Flags

None.

## Self-Check: PASSED

- FOUND: quality.json, tests/cmake/quality.cmake, .github/workflows/ci.yml, docs/maintaining.md
- FOUND commit d3cc725
- FOUND commit 3f76c2e
- FOUND commit 328c405
- FOUND commit 59aaf53
