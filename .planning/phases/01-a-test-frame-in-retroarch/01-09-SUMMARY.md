---
phase: 01-a-test-frame-in-retroarch
plan: 09
subsystem: ci-workflow
status: complete
tags:
  - ci
  - workflow
  - retroarch
  - perf
  - hygiene
requires:
  - "01-07: tests/cmake/install_line.cmake with FLIP, the three zips per leg"
  - "01-08: lane presets, tests/cmake/hashes.cmake MODE=compare, classify-changes.sh release_inputs, pins.txt retroarch-dmg and wasmtime rows, the bench preset"
provides:
  - ".github/workflows/ci.yml: changes (heavy, release_inputs), hygiene, title, build x6, asan, nofp x2, be32, wasm32 (report only), hash-equality (COUNT=7), retroarch, perf (merge base, report mode), CI required; workflow_call input ref"
  - "Artifacts archives-<os>-<arch> (three zips and commit.txt), hashes-<os>-<arch>, hashes-be32, hashes-wasm32"
  - "tests/retroarch/: run_retroarch.cmake, test.cfg.in, bmp.c, compare.c, flip.cmake (293 lines)"
  - "Hygiene CTests policy.ci_required(.self_test), hygiene.timeout_comments(.self_test), policy.action_pins(.self_test), policy.e2e_budget(.self_test)"
affects:
  - "plan 10: rehearsal and cleanup jobs join CI required's needs and the jq program gains the release_inputs clause; policy.ci_required needs new synthetic cases"
  - "plan 11: pushes the branch and drives every job green; confirms the 256x192 screenshot"
  - "plan 12: replaces every '# provisional, seed value' timeout with a measured one; docs/testing.md describes the jobs"
actuals:
  tokens: 12561
  tasks: 2
  commits: 2
plan_head_before: abd950941e93a1f944878d39c05b34810a07f8af
plan_head_after: 913012be91f5264c008e8145367c69cd714d739c
tech-stack:
  added: []
  patterns:
    - "CI required's synthetic needs are derived from its own needs list, so adding a job adds it to every roll-up case"
    - "A hosted job's negative control lives in a counted CMake -P script, not inline workflow shell"
key-files:
  created:
    - .github/workflows/ci.yml
    - tests/cmake/ci_required.cmake
    - tests/cmake/timeouts.cmake
    - tests/cmake/action_pins.cmake
    - tests/cmake/e2e_budget.cmake
    - tests/retroarch/run_retroarch.cmake
    - tests/retroarch/test.cfg.in
    - tests/retroarch/bmp.c
    - tests/retroarch/compare.c
    - tests/retroarch/flip.cmake
  modified:
    - tests/hygiene/tests.cmake
    - tests/inventory/hygiene.txt
decisions:
  - "wasm32 stays out of CI required's needs, as in the seed (policy.ci_required runs with REPORT_ONLY=wasm32); its Linux x64 comparison prints equal or differs to the step summary and never fails the job"
  - "The perf mode is a workflow env COGMASTER_PERF_MODE=report passed to perf_compare.cmake as MODE; arming is one ci: pull request changing it to gate"
  - "perf builds both commits with the bench preset and copies each cogmaster-run to equal-length paths; a merge base whose CMakePresets.json has no bench preset gives verdict NO_BASE"
  - "flip.cmake also runs compare and requires exit 1 naming pixel 7,3, so the whole negative control is inside the 300-line budget"
