---
phase: 01-a-test-frame-in-retroarch
plan: 07
subsystem: packaging
status: complete
tags:
  - install
  - cpack
  - pkg-config
  - readme
  - release
requires:
  - "01-05: the libretro module and cogmaster_libretro.info, installed as component libretro"
  - "01-06: tests/policy/tests.cmake, docs.budgets and expect_fail.cmake controls"
provides:
  - "Install rules (top level only): include/cogmaster.h, include/cogmaster_version.h, lib/libcogmaster.a, lib/cmake/cogmaster/{cogmasterConfig,cogmasterConfigVersion,cogmasterTargets}.cmake with SameMinorVersion, lib/pkgconfig/cogmaster.pc; exported target cogmaster::cogmaster"
  - "cmake/packaging.cmake: CPack ZIP per component, cogmaster-{library,runner,libretro}-<os>-<arch>.zip, no version, no top-level folder, LICENSE and THIRD-PARTY-NOTICES.md at every root; COGMASTER_OS and COGMASTER_ARCH"
  - "Presets: package ci and ci-msvc; test ci-release and ci-msvc-release (label ^release$, junit-release.xml); workflows ci and ci-msvc run configure, build, test, package, release test"
  - "CTests install.stage, install.consumer, install.consumer.cxx, install.consumer.dll_crt, embed.subdirectory (label install); release.archive_contents, release.archive_contents.self_test, release.floor, release.floor.self_test (label release); release.single_version_source, .next, .self_test (label policy)"
  - "tests/cmake/install_line.cmake with -DREADME, -DBASE_URL, -DFLIP, -DHOME_DIR"
  - "User README with the single checksum-verifying install line"
affects:
  - "plan 09: the macOS arm64 build leg runs install_line.cmake twice (plain and FLIP=1) against build/ci/"
  - "plan 10: the release workflow runs install_line.cmake against releases/latest/download/ and publish.sh consumes these 18 zip names"
  - "every later file that would hold the version string, which release.single_version_source rejects"
actuals:
  tokens: 12600
  tasks: 3
  commits: 3
plan_head_before: 1c43ecc745b45526e26ecec49a0a11fe3125f806
plan_head_after: cabe26b0ca282a40b1319b6c12f838e6e0ce7ebb
tech-stack:
  added: []
  patterns:
    - "An outside consumer project is built by ctest --build-and-test against the staged install, with this build's compilers and flags passed through"
    - "Release-labelled tests run in a second test step after the package step and read the zips it wrote"
    - "A README command is tested by extracting the one line with a fixed prefix and running it under sh with HOME set to a scratch directory"
key-files:
  created:
    - cmake/cogmasterConfig.cmake.in
    - cmake/cogmaster.pc.in
    - cmake/packaging.cmake
    - tests/install/tests.cmake
    - tests/consumer/CMakeLists.txt
    - tests/consumer/main.c
    - tests/consumer/main.cpp
    - tests/embed/CMakeLists.txt
    - tests/release/tests.cmake
    - tests/cmake/archive_contents.cmake
    - tests/cmake/floor.cmake
    - tests/cmake/single_version.cmake
    - tests/cmake/install_line.cmake
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - tests/tests.cmake
    - tests/policy/tests.cmake
    - tests/inventory/ci.txt
    - README.md
    - docs/embedding.md
decisions:
  - "Install rules and packaging are registered only when cogmaster is the top-level project, as in the seed: an embedding project installs nothing of ours"
  - "The library is static only, as in the seed; there is no shared library and so no SOVERSION. install.consumer.dll_crt is the seed's MSVC DLL C runtime variant of the consumer"
  - "The consumer project also builds consumer_pc from cogmaster.pc alone when pkg-config exists and the compiler is not MSVC; it is built, not run, by every install.consumer* test on such a lane"
  - "install_line.cmake rewrites a SHA256SUMS in a file:// base only when that file names the libretro zip alone (one it wrote), so a rebuilt zip is not refused for a stale entry and a release's full list is never touched"
