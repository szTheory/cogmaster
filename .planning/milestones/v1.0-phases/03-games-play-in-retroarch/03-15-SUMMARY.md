---
phase: 03-games-play-in-retroarch
plan: "15"
status: complete
subsystem: compatibility-predicates
tags:
  - runner
  - predicates
  - quoted-csv
  - compatibility
requires:
  - phase: 03-14
    provides: Deterministic fixture execution, classified cartridges and stable public host surfaces
provides:
  - Bounded accepted frame, RAM, mailbox and boot-smoke predicates with distinct exit statuses
  - Backward-compatible API 8 display-enabled run observation
  - One bounded quoted CSV parser with canonical execution facts and protected values separated
  - Four executable own-fixture ledger evaluations, one unknown CC0 seed and deterministic public counts
affects:
  - 03-16
  - 03-17
  - 03-18
tech-stack:
  added: []
  patterns:
    - Public per-frame info, crop, peek and fault observations only
    - Quoted CSV stays outside CMake lists; validated execution facts use bracket-quoted scalars
    - Normalize region, configuration, frame and address independently of accepted expected values
key-files:
  created:
    - runner/expect.c
    - runner/expect.h
    - scripts/compat-common.sh
    - tests/compat/ledger.csv
    - tests/compat/tests.cmake
    - tests/runner/expect.cmake
    - tests/api/v8.c
    - docs/compatibility.md
  modified:
    - include/cogmaster.h
    - src/frame.c
    - src/internal.h
    - runner/main.c
    - runner/args.c
    - runner/exit_status.h
    - docs/conformance.md
    - docs/runner.md
    - docs/embedding.md
    - libretro/cogmaster_libretro.info.in
key-decisions:
  - Add only a size-tagged sampled display-enable result; preserve legacy 12-byte and 16-byte callers.
  - Saturate distinct native-frame observation at two, with no sampled-HALT or general stall inference.
  - Use one POSIX-sh parser for normalization, safe execution facts and deterministic public documentation.
  - Keep expected hashes and bytes outside normalized case identity; keep GAME-06 pending through the remaining ratchet and library slices.
requirements-completed: []
coverage:
  - id: accepted-predicates
    description: Independent frame and RAM expectations pass while altered values exit 3; malformed and conflicting inputs exit 2, unavailable profiles 77 and missing files 1
    requirement: GAME-06
    verification:
      - kind: integration
        ref: runner.expect.predicates
        status: pass
    human_judgment: false
  - id: honest-boot-smoke
    description: Interrupt-driven HALT with display and distinct frames passes; enabled static display and first-fault controls fail with observations, without gameplay or sound claims
    requirement: GAME-06
    verification:
      - kind: integration
        ref: runner.expect.predicates
        status: pass
      - kind: unit
        ref: abi.api_v8
        status: pass
    human_judgment: false
  - id: quoted-csv-normalization
    description: Quoted commas, doubled quotes, semicolons and CRLF round-trip; malformed fields, empty records, duplicate aliases and byte/record/field/row bounds reject; permutations are stable
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.csv
        status: pass
    human_judgment: false
  - id: executable-fixture-ledger
    description: Four accepted predicates across two admitted own fixtures execute once each with pinned bytes and the resolved board checked; CC0 metadata stays unknown
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.fixtures
        status: pass
    human_judgment: false
  - id: generated-honest-counts
    description: Generated status, predicate and fixture counts distinguish evaluated and unknown coverage, header-only means zero, and stale documentation is caught independently
    requirement: GAME-06
    verification:
      - kind: integration
        ref: policy.compat_doc_fresh and policy.compat_doc_fresh.self_test
        status: pass
      - kind: integration
        ref: compat.csv
        status: pass
    human_judgment: false
actuals:
  tokens: 17142
  tasks: 2
  commits: 4
commits: 4
plan_head_before: c737dabafe2a1aab5313371b183dbeddfa95c099
plan_head_after: 577c37012670dd674121daa1f64b9f7603c1f910
duration: 14min
completed: 2026-10-20
---

# Phase 3 Plan 15: Bounded Predicates and Quoted Compatibility Summary

**The runner judges accepted bounded predicates; one quoted CSV parser publishes four fixture evaluations and explicit unknown/zero coverage without support claims.**

## Accomplishments

- Added `--expect FILE` with 64-record, 255-byte record and 1,000,000-frame bounds. Requested frame/hash,
  RAM byte, structural mailbox and strict boot-smoke predicates use production public observations.
  Exit 3 is mismatch; usage, infrastructure and capability refusal stay distinct. Expected values remain
  separate from observed output and normalized execution facts include resolved profiles, board, devices,
  script-content digest, cap and behaviour revision.
- Appended the sampled `display_enabled` result under API 8. Old result sizes preserve untouched tail bytes.
  Smoke counts distinct native crops only up to its threshold of two and records display/fault facts.
  Static display can fail; healthy interrupt-driven HALT passes. No gameplay, completion or sound claim.
- Added the nine-column ledger and shared `normalize`, `cases` and `docs` modes. Files, rows, records and
  fields are bounded; escaped quotes/commas/semicolons stay data. Duplicates reject after numeric/profile
  normalization and before output. Empty/header-only and shuffled-order cases have explicit controls.