requirements-completed: []
coverage:
  - id: D1
    description: "ci.yml defines changes, hygiene, title, six build legs, asan, nofp, be32, wasm32, hash-equality, retroarch, perf and CI required; every uses: is pinned to a 40-hex commit with a # v comment; actionlint finds nothing but the runner labels its release does not know yet"
    requirement: FRAME-01
    verification:
      - kind: other
        ref: "grep -Ec '^  (changes|...|ci-required):' .github/workflows/ci.yml (prints 12); pinned and total uses: counts equal (16 and 16); actionlint -ignore 'is unknown. available labels' .github/workflows/ci.yml (exit 0)"
        status: pass
      - kind: e2e
        ref: "hosted CI run of the build legs, plan 11"
        status: unknown
    human_judgment: false
  - id: D2
    description: "hash-equality compares the six legs' and be32's hashes.txt with COUNT=7; wasm32 compares with Linux x64 and only reports"
    requirement: FRAME-02
    verification:
      - kind: e2e
        ref: "hosted CI run of hash-equality and wasm32, plan 11"
        status: unknown
    human_judgment: false
  - id: D3
    description: "The asan lane runs the ci inventory without abi and release, the nofp lanes run on x64 and arm64 with the nofp inventory"
    requirement: FRAME-04
    verification:
      - kind: e2e
        ref: "hosted CI run of asan and nofp, plan 11"
        status: unknown
    human_judgment: false
  - id: D4
    description: "policy.ci_required runs the CI required jq program on 12 synthetic cases and its control (heavy ignored) fails naming 'build skipped when heavy=true: want fail, got exit 0'; hygiene.timeout_comments and policy.action_pins pass on ci.yml and their controls fail with 'timeout_comments: 2 lines' and 'action_pins: sample.yml: uses: actions/checkout@v4'"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.ci_required"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.ci_required.self_test"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.timeout_comments.self_test"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.action_pins.self_test"
        status: pass
    human_judgment: false
  - id: D5
    description: "The macOS arm64 build leg runs the README install line against its own libretro zip and with FLIP=1; the retroarch job runs the pinned RetroArch 1.22.2 on the core for 60 frames, compares its screenshot with cogmaster-run --png 60 at 256x192, and flip.cmake requires compare to reject a one-byte-flipped copy"
    requirement: FRAME-07
    verification:
      - kind: other
        ref: "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -P tests/cmake/install_line.cmake (exit 0) and the same with -DFLIP=1 (exit 0)"
        status: pass
      - kind: other
        ref: "cmake -P tests/retroarch/flip.cmake on the runner's own frame 60 BMP (exit 0, 'first difference at 7,3'); the same with COMPARE=/usr/bin/true (exit 1)"
        status: pass
      - kind: e2e
        ref: "hosted retroarch job, plan 11"
        status: unknown
    human_judgment: false
  - id: D6
    description: "policy.e2e_budget counts 293 lines under tests/retroarch; its control fails with 'e2e_budget: 301 lines' on a 301-line fixture"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.e2e_budget"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.e2e_budget.self_test"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every build leg also builds and tests the installed-package, C++ header and add_subdirectory consumers through the ci or ci-msvc workflow"
    requirement: FRAME-08
    verification:
      - kind: e2e
        ref: "hosted CI run of the six build legs, plan 11"
        status: unknown
    human_judgment: false
  - id: D8
    description: "perf measures Cachegrind Ir/frame of the head commit and of git merge-base origin/<base> <head>, the base twice, and passes MODE=report to perf_compare.cmake; workflow_dispatch measures one commit three times as a control"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "grep -Eq 'git merge-base' .github/workflows/ci.yml (exit 0)"
        status: pass
      - kind: e2e
        ref: "hosted perf job on the probe pull request, plan 11"
        status: unknown
    human_judgment: false
---

# Phase 1 Plan 09: The Required Workflow and the Hosted RetroArch Compare Summary

**`ci.yml` now has 12 jobs: six build legs, the asan, nofp, be32 and wasm32 lanes, the seven-file hash
comparison, a RetroArch 1.22.2 screenshot compare on macOS, a report-only Cachegrind job against the merge base,
and the `CI required` roll-up. Four hygiene gates with negative controls check the roll-up, the timeouts, the action
pins and the 300-line RetroArch budget (293 lines used).**

## What was built

- **Task 1** (`c74db83`): `ci.yml` was adapted from the seed. The `changes` job now outputs `release_inputs`. The
  macOS arm64 leg runs the install line plain and with `FLIP=1`. The `archives-*` artifact carries `commit.txt`.
  The wasm32 comparison only reports. `perf` measures against the merge base with the `bench` preset. Also added:
  `ci_required.cmake` and `timeouts.cmake` from the seed, the new `action_pins.cmake`, and six hygiene CTests.
- **Task 2** (`913012b`): the seed's four `tests/retroarch/` files, verbatim. A new `flip.cmake` replaces the
  seed's inline shell for the byte flip. Also added: the `retroarch` job, `e2e_budget.cmake`, and two hygiene
  CTests.

## Verification (command, exit status)

