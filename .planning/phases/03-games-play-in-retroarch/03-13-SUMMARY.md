---
phase: 03-games-play-in-retroarch
plan: "13"
status: complete
subsystem: runner-persistence
tags:
  - battery
  - atomic-replacement
  - signals
  - native-paths
requires:
  - phase: 03-12
    provides: Classified stable battery spans, write generations and transactional imports
provides:
  - Existing-directory .srm import with bounded reads and dedicated size refusal
  - Checked native same-directory replacement and dirty-generation acknowledgement
  - Unicode Windows command-line and host file paths
  - Recurring stage failure, signal and interrupted-write preservation tests
affects:
  - 03-14
  - 03-15
tech-stack:
  added: []
  patterns:
    - Keep file I/O outside the core and acknowledge only successful replacement
    - Exclusive native temporary creation preserves unrelated names
    - Compile persistence faults and signal hooks only into test artifacts
key-files:
  created:
    - runner/save.c
    - runner/save.h
    - runner/path.h
    - tests/runner/test_save.c
  modified:
    - runner/main.c
    - runner/args.c
    - runner/args.h
    - runner/exit_status.h
    - runner/probe.c
    - runner/stop.c
    - CMakeLists.txt
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/runner.md
    - .planning/preparation/HOSTS-AND-INTEGRATION.md
key-decisions:
  - Use same-directory POSIX rename or Windows MoveFileExW with replace-existing and write-through flags for both first creation and overwrite; never predelete the target.
  - Avoid ReplaceFileW because documented merge-failure cases can remove the old target name; use no cross-volume copy fallback.
  - Convert Windows wide argv to UTF-8 once and use wide native file APIs so both content and save paths retain Unicode characters.
requirements-completed: []
coverage:
  - id: bounded-raw-import
    description: Missing, empty, short and exact saves import with FF padding; oversize exits 4 unchanged; directory and read failures exit 1; unknown and unsupported persistence refuse before execution
    requirement: GAME-05
    verification:
      - kind: integration
        ref: runner.save.import, runner.save.imports and runner.save.excluded
        status: pass
    human_judgment: false
  - id: native-dirty-replacement
    description: First creation and overwrite preserve exact battery bytes, clean generations do no replacement, and spaces and Unicode retain the intended stem and target
    requirement: GAME-05
    verification:
      - kind: integration
        ref: runner.save.replacement and runner.save.import
        status: pass
    human_judgment: false
  - id: failed-save-preservation
    description: Create, short-write, flush, sync, close and replace faults preserve the previous target or missing target, retain dirty acknowledgement and permit retry without touching unrelated temporaries
    requirement: GAME-05
    verification:
      - kind: unit
        ref: runner.save.failures and runner.save.native_error
        status: pass
      - kind: integration
        ref: runner.save.process.create, write, flush, sync, close and replace
        status: pass
    human_judgment: false
  - id: signal-and-interruption
    description: SIGTERM and SIGINT finish admitted frames and flush outside handlers, clean signals skip replacement, failed saves exit 1, and abrupt interruption leaves the old target and complete private temporary
    requirement: GAME-05
    verification:
      - kind: integration
        ref: runner.save.process.term, int, int_write, normal_write and interrupt
        status: pass
      - kind: unit
        ref: runner.save.clean_sigint and runner.save.clean_sigterm
        status: pass
    human_judgment: false
actuals:
  tokens: 13808
  tasks: 2
  commits: 4
commits: 4
plan_head_before: ca6b3707c4cf6f00e8687bb4f79899fc5f353a50
plan_head_after: f8e8597b13ff64ffef2c9099b78429dc1212c3fa
duration: 10min
completed: 2026-10-20
---

# Phase 03 Plan 13: Checked runner battery files Summary

The runner imports bounded raw .srm files and persists dirty generations through checked native replacement, including graceful signals and tested failure preservation.

## Tasks and Commits

1. Import and replace .srm files: RED `cc288d4`, GREEN `aa316cd`.
2. Finish signals and prove failure preservation: RED `bcece70`, GREEN `f8e8597`.

## Results

`--save-dir` requires content and an existing directory, strips only the last ROM extension and uses `.srm`.
Missing files begin with FF; shorter data pads with FF; the host reads at most the exact span plus one byte.
Oversize imports exit 4 unchanged, other I/O errors exit 1, and unknown or unsupported persistence exits 77
with distinct messages before a frame runs. Known nonbattery content has no save file.

Only dirty generations reach exclusive same-directory temporary creation. Full writes, flush, native sync
where supported, close and replacement are checked before acknowledgement. POSIX uses `rename`; Windows uses
`MoveFileExW` without a copy fallback. Windows arguments enter through `wmain` and retain Unicode through
wide native opens. No file, device or clock code entered the deterministic core and no dependency was added.