requirements-completed: []
coverage:
  - id: D1
    description: "install.stage installs the library component into build/ci/stage; install.consumer and install.consumer.dll_crt build tests/consumer against it with find_package(cogmaster CONFIG REQUIRED) and print 'consumer: api 1, one frame 256x192'; install.consumer.cxx builds main.cpp and prints 'consumer_cxx: api 1'"
    requirement: FRAME-08
    verification:
      - kind: integration
        ref: "tests/install/tests.cmake#install.consumer"
        status: pass
      - kind: integration
        ref: "tests/install/tests.cmake#install.consumer.cxx"
        status: pass
      - kind: integration
        ref: "tests/install/tests.cmake#install.consumer.dll_crt"
        status: pass
      - kind: other
        ref: "ctest --test-dir build/ci --no-tests=error -L '^install$' --output-on-failure && ctest --test-dir build/ci -L '^install$' --show-only=json-v1 | jq -e '.tests | length == 5' (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "embed.subdirectory adds the source tree with COGMASTER_BUILD_TESTS, COGMASTER_BUILD_RUNNER and COGMASTER_BUILD_LIBRETRO off, fails on C++ being enabled, on cogmaster-run, cogmaster_libretro, cogmaster-test-core or cogmaster-host existing, or on any registered test, and runs the consumer frame"
    requirement: FRAME-08
    verification:
      - kind: integration
        ref: "tests/install/tests.cmake#embed.subdirectory"
        status: pass
    human_judgment: false
  - id: D3
    description: "cogmaster.pc carries -I include and -lcogmaster relative to its own directory; the consumer built by hand with exactly those flags against build/ci/stage runs and prints the API version (no pkg-config on the local Mac; the Linux legs build consumer_pc in CI)"
    requirement: FRAME-08
    verification:
      - kind: other
        ref: "cc -std=c17 -Ibuild/ci/stage/include tests/consumer/main.c -Lbuild/ci/stage/lib -lcogmaster -o <scratch>/pc_consumer && <scratch>/pc_consumer (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The ci workflow packages three version-free zips per leg; release.archive_contents finds LICENSE and THIRD-PARTY-NOTICES.md byte-identical to the tree's at each root, exactly the expected members (libretro: cores/ and info/ only, no test modules) and no build path; its self test fails a zip without the notices with 'lacks: THIRD-PARTY-NOTICES.md'"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/release/tests.cmake#release.archive_contents"
        status: pass
      - kind: integration
        ref: "tests/release/tests.cmake#release.archive_contents.self_test"
        status: pass
      - kind: other
        ref: "ls build/ci/cogmaster-*-macos-arm64.zip | wc -l (3); unzip -Z1 build/ci/cogmaster-libretro-macos-arm64.zip | grep -Eq 'rgb565|_gg' (exit 1)"
        status: pass
      - kind: other
        ref: "cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake (exit 0, 4 tests ran once each)"
        status: pass
    human_judgment: false
  - id: D5
    description: "release.floor reads the packaged runner and core: macOS minimum OS within 11.0 on arm64, only libSystem linked, only _retro_ exports; its self test with floor 9.0 fails naming 'minimum OS ... is above the floor 9.0'. Linux and Windows branches run on their legs in CI"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/release/tests.cmake#release.floor"
        status: pass
      - kind: integration
        ref: "tests/release/tests.cmake#release.floor.self_test"
        status: pass
    human_judgment: false
  - id: D6
    description: "release.single_version_source and .next find the current, next minor and next patch version in no tracked file outside version.txt, the release-please files, CHANGELOG.md and .planning/; the self test's fixture repository fails naming 'notes.md holds 1.2.3'"
    requirement: FRAME-06
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#release.single_version_source"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#release.single_version_source.next"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#release.single_version_source.self_test"
        status: pass
    human_judgment: false
  - id: D7
    description: "The README's one install line, run against build/ci/, verifies SHA256SUMS before tar and installs cores/cogmaster_libretro.dylib and info/cogmaster_libretro.info; with FLIP=1 the tampered copy is refused (shasum exit 1) and no cores/ directory exists; a README line with the shasum step removed makes the FLIP run exit 1"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -P tests/cmake/install_line.cmake (exit 0, install_line: installed)"
        status: pass
      - kind: e2e
        ref: "cmake -DREADME=README.md -DBASE_URL=file://$PWD/build/ci/ -DFLIP=1 -P tests/cmake/install_line.cmake (exit 0, checksum refused the tampered zip)"
        status: pass
      - kind: other
        ref: "cmake -DREADME=<README without the shasum step> -DBASE_URL=file://$PWD/build/ci/ -DFLIP=1 -P tests/cmake/install_line.cmake (exit 1, the line accepted a tampered zip)"
        status: pass
    human_judgment: false
  - id: D8
    description: "README is user-facing, 3,315 bytes (under 12,288), states no version number, and links docs/runner.md and docs/embedding.md; the whole ci workflow including docs.budgets passes"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.budgets"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset ci (exit 0: 77 tests, then package, then 4 release tests)"
        status: pass
    human_judgment: false