- `cmake --workflow --preset hygiene`: exit 0 (22 tests).
- `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`: exit 0.
- `cmake --workflow --preset ci`: exit 0.
- `ctest --test-dir build/hygiene --no-tests=error -R '^(policy\.ci_required|hygiene\.timeout_comments|policy\.action_pins)'`: exit 0. The same selection with `--show-only=json-v1 | jq -e '.tests | length == 6'`: exit 0.
- `ctest --test-dir build/hygiene --no-tests=error -R '^policy\.e2e_budget'`: exit 0. The same selection with `--show-only=json-v1 | jq -e '.tests | length == 2'`: exit 0.
- `cmake -DDIR=tests/retroarch -P tests/cmake/e2e_budget.cmake`: exit 0. `cat tests/retroarch/* | wc -l`: prints 293.
- `grep -Ec '^  (changes|hygiene|title|build|asan|nofp|be32|wasm32|hash-equality|retroarch|perf|ci-required):' .github/workflows/ci.yml`: prints 12.
- The pinned `uses:` count equals the total `uses:` count: both are 16.
- `grep -Eq 'git merge-base' .github/workflows/ci.yml`: exit 0.
- `actionlint -ignore 'is unknown. available labels' .github/workflows/ci.yml`: exit 0. The installed actionlint does
  not know the `ubuntu-26.04` labels yet, which is the only other finding.
- The two `install_line.cmake` runs (plain and `-DFLIP=1`) against `build/ci/`: exit 0 each.
- `cc … tests/retroarch/bmp.c tests/retroarch/compare.c` and then `compare runner.bmp runner-copy.bmp` on the
  runner's frame 60, converted with `sips`: exit 0 and `256x192, every pixel equal`. `flip.cmake` on that BMP:
  exit 0. `flip.cmake` with `COMPARE=/usr/bin/true`: exit 1.
- `git log --format=%B origin/main..HEAD > build/commit-bodies.txt`: exit 0. The skip-token grep over that file:
  exit 1, so no commit body holds a skip token.

**Not run here:** none of the hosted jobs runs locally. In particular, RetroArch aborts in the agent sandbox.
Plan 11 pushes the branch and drives the run green.

## Deviations from Plan

### Auto-fixed Issues

**1. [Interpretation] How wasm32 is left out of the roll-up**
- **Found during:** Task 1
- **Issue:** the plan asks for "CI required needs every job" and for wasm32 to be "allowed to fail". The seed
  meets the second by leaving wasm32 out of the needs, with `REPORT_ONLY=wasm32` in `policy.ci_required`, which the
  plan also names.
- **Fix:** kept the seed's form: wasm32 is not in the needs, and the roll-up check accepts it as report-only. The
  wasm32 compare step prints its result and exits 0.
- **Commit:** `c74db83`

**2. [Rule 3 - Blocking] `flip.cmake` variable names**
- **Found during:** Task 2
- **Issue:** the first draft passed `-DIN`/`-DOUT`. `IN` is a keyword in `foreach()`, so the required-variable
  loop failed.
- **Fix:** renamed the variables to `BMP` and `FLIPPED`.
- **Commit:** `913012b`

**3. Smaller choices**
- `ci_required.cmake` builds its synthetic `needs` from the `needs:` list of `CI required` instead of a hardcoded
  list. Plan 10's `rehearsal` and `cleanup` jobs therefore join every case without an edit.
- The control in `policy.ci_required.self_test` fails at configure if `ci.yml` no longer holds the
  `$heavy == "false"` text it removes. A silent no-op replace would otherwise make the control vacuous.
- `perf` reads the base branch from `github.event.pull_request.base.ref`, with `main` as the default, rather than
  hard-coding `origin/main`.
- `flip.cmake` also runs `compare` and requires exit 1 naming pixel 7,3. The whole control therefore sits inside
  the counted budget.

## Known Stubs

None.

## Threat Flags

None beyond the plan's scope. The workflow keeps `contents: read`, and only `changes` and `title` add
`pull-requests: read`. Every checkout uses `persist-credentials: false`. The one downloaded binary, RetroArch, is
checked by SHA-256 through `fetch-pin.sh` before `hdiutil` opens it.

## Next

Plan 01-10 (release.yml and the rehearsal/cleanup jobs inside `ci.yml`).

## Self-Check: PASSED
