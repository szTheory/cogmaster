---
phase: 01-a-test-frame-in-retroarch
plan: 08
subsystem: ci-lanes-and-hygiene
status: complete
tags:
  - presets
  - portability
  - hygiene
  - pins
  - classifier
requires:
  - "01-03/01-04: cogmaster-run --frames 60 --hash and --machine gg, the pinned card hashes"
  - "01-05: the vendored libretro/libretro.h, which hygiene.format excludes"
  - "01-07: scripts/publish.sh prepare and verify, the 18 zip names, the ci workflow with package and release steps"
provides:
  - "Configure, build, test and workflow presets hygiene, asan, nofp, be32, wasm32 and bench (bench: configure and build of cogmaster-run only)"
  - "cmake/toolchains/ppc-linux-gnu.cmake (qemu-ppc emulator) and cmake/toolchains/wasm32-wasi.cmake (clang-21, wasi-libc, emulated signals, wasmtime from build/pins/wasmtime)"
  - "Options COGMASTER_HYGIENE and COGMASTER_NOFP; the inventory defaults to hygiene or nofp under them; a WASI configure fails naming the missing wasmtime and the fetch-pin command"
  - "pins.txt rows wasmtime-x86_64-linux, wasmtime-aarch64-linux, wasmtime-aarch64-macos 48.0.7 and retroarch-dmg 1.22.2"
  - "tests/cmake/hashes.cmake MODE=write (RUN, EMULATOR, OUT) and MODE=compare (FILES, COUNT); build/<preset>/hashes.txt holds the SMS and GG frame 60 hash lines"
  - "CTests runner.hashes_file, runner.hashes_compare.byte.self_test, runner.hashes_compare.count.self_test (ci, wasm32); abi.nofp_fixture (nofp only)"
  - "Hygiene preset CTests hygiene.tree, hygiene.tree.self_test, hygiene.format, hygiene.analyzer, hygiene.no_float, hygiene.no_float.self_test, policy.pins, policy.pins.self_test, policy.title, policy.title.self_test, ci.classify, policy.publish_set, policy.publish_set.self_test, policy.publish_ref"
  - "scripts/classify-changes.sh second output line release_inputs=true|false"
  - "Inventories tests/inventory/hygiene.txt, nofp.txt, wasm32.txt"
affects:
  - "plan 09: the hash-equality job runs hashes.cmake MODE=compare over the lanes' hashes.txt; policy.action_pins joins the hygiene inventory; the changes job reads release_inputs"
  - "plan 11: the asan, nofp, be32 and wasm32 lanes run for the first time on hosted runners"
  - "plan 12: the provisional timeout scales and the wasm32 toolchain deviation from ENGINEERING section 14 are recorded"
actuals:
  tokens: 13000
  tasks: 3
  commits: 4
plan_head_before: fa292f41328b1f6bfb980473931313cfd85d0544
plan_head_after: 510a545d6233fac5c058b42239389741fdbca78b
tech-stack:
  added: []
  patterns:
    - "A tree-scan negative control runs the scanner inside a throwaway git repository under the build tree, with the forbidden text assembled in CMake from pieces so the tracked test file never holds it"
    - "A control that must stop before writing checks both the named message and that DEST does not exist"
    - "Lane presets carry the seed's timeout scales in a $comment marked provisional until this repository's runs are measured"
key-files:
  created:
    - cmake/toolchains/ppc-linux-gnu.cmake
    - cmake/toolchains/wasm32-wasi.cmake
    - pins.txt
    - tests/abi/nofp_fixture.c
    - tests/cmake/hashes.cmake
    - tests/cmake/no_float.cmake
    - tests/cmake/pins.cmake
    - tests/hygiene/tests.cmake
    - tests/hygiene/fixtures/titles.txt
    - tests/inventory/hygiene.txt
    - tests/inventory/nofp.txt
    - tests/inventory/wasm32.txt
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - scripts/classify-changes.sh
    - scripts/check-title.sh
    - tests/tests.cmake
    - tests/abi/tests.cmake
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - runner/bench.c
    - runner/main.c
    - src/internal.h
    - src/state.h
    - tests/core/test_state.c
