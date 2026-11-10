---
phase: 02-the-cpu-matches-the-public-vectors
plan: 02
subsystem: z80-vectors
status: complete
tags:
  - z80
  - singlesteptests
  - vectors
  - negative-control
requires:
  - "02-01: cogmaster-sst2bin, cogmaster-test-sst, tests/z80/sst_stream.c and the committed sample"
provides:
  - "cogmaster-test-sst --corrupt-copy OUT NAME TEST FIELD and a build-time corrupted copy at build/<preset>/tests/vectors/sst-corrupt.bin"
  - "cogmaster-test-sst --allow FILE with tests/vectors/z80/allow.txt (empty, format comment only)"
  - "Six ci tests labelled portable;vectors: vectors.self_test, vectors.allow.self_test, vectors.sst2bin.accept, vectors.sst2bin.missing_key, vectors.sst2bin.unknown_key, vectors.sst2bin.bad_stream"
  - "docs/conformance.md, the vectors section"
affects:
  - "02-03 to 02-05: opcode plans run the sample with --allow tests/vectors/z80/allow.txt when a full-sample test is registered"
  - "phase 2 exit row now reads ctest -L vectors"
actuals:
  tokens: 9000
  tasks: 2
  commits: 4
plan_head_before: d7b191604f84fdb6dec071e788d272cca5acaaf7
plan_head_after: 69a48229c928ab4936701f46689886f7631700fd
tech-stack:
  added: []
  patterns:
    - "tests/z80/tests.cmake is dual-mode like tests/runner/tests.cmake: run with -P it converts one fixture and runs the harness on the result"
    - "Every z80 tool command, at build time and in tests, goes through CMAKE_CROSSCOMPILING_EMULATOR"
key-files:
  created:
    - tests/z80/fixtures/accept/00.json
    - tests/z80/fixtures/missing_key/00.json
    - tests/z80/fixtures/unknown_key/00.json
    - tests/z80/fixtures/bad_stream/00.json
    - tests/vectors/z80/allow.txt
    - docs/conformance.md
  modified:
    - tests/z80/sst_run.c
    - tests/z80/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/CONFORMANCE.md
    - .planning/preparation/SMSPOWER-EXTRACTS.md
decisions:
  - "The corrupted copy re-encodes the one test rather than flipping a byte in place: an unchanged final field is stored only once, in the initial state, so flipping that byte would change initial and final together and the core would still match"
  - "Allow-list lines quote the file name (\"dd 34\"), since upstream file names contain spaces, matching how the mismatch line prints it"
  - "An allow line counts as unmatched only when its file ran, so a subset run with --only or --group is not failed by lines for other files"
requirements-completed: []
coverage:
  - id: D1
    description: "A build-tree copy of the sample with file 00 test 0's expected final a changed makes the harness exit 1 with the mismatch line naming the file, the test and the field"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.self_test$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "The committed sample is unchanged and the corrupted copy differs from it"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "git diff --quiet -- tests/vectors/z80/sst-sample.bin && ! cmp -s build/ci/tests/vectors/sst-corrupt.bin tests/vectors/z80/sst-sample.bin"
        status: pass
    human_judgment: false
  - id: D3
    description: "An allow-list line that matches no failure makes the harness exit 1"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.allow\\.self_test$'"
        status: pass
    human_judgment: false
  - id: D4
    description: "The converter's refusals and acceptance are tests: missing key exits 1 naming it, an unknown key converts, a stream that does not rebuild exits 1 naming file 00 and test 0, and the accept fixture converts and runs with executed 1"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.sst2bin\\.'"
        status: pass
    human_judgment: false
  - id: D5
    description: "Exactly six vectors tests are registered and listed in both inventories, and the ci workflow and its junit inventory check pass"
    requirement: CPU-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci"
        status: pass
      - kind: other
        ref: "cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/conformance.md describes the sample, pin, licence, access form, templates, regeneration, negative control, allow list and running one file, with no planning IDs; the preparation pack no longer names the z80 label, the Python converter or p as never changing"
    requirement: CPU-01
    verification:
      - kind: other
        ref: "grep -rIE planning-ID pattern over src tests docs include exits 1; grep for ctest -L z80 and sst2bin.py exits 1"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene"
        status: pass
    human_judgment: false
duration: 12min
completed: 2026-10-19
---

# Phase 2 Plan 02: Vector harness negative controls Summary

