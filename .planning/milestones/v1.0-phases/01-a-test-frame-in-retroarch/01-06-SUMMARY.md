---
phase: 01-a-test-frame-in-retroarch
plan: 06
subsystem: tests
tags:
  - abi
  - nm
  - header
  - policy
  - budgets
requires:
  - "01-01: cogmaster_add_test, the inventory check, tests/check.h and tests/cmake/expect_fail.cmake"
  - "01-05: libretro/cogmaster_libretro.info.in and THIRD-PARTY-NOTICES.md, read by policy.notices"
provides:
  - "tests/cmake/abi.cmake: nm -P modes undefined, globals and symbols with per-OS helper rows"
  - "tests/cmake/api_version.cmake: every header name must appear in tests/api/vK.c with K at most COGMASTER_API_VERSION"
  - "tests/api/v1.c (target cogmaster-test-api-v1): the API version 1 consumer"
  - "Object targets cogmaster_header_c17 and cogmaster_header_cxx17, built only by their tests; C++ is enabled only inside tests/abi/tests.cmake"
  - "Fixture libraries cogmaster-abi-libc and cogmaster-abi-global"
  - "tests/cmake/budgets.cmake, no_skip.cmake, api_names.cmake (with -DOTHER), notices.cmake"
  - "CTests abi.undefined, abi.undefined.self_test, abi.globals, abi.globals.self_test, abi.symbols, abi.header.c17, abi.header.cxx17, abi.api_v1, abi.api_version, abi.api_version.self_test, docs.budgets, docs.api_names, docs.api_names.self_test, policy.no_skip, policy.no_skip.self_test, policy.notices"
affects:
  - "the nofp preset (adds abi.nofp_fixture to tests/abi/tests.cmake)"
  - "every later header change, which must keep tests/api/v1.c covering each name or add v2.c with a bump"
  - "every later public name, which must appear in docs/embedding.md"
  - "any src/ change that adds a writable global, a libc import or a non-cogmaster_ external symbol"
actuals:
  tokens: 9900
  tasks: 2
  commits: 2
plan_head_before: 89644a613e1c5cb21728dc50bdf0e03786f7f747
plan_head_after: a78dce3a71238000881582aa53ee26463e19e799
tech-stack:
  added: []
  patterns:
    - "Each binary check runs nm -P through a CMake -P script on the built archive; a fixture archive built with the core's own flags is its negative control"
    - "A self test wraps the real check in expect_fail.cmake with an exact exit code and a regex naming the fixture's symbol or file"
    - "The header is compiled alone as C17 and C++17 by tests that build an EXCLUDE_FROM_ALL object target under a build_tree resource lock"
key-files:
  created:
    - tests/abi/tests.cmake
    - tests/abi/global_fixture.c
    - tests/abi/libc_fixture.c
    - tests/cmake/abi.cmake
    - tests/header/header.c
    - tests/header/header.cpp
    - tests/api/v1.c
    - tests/cmake/api_version.cmake
    - tests/policy/tests.cmake
    - tests/cmake/budgets.cmake
    - tests/cmake/no_skip.cmake
    - tests/cmake/api_names.cmake
    - tests/cmake/notices.cmake
  modified:
    - tests/tests.cmake
    - tests/inventory/ci.txt
decisions:
  - "The API version rule follows the seed: every name the header declares must appear in tests/api/vK.c with K at most COGMASTER_API_VERSION. Once v1.c is frozen at the first release, a new declaration needs v2.c and with it API version 2. No git tag is read, so the check works in a shallow CI checkout."
  - "All budgets are strict: a file fails at or over its budget (24,576, 12,288, 40,960 bytes)"
  - "docs.api_names ignores header names that end in _, because they are comment wildcards such as COGMASTER_STOP_*. The guide may cite names from cmake/version.h.in and CMakeLists.txt (passed as -DOTHER) without the header declaring them."
  - "enable_language(CXX) lives in tests/abi/tests.cmake, so a build with tests off or an embedding build never needs a C++ compiler"
