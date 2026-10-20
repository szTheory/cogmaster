---
phase: 03-games-play-in-retroarch
plan: "14"
status: complete
subsystem: battery-continuation
tags:
  - battery
  - libretro
  - save-memory
  - reset
requires:
  - phase: 03-12
    provides: Exact classified battery spans, generations and transactional imports
  - phase: 03-13
    provides: Atomic native runner persistence and deterministic signal hooks
provides:
  - Numeric battery continuation across separate runner processes after normal exit, SIGTERM and SIGINT
  - Missing-save, exact-size wrong-save and unchanged oversize controls
  - Production libretro SAVE_RAM export before RAM enable, stable through frames and Restart
  - Real classified media reset and generation evidence on SMS2 and native Game Gear
affects:
  - 03-15
  - 03-18
tech-stack:
  added: []
  patterns:
    - Reuse the classified owned fixture without changing its pinned bytes
    - Frontends restore through the exposed writable span and own all filesystem I/O
    - Keep in-process frontend checks separate from the hosted RetroArch code budget
key-files:
  created:
    - tests/libretro/battery.h
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/mapper/test_battery.c
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/conformance.md
    - docs/embedding.md
    - docs/testing.md
key-decisions:
  - Keep the existing fixture identity; its CRC-checked mailbox already reports the previous and incremented counter.
  - Delegate SAVE_RAM directly to the production battery query, guarded by the adapter loaded lifetime.
  - Keep libretro_saves false and experimental status until their separate hosted proof contracts are met.
requirements-completed:
  - GAME-05
  - GAME-04
coverage:
  - id: fresh-process-continuation
    description: Separate children persist and continue 0 to 1 to 2 after normal exit, SIGTERM and SIGINT, checking full 32768-byte saves and no leaked temporary
    requirement: GAME-05
    verification:
      - kind: integration
        ref: runner.save.continuation.normal, runner.save.continuation.term and runner.save.continuation.int
        status: pass
    human_judgment: false
  - id: independent-save-controls
    description: Missing save reports 0100, a same-size save with counter 41 reports 2A29, and oversize exits 4 without changing the target digest
    requirement: GAME-05
    verification:
      - kind: integration
        ref: runner.save.continuation.normal, runner.save.continuation.term and runner.save.continuation.int
        status: pass
    human_judgment: false
  - id: frontend-save-memory
    description: Real classified SAVE_RAM exists before enable, remains stable through frames and Restart, continues restored bytes across unload and reload, and invalidates callbacks outside the loaded lifetime
    requirement: GAME-05
    verification:
      - kind: integration
        ref: libretro.battery.sms2 and libretro.battery.gg
        status: pass
      - kind: unit
        ref: battery.excluded and mapper.codemasters.ram
        status: pass
    human_judgment: false
  - id: real-battery-reset-preservation
    description: SOFT and POWER preserve pointer, length, all bytes and generation on SMS2 and native GG; import and direct restoration are generation-neutral and guest continuation increments once
    requirement: GAME-04
    verification:
      - kind: integration
        ref: battery.continuation, libretro.battery.sms2 and libretro.battery.gg
        status: pass
    human_judgment: false
actuals:
  tokens: 4992
  tasks: 2
  commits: 3
commits: 3
plan_head_before: a45ccb7c6460e8d58c2fc1becde9b55d721bbdf1
plan_head_after: 164cef2bd3eec146141642ceac4fe42eeec92d6b
duration: 5min
completed: 2026-10-20
---

# Phase 3 Plan 14: Battery Continuation and Frontend Memory Summary

**Fresh runner processes continue classified battery counters after exit and signals; libretro exports the same stable writable span through Restart.**

## Performance

- Started: 2026-10-20T21:44:33Z
- Completed: 2026-10-20T21:49:05Z
- Tasks: 2
- Files changed: 11
- Runtime: Codex; model: GPT-6
- Actual tokens: 19,968 realized diff bytes divided by four; commits measured from the persisted plan ledger.

## Accomplishments

The owned battery image remains byte-identical and goes through production hash classification. New
process tests independently predict mailbox words and compare every saved byte, including the FF tail.
Each continuation launches a separate uninstrumented production runner. Signal cases reuse the existing
test-only synchronous raise before frame 3; the admitted frame completes and persistence runs outside
the handler. WASI uses emulated C signals rather than claiming external asynchronous signal support.