decisions:
  - "The hygiene branch finds clang-format and clang-tidy with a fixed HINTS list (Homebrew llvm@21 on Apple Silicon and Intel, Ubuntu llvm-21) and REQUIRED, and checks that both report major version 21, not only clang-format"
  - "The nofp preset names gcc, as the seed does, with a comment that the hosted image's GCC is 15; the lane fails configure on a compiler that is not GCC"
  - "pins.txt keeps no cmake-linux rows: the seed uses them only for local container runs, not in CI"
  - "release_inputs is forced true together with heavy only for input that cannot be trusted (no paths, a missing or non-numeric count, a count over 3000); a readable list of non-release paths gives release_inputs=false"
  - "check-title.sh and classify-changes.sh set LC_ALL=C, so a subject starting with a non-ASCII letter fails the lower-case rule on every host"
requirements-completed: []
coverage:
  - id: D1
    description: "Nine configure presets and nine workflow presets exist (dev, ci, ci-msvc, hygiene, asan, nofp, be32, wasm32, bench) and CMake accepts the presets file"
    requirement: FRAME-02
    verification:
      - kind: other
        ref: "cmake --list-presets=all && jq -e (visible configure presets and workflow presets equal the nine names) CMakePresets.json (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "runner.hashes_file writes build/ci/hashes.txt holding exactly the pinned SMS and GG frame 60 lines with no carriage return; the compare mode fails on two files one hex digit apart and on two files where three are required"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hashes_file"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hashes_compare.byte.self_test"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hashes_compare.count.self_test"
        status: pass
      - kind: other
        ref: "grep -Ec (the two pinned lines) build/ci/hashes.txt (prints 2); tr -d CR < build/ci/hashes.txt | cmp -s - build/ci/hashes.txt (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The asan, nofp, be32 and wasm32 lanes exist with toolchains, inventories and the nofp fixture control; they cannot configure on this Mac (no clang-21, GCC, powerpc cross compiler or wasmtime) and first run on hosted Ubuntu in a later plan"
    requirement: FRAME-02
    verification:
      - kind: other
        ref: "cmake --preset asan / nofp / be32 / wasm32 (exit 1 locally: missing clang-21, AppleClang refused by the nofp check, missing powerpc-linux-gnu-gcc, missing clang-21)"
        status: unknown
    human_judgment: false
  - id: D4
    description: "hygiene.tree scans tracked and untracked, not ignored files; hygiene.tree.self_test plants a home-directory path in an untracked file of a throwaway repository and scripts/hygiene.sh --tree exits 1 with 'hygiene: home directory path'"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.tree"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.tree.self_test"
        status: pass
      - kind: other
        ref: "grep -Ec '/(Users|home)/[A-Za-z]' tests/hygiene/tests.cmake (prints 0); sh scripts/hygiene.sh --tree (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "policy.pins checks every pins.txt row; policy.pins.self_test runs scripts/fetch-pin.sh on a file:// row with an all-zero digest, which exits 1 with 'payload: sha256 mismatch' and leaves DEST absent"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.pins"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.pins.self_test"
        status: pass
    human_judgment: false
  - id: D6
    description: "policy.title passes 9 and fails 27 fixture titles and bodies, including a plan-number scope, feat(ci), a non-ASCII first letter and a skip token in the body; policy.title.self_test fails 'Phase 1: A test frame in RetroArch' with 'not a Conventional Commit title'"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.title"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.title.self_test"
        status: pass
    human_judgment: false
  - id: D7
    description: "scripts/classify-changes.sh prints heavy= and release_inputs=; ci.classify checks both lines over 21 lists, among them each of the five release inputs, a rename away from scripts/publish.sh, look-alike paths and a .planning-only list; empty input gives heavy=true and release_inputs=true"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#ci.classify"
        status: pass
      - kind: other
        ref: "printf '' | sh scripts/classify-changes.sh 0 (heavy=true, release_inputs=true)"
        status: pass
    human_judgment: false
  - id: D8
    description: "hygiene.format (every tracked C file but libretro.h, clang-format 21), hygiene.analyzer (clang-analyzer over src/*.c) and hygiene.no_float with its six-line control pass; policy.publish_set lists 18 archives in C-locale order, its control fails naming cogmaster-runner-windows-x64.zip, and policy.publish_ref refuses a pull-request ref"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.format"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#hygiene.analyzer"
        status: pass
      - kind: integration
        ref: "tests/hygiene/tests.cmake#policy.publish_set.self_test"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene (exit 0, 14 tests); cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake (exit 0)"
        status: pass
    human_judgment: false
---

# Phase 1 Plan 08: Portability Lanes and the Hygiene Preset Summary

**Six new lane presets (hygiene, asan, nofp, be32 with qemu-ppc, wasm32 with wasmtime 48.0.7, and bench) now exist.
Every test lane writes `build/<preset>/hashes.txt` with the SMS and Game Gear hash lines for the cross-lane compare.
A `hygiene` workflow runs 14 tree, format, analyzer, float, pin, title, classifier and publish gates, and each gate
has a negative control that fails with its named message.**