Nineteen recurring save cases cover native first-create/overwrite, all import classes, clean generations,
Unicode names, independent old-byte equality, dirty retry, own-temporary cleanup, unrelated-temporary survival,
native directory replacement failure, clean and dirty SIGINT/SIGTERM, normal-exit failure and interruption
after a complete temporary is closed but before publication. All large test buffers are static. Test-only
fault selectors and early exit are compiled out of the installed runner; uninstrumented helpers are shared
between the test runners. Both exact lane inventories include every new case.

Atomic visibility depends on native filesystem rename semantics. The containing directory is not synced,
WASI omits the native sync request, and no universal power-loss or multiwriter protocol is claimed. Abrupt
termination may leave this process's temporary; later runs never sweep arbitrary temporaries. Raw files do
not bind identity and same-stem content shares a name. Interval saves, SIGUSR1 and EEPROM remain deferred.

GAME-05 remains unchecked because this plan completes its runner slice; libretro persistence and the broader
continuation evidence still belong to the remaining host plans.

## Verification

- `cmake --workflow --preset ci` — exit 0 after task 1, exit 0 again at the tracer feedback gate, and exit 0
  after task 2: 326 main tests and four release checks in the final run.
- `ctest --test-dir build/ci -R '^runner.save.' --output-on-failure` — exit 0, all 19 save cases executed.
- `git diff --check` — exit 0.
- `sh scripts/hygiene.sh --tree` — exit 0; normal commit hooks also passed.
- clang-format 21 dry-run over the changed C/header files — exit 0.
- `ctest --test-dir build/ci -R '^(docs.budgets|fixtures.e2e.budget)$' -V` — exit 0; the registered docs budget
  check reported 449 repository-check lines / 5,297 core lines, strictly below one tenth.
- `ctest --test-dir build/ci -R '^policy.e2e_budget$' -V` — exit 0; unchanged hosted helpers total 300 lines.
- Summary coverage classification — exit 0; all four deliverables automatically covered, no schema errors.

Hosted run URL: none for this local execution. The new cases are registered in the existing native Windows
ci-msvc and wasm32 inventories; those platforms were not executed locally and need the phase's hosted CI.
An optional wasm32 build probe found no configured `build.ninja`; it changed no source and was not a required
task verification. No committed logs or owner verification were introduced.

## TDD Evidence

- Task 1: `ctest --test-dir build/ci -R '^runner.save.import$' --output-on-failure --output-junit ...`
  exited 8: expected runner exit 0 but got exit 2, unknown option. Its actual JUnit report was classified
  `RED_EVIDENCE_OK`; semantic review confirmed the target executed the missing save option. GREEN implemented
  the host path and passed the complete workflow before commit.
- Task 2: the corresponding `runner.save.failures` JUnit run exited 8: the test-only fault selector was
  intentionally unhandled, so flush incorrectly succeeded, acknowledged and created the target. The actual
  report was classified `RED_EVIDENCE_OK`; semantic review confirmed failures of the planned return-status,
  dirty-generation and missing-target assertions. GREEN implemented the compiled test faults and expanded
  preservation and signal coverage. Evidence files live only in ignored build output; no log was committed.

Global TDD mode was false; both tasks still followed RED/GREEN because their task attributes requested TDD.
No separate refactor commit was needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Preserve Unicode content paths on Windows**
- Found during task 1: narrow CRT argv and `fopen` could lose the intended content or save filename.
- Added `runner/path.h`, the Windows wide entry point and wide opens in `runner/probe.c` and main's existing
  file entry points. These companion files are necessary for the planned non-ASCII path contract.
- Verified by the recurring copied-content and save-directory Unicode case; committed in `aa316cd`.

## Decisions Made

Windows `MoveFileExW` supports first creation and replacement in the same directory. `ReplaceFileW` was
avoided because its documented failure results include loss of the old target name; its write-through flag
is also unsupported. The native move path fits the plan's allowed helper choice and adds no service or lock.
Sources checked during execution: Microsoft Learn's MoveFileExW, ReplaceFileW and fopen/_wfopen documentation.

The exit table keeps the existing short alphabetic status-name convention (`save` for exit 4), so its existing
docs equality gate continues to check the new line without expanding tooling.

## Known Stubs

None. Changed production and test files were scanned for TODO, FIXME, placeholder and unavailable markers;
no incomplete data path prevents this plan's goal. No additional security surface beyond the planned runner
file access was introduced.

## Readiness

The runner persistence slice is ready for the libretro export and continuation work in plan 03-14. No blocker,
authentication gate or owner setup remains. Runtime: Codex. Model: GPT-6.

## Self-Check: PASSED

All four new files and this summary exist. All four recorded task commits are ancestors of HEAD. The
persisted Git ledger measures four commits between the recorded base and implementation head; all
implementation changes are committed, no tracked files were deleted and no implementation file is untracked.