---

# Phase 1 Plan 07: Install, Packaging and README Install Line Summary

cogmaster now installs as a relocatable CMake package and pkg-config file, which C and C++ consumers and an
`add_subdirectory` embed all build against. The ci workflow packages three version-free zips per leg, each with
`LICENSE` and `THIRD-PARTY-NOTICES.md` at its root. The user README's one RetroArch install line runs `shasum -c`
before it extracts anything; a test runs that line against the built zip and against a copy with one byte flipped.

## What was built

- **Task 1** (`b259c39`): install, export and pkg-config rules (top level only), `cogmasterConfig.cmake.in`,
  `cogmaster.pc.in`; `tests/install/tests.cmake` with five `install`-labelled tests; consumer project
  (`main.c`, `main.cpp`, optional `consumer_pc` from the `.pc`); embed project.
- **Task 2** (`884e3c0`): `cmake/packaging.cmake`; package and release test presets added to the `ci` and
  `ci-msvc` workflows; `release.archive_contents`, `release.floor` and their self tests; the three
  `release.single_version_source` tests in `tests/policy/tests.cmake`.
- **Task 3** (`cabe26b`): rewritten `README.md`; `tests/cmake/install_line.cmake` with the flipped-byte
  control; the release archive's package and pkg-config use documented in `docs/embedding.md`.

## Verification (command, exit status)

- `cmake --workflow --preset ci`: exit 0 (77 tests, package, 4 release tests).
- `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake`: exit 0.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `ctest --test-dir build/ci -L '^install$' --show-only=json-v1 | jq -e '.tests | length == 5'`: exit 0.
- `cmake -DREADME=README.md "-DBASE_URL=file://$PWD/build/ci/" -P tests/cmake/install_line.cmake`: exit 0.
- `cmake -DREADME=README.md "-DBASE_URL=file://$PWD/build/ci/" -DFLIP=1 -P tests/cmake/install_line.cmake`: exit 0.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing documentation] docs/embedding.md described only the source-tree route**
- **Found during:** Task 3
- **Issue:** The README points to `docs/embedding.md` for CMake and pkg-config use, but the guide covered only
  `add_subdirectory`, and its list of options that default to off left out `COGMASTER_BUILD_LIBRETRO`.
- **Fix:** Added a "Release archive" paragraph with `find_package` and `pkg-config` examples and the
  SameMinorVersion rule; named `COGMASTER_BUILD_LIBRETRO` in the list. CLAUDE.md rule 5: behaviour and docs land together.
- **Files modified:** docs/embedding.md
- **Commit:** cabe26b

**2. [Plan scope] libretro/CMakeLists.txt not modified**
- The plan lists it, but plan 05 already installs only `cogmaster_libretro` into `cores/` and `info/` under
  component `libretro`, and never installs the RGB565 and GG test modules. `release.archive_contents` and the
  `grep -Eq 'rgb565|_gg'` check confirm this, so the file needed no change.

**3. [Plan wording] No shared library, so no SOVERSION**
- The plan asks for a SOVERSION "where the seed builds one". The seed builds a static library only, and so does
  cogmaster. `install.consumer.dll_crt` is the seed's MSVC DLL C runtime variant.

## Known Stubs

None.

## Threat Flags

None. The install line downloads only from the project's own release URL and runs `tar` only after
`shasum -c` succeeds. The tampered-copy control proves the order.

## Next

Plan 01-08. Plan 01-09 adds the two `install_line.cmake` runs to the macOS arm64 build leg in CI. FRAME-06, FRAME-07
and FRAME-08 are still listed in later plans of this phase, so none is ticked here. The README now links
`docs/runner.md` and `docs/embedding.md`, which settles plan 01-06's note that runner.md was not linked anywhere.

## Self-Check: PASSED