The production adapter now returns the exact battery query pointer and size for SAVE_RAM. Loaded-lifetime
guards suppress stale memory after unload, failed reload and deinitialization. SMS and native GG frontend
tests restore counter 7, observe 8, Restart to 9, and restore again after reload. Unknown identity and
no-content controls expose NULL/0; existing battery-query and mapper tests retain unsupported/volatile
controls without needing commercial bytes or a fixture-only loader path.

The real-media library test now exercises both reset kinds on both machines, compares the whole battery
span before and after reset, and verifies generation-neutral imports and direct host restoration. No
adapter filesystem code, new dependency, hosted-save claim or interop claim was added.

## Task Commits

1. Task 1 — `aad8b0f`: prove fresh-process battery continuation.
2. Task 2 RED — `60e9fcb`: require stable frontend battery memory.
3. Task 2 GREEN — `164cef2`: expose classified frontend battery memory and document reset contracts.

## Verification

All commands ran from the repository root. No hosted run was dispatched for this plan; run URL is not
applicable to these local checks. Platform execution belongs to the phase CI run.

| Command | Exit | Evidence |
|---|---|---|
| `cmake --workflow --preset ci` | 0 | Task 1 and tracer rerun each passed 329 tests plus 4 packaging checks; final run passed 331 plus 4 |
| `cmake --workflow --preset hygiene` | 0 | 32 checks passed, including format, analyzer, repository ratio and hosted-code budget |
| `ctest --test-dir build/ci -R '^runner.save.continuation\|^libretro.battery\|^battery.continuation' --output-on-failure` | 0 | All 6 targeted checks passed |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/battery-red.json --raw` | 0 | RED_EVIDENCE_OK for the actual JUnit report |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check evaluation-scope --plan 03-14 --commits-only --raw` | 0 | All 3 task commits resolved on this branch |
| `git diff --check` | 0 | No whitespace errors |

An optional `cmake --workflow --preset wasm32` probe exited 1 during configure because `clang-21` was
absent from PATH. No wasm32 tests ran locally; no dependencies were installed or toolchain files changed.
The exact wasm32 inventory includes the three new process checks. Windows execution likewise remains
with hosted ci-msvc. These are platform-evidence limits, not skipped required native verification.

## TDD Gate Compliance

Task 1 adds recurring proof of behavior already implemented in plans 03-11 through 03-13. Inspection and
execution confirmed the existing guest program and persistence path already satisfy the numeric contract;
no production implementation or artificial failing assertion was needed. Initial test-development mistakes
about mailbox completion frames were corrected to the fixture's actual frame-3 completion, without changing
its pinned identity.

Task 2 RED executed both named frontend checks and failed specifically at the expected non-NULL pointer
and 32,768-byte assertions: existing callbacks returned NULL/0. CTest exited 8 and emitted a fresh JUnit
report; the classifier returned RED_EVIDENCE_OK. Semantic inspection confirmed real fixture loading and
target execution, with no setup, compilation or discovery failure. The unmodified report and record remain
ignored build output, not committed logs. GREEN delegates to the production query and passes both targets
and the full workflow. No separate refactor was necessary.

## Decisions Made

- Reuse the existing classified fixture and pinned facts instead of regenerating identical artifacts.
- Put the frontend battery checks beside the existing host helper headers to keep the host module small.
- Leave `.info` claims conservative: in-process span proof does not establish the deferred hosted file round trip.

## Deviations from Plan

No production-contract deviations. The existing fixture already supplied the required counter record,
so its assembly, link file, identity facts, database and image digest needed no edits. The existing
`tests/mapper/test_battery.c` continuation test was extended for real-media reset/generation evidence;
`tests/libretro/battery.h` follows the existing host's helper organization. Task 1's process assertions live
in the existing runner CMake integration harness rather than duplicating process spawning in C.

## Known Stubs

None. No new stub, skipped test or unrun required verification was introduced.

## Self-Check: PASSED

All listed created and modified files exist. All three task hashes are ancestors of HEAD, the scope query
resolves all three, both task acceptance contracts pass automated checks, and all required workflows passed.

## Next Plan Readiness

GAME-05 is complete; GAME-04's previously completed input criteria now also have real battery Restart
coverage. Continue with plan 03-15 within the active execute-phase step. No owner verification is required.
