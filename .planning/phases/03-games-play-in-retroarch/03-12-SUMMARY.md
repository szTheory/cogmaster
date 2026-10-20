---
phase: 03-games-play-in-retroarch
plan: "12"
status: complete
subsystem: battery-media
tags:
  - persistence
  - mapper
  - transactional-import
  - fuzz
requires:
  - phase: 03-11
    provides: Immutable cartridge classification and owned battery identity
provides:
  - Fixed exact supported battery spans with explicit persistence knowledge
  - Generation increments for every mapped persistent write including same-value writes
  - Transactional bounded raw imports with FF padding and dedicated oversize status
  - Owned guest continuation and recurring sanitizer-compatible save fuzz replay
affects:
  - 03-13
  - 03-14
tech-stack:
  added: []
  patterns:
    - Physical mapper translation precedes persistent extent accounting
    - Validate all raw import arguments before copying or padding
    - Whole-state equality for rejected and repeated imports
key-files:
  created:
    - src/battery.c
    - tests/mapper/test_battery.c
    - tests/api/v6.c
    - tests/api/v7.c
    - tests/fuzz/fuzz_save.c
  modified:
    - include/cogmaster.h
    - src/cart.c
    - src/internal.h
    - src/mapper.c
    - src/state.c
    - CMakeLists.txt
    - tests/abi/tests.cmake
    - tests/mapper/tests.cmake
    - tests/fuzz/tests.cmake
    - tests/fuzz/seeds.c
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/fuzz.txt
    - docs/embedding.md
    - docs/hardware/cartridges.md
    - docs/testing.md
key-decisions:
  - Append battery queries as API 6 and raw import as API 7 because earlier plans already added API versions 3 through 5; retain existing layouts and numeric values.
  - Known-none imports return no-content; unknown or unsupported persistence returns unsupported-board; oversized supported imports return save-size before reading the source.
  - Preserve physical RAM decoding independently of the persistent extent, including volatile physical offsets outside an exported prefix.
requirements-completed: []
coverage:
  - id: fixed-classified-spans
    description: Exact supported 8/16/32 KiB spans, independent physical aliases, fresh FF, stable pointers through enable, frames and both resets; unknown, none and unsupported expose no span
    requirement: GAME-05
    verification:
      - kind: unit
        ref: battery.spans and battery.excluded
        status: pass
      - kind: integration
        ref: battery.fixture
        status: pass
    human_judgment: false
  - id: mapped-write-generation
    description: Same-value writes count individually; reads, disabled writes, registers, volatile offsets, resets and imports do not increment; internal states visit and validate generation
    requirement: GAME-05
    verification:
      - kind: unit
        ref: battery.generation, battery.excluded and core.state.record_sweep
        status: pass
    human_judgment: false
  - id: bounded-transactional-import
    description: Empty, one-byte, short and exact imports copy and FF-pad only the persistent extent; oversize, SIZE_MAX and invalid arguments preserve the entire state; repeat imports are generation-neutral
    requirement: GAME-05
    verification:
      - kind: unit
        ref: battery.import and abi.api_v7
        status: pass
      - kind: integration
        ref: fuzz.regress.save under CI and Apple Clang ASan plus UBSan
        status: pass
    human_judgment: false
  - id: raw-wrong-data-continuation
    description: Production-classified owned fixture accepts two same-size raw saves and reports their independently expected previous and next counters
    requirement: GAME-05
    verification:
      - kind: integration
        ref: battery.continuation
        status: pass
    human_judgment: false
  - id: save-fuzz-entry
    description: Existing nightly fuzz framework includes raw save import with edge lengths, all persistence classes, exact bytes, FF suffixes and refusal equality
    requirement: GAME-05
    verification:
      - kind: integration
        ref: fuzz.libfuzzer.save with installed libFuzzer and Apple Clang sanitizer runtime
        status: pass
    human_judgment: false
actuals:
  tokens: 11856
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 195a2b37130f12c9e9fd2a2fcd903dc2147929f9
plan_head_after: 53573de3b6c7f42c8455ac5d2eef48fe2ab8271d
duration: 12min
completed: 2026-10-20
---

# Phase 03 Plan 12: Exact battery media and transactional import Summary

Classified battery memory now has a stable public span, per-write generations and validated FF-padded raw imports without core file I/O.

## Tasks and Commits

1. Expose classified media and count mapped writes: RED `8f4de87`, GREEN `c8d7a94`.
2. Import raw saves transactionally and replay hostile lengths: RED `3a5b8db`, GREEN `53573de`.

## Results

`cogmaster_battery` carries `size`, writable `data`, `length`, `knowledge` and uint64 `generation`.
Knowledge values are UNKNOWN 0, NONE 1, KNOWN 2 and UNSUPPORTED 3. Only supported classified battery
media exposes bytes; unknown images and Moldorian's unsupported 24576-byte declaration expose NULL
and zero length. The supported pointer exists before RAM enable and remains fixed through frames and
both reset kinds. The separate declared persistent extent never replaces physical mapper decoding.

Mapped writes translate to a physical offset before counting persistence. Two writes of FF to an
already-FF byte increment generation twice. Disabled cartridge writes, register writes, reads and
volatile writes do not. Both resets preserve bytes and generation. State format and behavior revision
10 include generation; validation permits only zero when no supported persistence exists. Public
output size tags reject undersized queries without mutation and preserve unknown output tails.