requirements-completed: []
coverage:
  - id: D1
    description: "abi.undefined: the Release archive imports only memcpy, memmove, memset, memcmp and the per-OS helper rows; abi.undefined.self_test builds a fixture that calls abort and passes only when the check exits 1 naming abort"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.undefined"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.undefined.self_test"
        status: pass
      - kind: other
        ref: "nm -u build/ci/libcogmaster.a | grep -Eq 'clock|time|gettimeofday|malloc|free|printf|fopen' (exit 1, no match)"
        status: pass
    human_judgment: false
  - id: D2
    description: "abi.globals: the archive has no B, b, D, d or C symbol; abi.globals.self_test passes only when the check exits 1 naming the fixture's counter"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.globals"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.globals.self_test"
        status: pass
    human_judgment: false
  - id: D3
    description: "abi.symbols: every defined external symbol is a cogmaster_ function the header declares or an internal cogmaster__ name, and every declared function is defined"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.symbols"
        status: pass
      - kind: other
        ref: "cmake -DMODE=symbols -DHEADER=build/ci/tests/abi/added_function.h ... -P tests/cmake/abi.cmake (exit 1, declared, not defined: cogmaster_added)"
        status: pass
    human_judgment: false
  - id: D4
    description: "abi.header.c17 and abi.header.cxx17 compile include/cogmaster.h alone with -std=c17 or -std=c++17 and -Wall -Wextra -Wpedantic -Werror (MSVC /W4 /WX with /TC or /TP), checking every struct's size tag and every function's signature"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.header.c17"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.header.cxx17"
        status: pass
    human_judgment: false
  - id: D5
    description: "abi.api_v1 creates, loads the test card, reads info, runs frames, copies the frame, reads the fault, resets and destroys through API version 1; abi.api_version requires every header name in tests/api/vK.c, and its self test with an added function and no bump fails naming cogmaster_added"
    requirement: FRAME-04
    verification:
      - kind: unit
        ref: "tests/api/v1.c#abi.api_v1"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.api_version"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.api_version.self_test"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs.budgets: include/cogmaster.h (3,588 bytes) is under 24,576, README.md (1,549) under 12,288, and each docs/*.md under 40,960; a README of exactly 12,288 bytes fails"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.budgets"
        status: pass
      - kind: other
        ref: "cmake -DROOT=<fixture with a 12288-byte README.md> -P tests/cmake/budgets.cmake (exit 1, README.md: 12288 bytes, budget 12288)"
        status: pass
    human_judgment: false
  - id: D7
    description: "policy.no_skip: no test registration under tests/, cmake/ or the top CMakeLists.txt uses a skip return code, a skip regex or the disabled property; its self test fails naming CMakeLists.txt:1"
    requirement: FRAME-05
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#policy.no_skip"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#policy.no_skip.self_test"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs.api_names: every public name in the header appears in docs/embedding.md and the guide names no cogmaster_ or COGMASTER_ symbol that is undeclared; its self test names both cogmaster_added and cogmaster_removed"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.api_names"
        status: pass
      - kind: integration
        ref: "tests/policy/tests.cmake#docs.api_names.self_test"
        status: pass
    human_judgment: false
  - id: D9
    description: "policy.notices: every SPDX line in tracked C files is MIT or MIT AND an admitted licence, every path a notice names exists, and the libretro .info licence is MIT"
    requirement: FRAME-04
    verification:
      - kind: integration
        ref: "tests/policy/tests.cmake#policy.notices"
        status: pass
    human_judgment: false
duration: 6min
completed: 2026-10-19
status: complete
---

# Phase 1 Plan 06: ABI, header, budget and policy gates Summary

**`nm` now checks the Release `libcogmaster.a` on three points: it imports only the four memory functions (plus Apple's `bzero`), it has no writable data, and it exports only the `cogmaster_` names the header declares. A fixture archive is the negative control for each check. The header compiles alone as C17 and as C++17 with warnings fatal. A version-1 consumer drives the whole API. Byte budgets, the no-self-skip rule, guide/header name agreement and the notices rule all run in `ci`, each with a control that must fail.**

## Performance

- **Duration:** about 6 min
- **Started:** 2026-10-19T21:47Z
- **Completed:** 2026-10-19T21:53Z
- **Tasks:** 2
- **Files created:** 13, modified: 2

## Accomplishments

- **Binary gates.** `tests/cmake/abi.cmake` is the seed script with renames. Its helper rows allow Darwin `bzero`/`__bzero`, the x86-64 Linux `_GLOBAL_OFFSET_TABLE_` and the 32-bit PowerPC libgcc integer routines. On this host, `nm -u` lists `_bzero`, `_memcpy` and internal `cogmaster__` cross-object references, and nothing else.
- **Header in two languages.** `header.c` and `header.cpp` assert that each of the seven boundary structs starts with `size`, that the status is 32 bits and that the API version is 1. They also bind all nine functions to typed pointers. The C++ file zero-initialises a struct with `{}`.
- **API v1.** `tests/api/v1.c` names every constant, type and field of the header, plus `cogmaster_version.h`. `abi.api_version` reports none missing.
- **Policy.** `docs.budgets` prints each file's size and budget. `policy.no_skip` builds its forbidden property names from pieces. `docs.api_names` found no gap in `docs/embedding.md` once the scope was settled (see Decisions), so the guide is unchanged. `policy.notices` reads `libretro/cogmaster_libretro.info.in`.

## Task Commits

1. **Task 1: nm-based ABI gates, API v1 consumer and the header as C17 and C++17**: `3e7f04c` (test)
2. **Task 2: Byte budgets, no-self-skip policy, guide names and notices**: `a78dce3` (test)

## Evidence (command, exit status)

- `cmake --workflow --preset ci`: exit 0, 69 tests (63 after Task 1).
- `ctest --test-dir build/ci --no-tests=error -L '^abi$' --output-on-failure && ctest --test-dir build/ci -L '^abi$' --show-only=json-v1 | jq -e '.tests | length == 10'`: exit 0.
- `ctest --test-dir build/ci --no-tests=error -L '^policy$' --output-on-failure && ctest --test-dir build/ci -L '^policy$' --show-only=json-v1 | jq -e '.tests | length == 6'`: exit 0.
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0.
- `nm -u build/ci/libcogmaster.a | grep -Eq 'clock|time|gettimeofday|malloc|free|printf|fopen'`: exit 1 (no match).
- `ninja -C build/ci -t commands cogmaster_header_c17 cogmaster_header_cxx17`: the commands carry `-std=c17` / `-std=c++17` with `-Wall -Wextra -Wpedantic -Werror` (exit 0).
- One-off: `abi.cmake` in mode `symbols` against `build/ci/tests/abi/added_function.h` exited 1 and printed `declared, not defined: cogmaster_added`.
- One-off: `budgets.cmake` on a fixture root with a 12,288-byte README exited 1 and printed `README.md: 12288 bytes, budget 12288`.
- `wc -c < include/cogmaster.h`: 3588. `wc -c < README.md`: 1549.
- `sh scripts/hygiene.sh --tree`: exit 0.
- `grep -rEn '\b(D-[0-9]{2}|FRAME-[0-9]{2})\b' tests/abi tests/header tests/api tests/policy tests/cmake/{abi,api_version,budgets,no_skip,api_names,notices}.cmake`: exit 1, nothing printed.

## Deviations from Plan

### Interpretations

**1. The API version rule is the seed's, not a tag comparison**
- **Found during:** Task 1
- **Issue:** The plan describes the rule as comparing the header with the last release tag's header, and calls `api_version.cmake` a seed copy. The seed does something else: it requires every header name to appear in `tests/api/vK.c` with K at most the API version.
- **Resolution:** The seed was kept. Once `v1.c` is frozen at release, a new declaration fails until `v2.c` exists, and `v2.c` only counts once `COGMASTER_API_VERSION` is 2. This enforces the same rule without reading git tags, which a shallow CI checkout lacks. The self test adds `cogmaster_added` with no bump and expects `in no tests/api/vK.c with K <= 1: cogmaster_added`.
- **Commit:** `3e7f04c`

**2. `docs.api_names` scope**
- **Found during:** Task 2
- **Issue:** The seed script flagged `COGMASTER_STOP_` and `COGMASTER_RESET_`, which are comment wildcards in the header. It also flagged five names the guide correctly cites from elsewhere: `COGMASTER_BUILD_RUNNER`/`_TESTS` (CMake options) and `COGMASTER_VERSION_MAJOR`/`_MINOR`/`_PATCH` (the generated version header).
- **Fix:** Header names ending in `_` are dropped. A new `-DOTHER` list (`cmake/version.h.in`, `CMakeLists.txt`) holds names the guide may cite, though the header need not declare them. The self test still fails, naming both fixture names. `docs/embedding.md` needed no edit.
- **Files modified:** `tests/cmake/api_names.cmake`, `tests/policy/tests.cmake`
- **Commit:** `a78dce3`

**3. Strict budgets everywhere**
- The seed failed only above a budget. The check now fails at or over the budget for every file. This is the plan's reading of "under", assumption A-06-1.

**4. C++ enabled in the test file**
- `enable_language(CXX)` sits in `tests/abi/tests.cmake`, not in `CMakeLists.txt` (the seed has it there), because CMakeLists.txt is not in this plan's file list. The effect is the same: only builds with tests on need a C++ compiler.

**5. Seed registrations not carried over**
- The seed's policy file also registers the `release.single_version_source*` tests. They belong to a later plan and are not registered here. Likewise `abi.nofp_fixture` arrives with the `nofp` preset in plan 08.

## Requirement status

FRAME-04 and FRAME-05 are not marked complete here. Plans 08, 09, 11 and 12 still carry them: the `nofp` lane and `hygiene.no_float`, the other FRAME-05 gates, and the per-leg CI runs. Their REQUIREMENTS.md boxes stay open.

## Known Stubs

None.

## Issues Encountered

- **Repository-check code size.** `tests/cmake/*.cmake` now holds 680 lines, against 1,076 lines in `src/`. Rule 7 caps check code at a tenth of `src/` lines from phase 3 onward. That holds only if `src/` grows about tenfold by then, or the checks are trimmed. This is logged for the phase-3 discuss step.
- **Untested here:** the MSVC (`/TC`, `/TP`, `llvm-nm` on COFF) and Linux helper rows. They run on the `ci-msvc` and Linux legs in plan 11's CI run.

## Next Phase Readiness

Plan 08 can add `abi.nofp_fixture` to `tests/abi/tests.cmake` under the `nofp` preset. Any later `src/` change that adds a global, a libc call or a stray external name now fails `ci`.

## Self-Check: PASSED

- All 13 created files exist on disk.
- Commits 3e7f04c and a78dce3 are ancestors of HEAD.