- Checked each evaluated own fixture against its admitted image digest, then executed the accepted predicate
  and compared its status and resolved board. Four accepted evaluations cover two fixtures. The one public
  CC0 game seed remains unknown. Generated documentation includes honest counts and experimental status.

## Task Commits

1. Task 1 RED — `f71942f`: assert independent runner expectations.
2. Task 1 GREEN — `182b934`: judge bounded accepted runner predicates.
3. Task 2 RED — `68c9fa5`: assert lossless quoted ledger normalization.
4. Task 2 GREEN — `577c370`: publish bounded quoted compatibility evidence.

The measured task range contains four commits and a 68,568-character realized diff (17,142 chars/4 tokens).
Thirty files changed. Runtime: Codex. Model: GPT-6. No files were deleted.

## Verification

| Command | Exit | Evidence |
|---|---:|---|
| `cmake --workflow --preset ci` | 0 | 337 tests and 4 packaged-output checks passed; exact native inventory matched |
| `cmake --workflow --preset hygiene` | 0 | 35 tests passed, including all 3 compatibility parser/doc checks, formatting and static analysis |
| `ctest --test-dir build/ci -R compat --output-on-failure` | 0 | Parser, admitted fixture execution, document freshness and independent stale-document control passed |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | 0 | 508 counted repository-check lines / 5,298 core lines, strictly under one tenth |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check evaluation-scope --plan 03-15 --commits-only --raw` | 0 | All four task commits resolved |
| `git diff --check` | 0 | No whitespace errors |

No new hosted run was requested in this plan, so no hosted run URL is claimed. Windows execution and cross-platform
evidence remain with the phase CI lanes; no local WASI compiler retry or install was attempted. Required native
verification ran fully. Console/JUnit reports remain ignored build output, never committed logs.

## TDD Gate Compliance

Task 1 RED executed the production CLI and failed its independent accepted-hash/RAM assertion: `--expect` was
unknown and returned 2 instead of the expected pass. CTest exited 8 and produced a fresh unchanged JUnit report.
The evidence classifier returned `RED_EVIDENCE_OK`; semantic inspection confirmed that the named target ran and
failed for the planned missing behavior, with no setup or discovery fault. GREEN added the bounded evaluator,
then checked independent changed-hash/byte controls, syntax bounds, refusal, I/O and multiple requested frames.
The tracer gate reran the complete CI workflow successfully before ledger expansion.

Task 2 RED executed the named CSV contract and failed its output assertion on `normalizer=unavailable`, rather
than failing command loading or shell setup. Its unchanged JUnit report also returned `RED_EVIDENCE_OK`.
GREEN implemented the shared normalizer and added malformed-record, duplicate-alias, row-order, bound,
zero-coverage and corruption controls. No separate behavior-neutral refactor commit was needed.

## Decisions Made

- Add only the public observation needed by this consumer and keep old tagged output layouts intact.
- Retain explicit saturated distinct-frame evidence and healthy HALT behavior; invent no stall detector.
- Keep all three ledger consumers behind one bounded parser. CMake receives only validated execution scalars,
  without title/evidence interpolation or raw CSV splitting.
- Keep GAME-06 pending: passing-case protection and private-library execution belong to plans 03-16 and 03-17.

## Deviations from Plan

- **[Rule 2 - Missing critical compatibility]** The required display observation needs an API addition. Updated
  `include/cogmaster.h`, `src/frame.c`, the layout assertion, API 8 coverage and embedding prose together;
  old 12-byte and 16-byte callers have executable untouched-tail checks. Included in `182b934`.
- **[Rule 1 - Stale assertion]** The old unknown-option test explicitly rejected `--expect`. Removed only that
  option from the future-option list and help exclusion after its implementation. Included in `182b934`.
- Test integration follows existing subsystem conventions: `tests/runner/expect.cmake` owns process-level
  predicate checks; the tiny display-only image joins the existing generated seed writer without committed
  ROM bytes. Hygiene-only registration has its own labels and exact inventory. No dependency was added.

## Known Stubs

None. No skipped test or unrun required verification was introduced. Remaining GAME-06 work is explicitly scoped
to the following plans, not represented by a passing implementation here.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: file-access | runner/expect.c | New host-side accepted-expectation file read; byte/record/frame/count bounds validate before emulation |
| threat_flag: file-access | scripts/compat-common.sh | New shared public ledger input; pre-parse 4 MiB cap, bounded records/fields/rows, strict grammar and safe execution scalar output |

## Self-Check: PASSED

All created files exist. All four task commits are ancestors of HEAD and resolve through the scope query.
Both native workflows passed with exact inventories. The ledger remains unknown for its CC0 seed, the core
remains experimental, and GAME-06 stays unchecked until its remaining planned contracts are demonstrated.

## Next Plan Readiness

Plan 03-16 can consume `scripts/compat-common.sh` directly. `normalize` emits sorted nine-column quoted CSV;
`cases` emits `compat_count` and one-based `compat_N_*` scalars for id, resolved machine/region/mapper,
full predicate/status/date, parameter, expected, config and cap. It intentionally omits untrusted free-text
title/evidence from executable CMake output. Current admitted fresh fixture results are written to
`build/<preset>/tests/compat-fixtures/results.csv` after status agreement. The ratio has about 21 counted
lines of headroom at the current core size. Continue only within the parent's active execute-phase step.