`cogmaster_import_battery` accepts lengths 0 through the supported extent, then copies with memmove
and fills only its remaining suffix with FF. NULL with zero length represents an empty save. Oversize
and SIZE_MAX reject with appended `COGMASTER_STATUS_ERR_SAVE_SIZE` 9 before any read or mutation.
All rejection paths preserve byte-identical full internal states. Repeated import is generation-neutral;
overlapping sources in the exposed span are supported. Unit setup also proves that physical RAM outside
a shorter persistent prefix is neither aliased into the prefix nor overwritten by import.

The actual owned fixture uses production identity lookup. Raw saves containing previous counters 7
and 19 are accepted at identical lengths; guest execution reports 7/8 and 19/20 respectively within
eight frames. Raw bytes have no game identity and are deliberately not rejected for differing contents.

Exact native/WASI inventories include the API and battery cases plus `fuzz.regress.save`; the nightly
inventory includes `fuzz.libfuzzer.save`. Generated save seeds exercise lengths 0, 1, 32767, 32768 and
32769, plus none/unknown/unsupported classes. No ROM or save bytes were committed. All large state
buffers are static for the WASI stack limit. Repository and hosted-test budgets continue to pass.

GAME-05 remains pending overall: runner replacement, signal flushing, frontend memory consumers and
fresh-process file continuation belong to plans 03-13 and 03-14. The APIs here have public host tests
and a production-classified guest consumer; no host persistence is claimed by this library-only plan.

## Command Evidence

Commands ran from the repository root. Their outputs remained in ignored build directories. No hosted
run was dispatched for this plan; hosted run URL is not applicable to these local checks.

- `ctest --test-dir build/ci -R '^battery.generation$' --output-on-failure --output-junit battery-red.xml` — exit 8, planned missing generation visitor assertion.
- `node <GSD_TOOLS> check tdd-red-evidence build/ci/battery-red.json --raw` — exit 0, RED_EVIDENCE_OK.
- `ctest --test-dir build/ci -R '^battery.import$' --output-on-failure --output-junit import-red.xml` — exit 8, planned import return and padding assertions before the import API existed.
- `node <GSD_TOOLS> check tdd-red-evidence build/ci/import-red.json --raw` — exit 0, RED_EVIDENCE_OK.
- `cmake --workflow --preset ci` — exit 0 after each task; final 307 tests and 4 release checks passed. The tracer reran this workflow successfully before task 2 expansion.
- `cmake --workflow --preset hygiene` — exit 0; final 32 checks passed, including formatting, analyzer, privacy, core symbol surface and proportional budgets.
- `cmake --preset asan -B build/asan-apple -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++` and `cmake --build build/asan-apple` — exit 0.
- `ctest --test-dir build/asan-apple -R '^(battery|abi.api_v[67]|fuzz.regress.save)' --output-on-failure` — exit 0; all 9 cases passed under ASan plus UBSan after final changes.
- `cmake --preset fuzz -B build/fuzz-apple -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++` — exit 0; the exact three-target fuzz inventory registered. Building compiled the instrumented sources, then failed linking because Xcode omits libFuzzer.
- `/usr/bin/clang -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all build/fuzz-apple/CMakeFiles/cogmaster-fuzz-save.dir/tests/fuzz/fuzz_save.c.o build/fuzz-apple/libcogmaster.a /opt/homebrew/opt/llvm@21/lib/clang/21/lib/darwin/libclang_rt.fuzzer_osx.a -lc++ -o build/fuzz-apple/tests/cogmaster-fuzz-save` — exit 0; links the installed libFuzzer archive with the working Apple sanitizer runtime, without a dependency download or repository change.
- `ctest --test-dir build/fuzz-apple -R '^fuzz.libfuzzer.save$' --output-on-failure` — exit 0; timed save fuzzing passed in 61.17 seconds.
- `git diff --check` — exit 0.

## TDD Gate Compliance

Both tasks committed RED before GREEN. The first actual CTest JUnit report demonstrates the missing
generation state visitor. For task 2, a test-local old-API compatibility adapter explicitly returned
ERR_ARGUMENT so planned import assertions executed rather than failing compilation on an absent symbol.
The report shows accepted-length and FF-padding assertion failures. Both unmodified reports passed
the RED evidence classifier; semantic inspection confirmed executed assertions and successful discovery.
The old-API test scaffolding was removed in GREEN; final tests call the production APIs. Global runtime
TDD gating and auto-advance were disabled. No refactor commit was necessary.

## Deviations from Plan

- Added versioned API 6/7 tests and their ABI registrations rather than placing new names in the already
  released API 3 contract. Earlier phase plans had advanced the public header to API 5.
- Retained GAME-05 as pending until host persistence and process continuation are implemented; marking
  it complete for span/import primitives would overstate the requirement.
- Local Homebrew LLVM 21 ASan deadlocked before main on macOS 26.6.2, matching the existing STATE concern.
  A sampled stack showed recursive sanitizer initialization through dyld's Swift shared-cache iterator
  and malloc. The affected checks were terminated (exit 143). The installed Apple runtime passed the
  focused sanitizer checks; the installed Homebrew libFuzzer archive supplied Xcode's missing archive.
  Hosted Linux compiler policy and the existing report-only nightly framework were unchanged.

## Deferred Issues

No implementation defect, stub, skipped planned verification, authentication gate or package install.
The existing host LLVM startup issue remains external to this plan; successful sanitizer replay and
timed fuzz evidence use the installed working runtime. Host flush acknowledgement and no-redundant-file
replacement are deliberately verified by the subsequent persistence plans, not simulated here.

## Self-Check: PASSED

All five created implementation/test artifacts exist, and all four task commits are ancestors of HEAD.
Final CI, hygiene and focused sanitizer checks passed after the final source edits; the production save
fuzz entry also completed its timed run. No generated files are untracked. The measured ledger spans
exactly four task commits; its recorded base and ending HEAD match git history.