**The vector harness is shown to fail: a build-tree copy of the sample with one expected final `a` flipped makes it exit 1 with the exact mismatch line. A dated allow list fails on any line that excuses nothing, and four hand-written fixtures pin the converter's refusals, all as `vectors.*` tests in `ci`.**

## Performance

- **Duration:** about 12 min
- **Completed:** 2026-10-20T02:22Z
- **Tasks:** 2 of 2
- **Files:** 13 created or modified

## Accomplishments

- `cogmaster-test-sst --corrupt-copy OUT NAME TEST FIELD` writes the sample with one test's expected final field flipped in its lowest bit. A build-time custom command, run through `CMAKE_CROSSCOMPILING_EMULATOR`, writes `build/<preset>/tests/vectors/sst-corrupt.bin`. `vectors.self_test` requires exit 1 and `mismatch "00" test 0 a: ...`.
- `--allow FILE` reads `YYYY-MM-DD "NAME" TEST FIELD https://... REASON` lines. A malformed line exits 1. A matching mismatch prints `allowed ...` and is not counted. A line whose file ran and that matched nothing prints `allow line N (...) matches no failure` and exits 1.
- Four one-test NOP fixtures in the upstream schema: `accept`, `missing_key` (no `wz`), `unknown_key` (extra `qff`), and `bad_stream` (opcode data moved to T2).
- `docs/conformance.md` (5,247 B) covers the vectors pipeline. ARCHITECTURE section 11, CONFORMANCE section 1.1 and SMSPOWER-EXTRACTS' `p` entry were corrected.

## Task Commits

1. **Task 1 (tracer): corrupted copy and vectors.self_test:** `08bf5a5`. Tracer gate: the `<verify>` command (`cmake --build --preset ci && ctest ... -R '^vectors\.self_test$'`) was re-run end to end and exited 0, so expansion continued.
2. **Task 2 (tdd): allow list, converter fixture tests, docs:**
   - RED `c9ad16d`: tests, fixtures and allow.txt
   - GREEN `db4cef4`: `--allow` in the harness
   - docs `69a4822`: conformance guide and preparation corrections

## TDD Gate Compliance

- RED (`c9ad16d`): `ctest --test-dir build/ci -R '^vectors\.'` ran 6 tests and 1 failed: `vectors.allow.self_test` got exit 2 (usage), because `--allow` did not exist yet. The four converter tests passed at RED because the converter already refused these inputs (plan 01). They are characterization tests of existing behaviour.
- GREEN (`db4cef4`): all 6 pass.
- REFACTOR: none beyond clang-format.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The corrupted copy re-encodes the test instead of flipping one byte**
- **Found during:** Task 1
- **Issue:** NOP leaves `a` unchanged, so test 0's final `a` is not stored. It comes from the initial byte through the change mask. Flipping that byte would change initial and final together, and the harness would pass.
- **Fix:** decode the test, flip `final[a]`, re-encode it with `sst_encode`, and copy every other byte unchanged. The copy is 1 byte longer than the sample.
- **Commit:** `08bf5a5`

**2. [Format] clang-format on a Task 1 line**
- One `fwrite` condition from Task 1 was reformatted in the GREEN commit `db4cef4`. There is no behaviour change.

**3. [Detail] Allow-list file names are quoted**
- Upstream names contain spaces, so the plan's `<file>` is written `"dd 34"`, as the mismatch line prints it.

## Known Stubs

None. `tests/vectors/z80/allow.txt` is empty on purpose (format comment only). No test runs the whole sample with it yet. That arrives with the opcode plans.

## Threat Flags

None. All changes are test-only host tools, fixtures and docs.

## Requirements

CPU-01 progressed but is not complete: the harness now proves it can fail, but the opcode pages are still to come. `requirements-completed` is empty and REQUIREMENTS.md is unchanged.

## Verification Run

- `cmake --workflow --preset ci`: exit 0
- `cmake --workflow --preset hygiene`: exit 0
- `check_junit.cmake` against `tests/inventory/ci.txt`: exit 0, 86 tests ran once each
- `ctest -R '^vectors\.' --show-only=json-v1 | jq -e '.tests | length == 6'`: true
- The asan preset could not be configured locally because clang-21 is not on PATH. CI covers that lane.

## Self-Check: PASSED

- All 6 created files exist on disk.
- Commits `08bf5a5`, `c9ad16d`, `db4cef4` and `69a4822` are ancestors of HEAD.
