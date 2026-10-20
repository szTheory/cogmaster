---
phase: 02-the-cpu-matches-the-public-vectors
plan: 12
subsystem: fixtures
status: complete
tags:
  - fixtures
  - wla-dx
  - toolchain
  - hashes
requires:
  - "02-11: cogmaster-run --probe and its checksum_ok key"
provides:
  - "cmake/WlaDx.cmake: WLA-DX v10.7 downloaded from the pins.txt wla-dx row, SHA-256 checked, wla-z80 and wlalink built natively at configure time; also a -P download-and-verify script mode"
  - "Option COGMASTER_FIXTURES (default ON; ON in ci, ci-msvc, asan, be32, wasm32; OFF with tests in bench)"
  - "Variables COGMASTER_WLA_Z80, COGMASTER_WLALINK, COGMASTER_WLA_SHA256, COGMASTER_FIXTURES_DIR; target cogmaster-fixtures"
  - "tests/roms/src/header.inc (32 KiB map, Sega header with ROMSIZE $C, .COMPUTESMSCHECKSUM), hello.asm, hello.link"
  - "tests/built/expected.txt: pinned image digests"
  - "hashes.txt gains 'fixture <name> sha256 <hex>' lines (hashes.cmake -DFIXTURES)"
  - "CTest fixtures.hello and fixtures.wla_pin.self_test, label fixtures"
affects:
  - "Plans adding mapper_sega, mapper_codemasters, ports and other fixtures: add the name to the foreach in tests/fixtures/tests.cmake and a line to tests/built/expected.txt"
actuals:
  tokens: 6900
  tasks: 2
  commits: 4
plan_head_before: d946d62138f827cec4c88d8b1e22cc1b47803ae2
plan_head_after: 9c8ecb0822cecd50e328c62f7f81b3db0214fb6d
tech-stack:
  added:
    - "WLA-DX v10.7 (GPL-2.0-or-later), as a build-tree tool only"
  patterns:
    - "Configure-time native child build through cmake -E env --unset of CC, CXX, CFLAGS, CXXFLAGS, LDFLAGS; native lanes pass their own C compiler, cross lanes none"
    - "Rebuild skipped while build/<preset>/wla-dx/pin.txt records the same version and digest and both programs exist"
key-files:
  created:
    - cmake/WlaDx.cmake
    - tests/fixtures/tests.cmake
    - tests/roms/src/header.inc
    - tests/roms/src/hello.asm
    - tests/roms/src/hello.link
    - tests/built/expected.txt
  modified:
    - pins.txt
    - .gitattributes
    - CMakeLists.txt
    - CMakePresets.json
    - tests/tests.cmake
    - tests/runner/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/cmake/abi.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
    - docs/testing.md
    - src/z80_ed.c
decisions:
  - "COGMASTER_FIXTURES is declared in CMakeLists.txt beside the test options, so the bench preset's OFF value is always a used variable"
  - "Fixture images are found by hashes.cmake as FIXTURES/*.sms, sorted, so a new fixture joins hashes.txt with no edit there"
  - "The negative control checks a local tarball against the real wla-dx digest, so it exercises the same function configure uses"
  - "hello.asm jumps to a label at $0000 rather than writing jp $; the bytes are the same C3 00 00"
requirements-completed: []
coverage:
  - id: D1
    description: "Configure downloads WLA-DX v10.7 from the pinned row, checks its SHA-256 and builds wla-z80 and wlalink natively on every test lane"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "gh run view 39368411916 --json conclusion --jq .conclusion"
        status: pass
    human_judgment: false
  - id: D2
    description: "fixtures.hello: build/<preset>/fixtures/hello.sms has the digest pinned in tests/built/expected.txt and cogmaster-run --probe prints checksum_ok=1"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.hello$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: "fixtures.wla_pin.self_test: a file:// tarball checked against the pinned digest fails the download with a hash mismatch, with no network"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^fixtures\\.wla_pin\\.self_test$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "hashes.txt carries a fixture hello line and the hash-equality job compares it across the six build legs and be32"
    requirement: CPU-03
    verification:
      - kind: integration
        ref: "grep -Eq '^fixture hello sha256 [0-9a-f]{64}$' build/ci/hashes.txt"
        status: pass
      - kind: integration
        ref: "gh run view 39368411916 --json jobs --jq '.jobs[] | select(.name==\"hash-equality\") | .conclusion'"
        status: pass
    human_judgment: false
  - id: D5
    description: "tests/roms/src/** is text with LF endings, after the /tests/roms/** -text line"
    requirement: CPU-03
    verification:
      - kind: other
        ref: "git check-attr text eol -- tests/roms/src/hello.asm"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 12: Pinned WLA-DX and the hello fixture Summary