## What was built

- **Task 1** (`9405034`): presets and workflows for the six lanes; the seed's two toolchain files; `pins.txt`;
  the hygiene and nofp branches in `CMakeLists.txt` and `tests/tests.cmake`; `abi.nofp_fixture`;
  `tests/cmake/hashes.cmake` and the three `runner.hashes_*` tests; the `ci`, `nofp` and `wasm32` inventories.
- **Task 2** (`c542382`, `9cc9cd9`): five files reformatted so `hygiene.format` passes (layout only).
  `tests/hygiene/tests.cmake` registers the tree scan and its throwaway-repository control, format, the analyzer,
  `no_float` with its control, and `policy.pins` with its wrong-digest control. `no_float.cmake` and `pins.cmake`
  are copied from the seed.
- **Task 3** (`510a545`): the `release_inputs=` line in `classify-changes.sh`, `LC_ALL=C` in it and in
  `check-title.sh`, `policy.title` and its control, the extended `ci.classify`, and the three publish policies.

## Verification (command, exit status)

- `cmake --list-presets=all >/dev/null && jq -e '…nine names…' CMakePresets.json`: exit 0.
- `cmake --workflow --preset ci`: exit 0 (80 tests, package, 4 release tests).
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake`: exit 0.
- `grep -Ec '^(frame 60 sha256 e14d…|frame 60 sha256 7404…)$' build/ci/hashes.txt`: prints 2. The carriage-return `cmp`: exit 0.
- `cmake --workflow --preset hygiene`: exit 0 (14 tests).
- `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`: exit 0.
- `ctest --test-dir build/hygiene --no-tests=error -R '^(policy\.title|…|policy\.publish_ref)$'`: exit 0. The same selection with `--show-only=json-v1 | jq -e '.tests | length == 6'`: exit 0.
- `printf 'src/a.c\nscripts/publish.sh\n' | sh scripts/classify-changes.sh 5 | grep -c '^release_inputs=true$'`: prints 1.
- `printf '.planning/STATE.md\n' | sh scripts/classify-changes.sh 1 | grep -Ec '^(heavy=false|release_inputs=false)$'`: prints 2.
- `cmake --workflow --preset bench`: exit 0.
- `sh scripts/hygiene.sh --tree`: exit 0.

**Not run here:** the asan, nofp, be32 and wasm32 lanes. This Mac has no clang-21 binary (only Apple clang 21), no
GCC, no powerpc cross compiler, no qemu and no wasmtime. Each preset fails configure with the missing tool's name,
and plan 11's hosted legs run them first. The libretro tests and `abi.nofp_fixture` are therefore not yet run on
those lanes.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Existing code was not clang-formatted**
- **Found during:** Task 2
- **Issue:** `hygiene.format` failed on `runner/bench.c`, `runner/main.c`, `src/internal.h`, `src/state.h` and
  `tests/core/test_state.c`. Earlier plans never had a format gate.
- **Fix:** ran `clang-format -i` (21.1.8) on those five files. Only layout changed, and the ci workflow passes.
- **Commit:** `c542382`

**2. [Rule 2 - Correctness] Title check now compares bytes**
- **Found during:** Task 3
- **Issue:** the plan's encoding truth says a subject that starts with a non-ASCII letter fails the lower-case rule.
  `check-title.sh` ran in the caller's locale, where `[a-z]` can match such a letter.
- **Fix:** set `LC_ALL=C` in `check-title.sh`, and in `classify-changes.sh` too. Added the fixture `fail|feat: éclair|`.
- **Commit:** `510a545`

**3. Smaller choices**
- The nofp compiler is `gcc`, as in the seed, not a versioned `gcc-15`. A `$comment` records that the hosted
  image's GCC is 15.
- `hygiene.analyzer` and the tool discovery also require clang-tidy at major version 21, not only clang-format.
- `policy.pins.self_test` wraps `fetch-pin.sh` in a short shell program. The program also fails if DEST exists
  afterwards, which makes the "before DEST is touched" claim a checked fact.

## Known Stubs

None.

## Threat Flags

None. The new scripts and tests read only the tree and fixtures under `build/`. The one download path,
`fetch-pin.sh`, already existed, and its control uses a `file://` URL.

## Next

Plan 01-09 (workflows: the hash-equality job over these `hashes.txt` files, `policy.action_pins`, and the changes job
reading `release_inputs`).

## Self-Check: PASSED