WLA-DX v10.7 is now built from its SHA-256-pinned tarball at configure time with the host compiler. It assembles
`hello.sms` into the build tree. The image's digest and `--probe checksum_ok=1` are checked, and fixture digests
now go into the cross-lane `hashes.txt` comparison.

## What was built

- **Task 1 (tracer), `e8d582a`:** `cmake/WlaDx.cmake`, the `wla-dx` pins row, the LF rule for fixture sources,
  `header.inc`/`hello.asm`/`hello.link`, the `cogmaster-fixtures` target, `fixtures.hello`, and the preset and
  inventory changes. Tracer gate: the automated verify was re-run end to end and passed before Task 2 started.
- **Task 2 (TDD):** RED `d6b98fc` registered `fixtures.wla_pin.self_test` and passed `-DFIXTURES` to
  `runner.hashes_file`. The plan's grep for `fixture hello sha256` then failed (exit 1). GREEN `047eac0` taught
  `hashes.cmake` to write the fixture lines and added the documentation.

## WLA-DX build time (measured, as D-21 asks)

The configure-time download plus native build of wla-z80 and wlalink took 2 to 3 s locally on Apple M-series.
On CI run 39368411916 it took 5 to 14 s per lane: linux-x64 7, linux-arm64 5, macos-arm64 6, macos-x64 14,
windows-x64 7, windows-arm64 9, asan 6, be32 9, wasm32 9. No cache and no timeout were added. Windows arm64
built with upstream's `/WX` unchanged, so the `/WX-` fallback was not needed. Every leg assembled the same
`hello` digest (hash-equality passed), so the build-once-and-share fallback was not needed either.

## Evidence

- `cmake --workflow --preset ci` exited 0 (166 tests, then 4 release tests).
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` exited 0.
- `sh scripts/hygiene.sh --tree` exited 0; `cmake --workflow --preset hygiene` exited 0, and `build/hygiene` holds no WLA-DX.
- `cmake --workflow --preset bench` exited 0 with no WLA-DX download.
- `gh workflow run ci.yml --ref phase/02-the-cpu-matches-the-public-vectors`, then `gh run watch 39368411916 --exit-status`, exited 0.
  `gh run view 39368411916 --json conclusion --jq .conclusion` prints `success`:
  https://github.com/szTheory/cogmaster/actions/runs/39368411916
- The first dispatch, run 39368145441, failed for reasons this plan did not cause (see Deviations):
  https://github.com/szTheory/cogmaster/actions/runs/39368145441

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] GCC `-Wsign-conversion` error in `src/z80_ed.c`**
- **Found during:** Task 2, first CI dispatch. This was the branch's first CI run, so the error came from an
  earlier plan.
- **Issue:** `f & ~HF` turns the enum into the int -17, which fails every GCC leg (linux, nofp, be32). Apple clang
  does not warn.
- **Fix:** `~(unsigned)HF`. A GCC 15 container compile of every core, runner and test source found no other error.
- **Commit:** `9c8ecb0`

**2. [Rule 3 - Blocking] `abi.undefined` on Windows x64: `__ImageBase`**
- **Found during:** Task 2, first CI dispatch.
- **Issue:** On x64, MSVC's switch jump tables use offsets from `__ImageBase`, a symbol the linker defines. It
  is not a runtime dependency.
- **Fix:** Added it to the Windows x64 helper row in `tests/cmake/abi.cmake`, with a comment citing the run.
- **Commit:** `9c8ecb0`

**3. [TDD note] The self test passed at RED**
- `cmake/WlaDx.cmake` was written whole in Task 1, script mode included, so `fixtures.wla_pin.self_test` passed
  when it was registered. Its failure path was checked by hand instead: `cmake -DURL=file://... -DSHA256=<pin>
  -P cmake/WlaDx.cmake` exits 1 with `file DOWNLOAD HASH mismatch`. The hashes half of the RED did fail as
  expected.

## Known Stubs

None.

## Next

Plan 02-13.

## Self-Check: PASSED
