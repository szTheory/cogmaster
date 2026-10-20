---
phase: 03-games-play-in-retroarch
plan: "17"
status: complete
subsystem: local-library-and-public-compatibility
tags:
  - local-library
  - privacy
  - quoted-csv
  - bounded-execution
requires:
  - phase: 03-15
    provides: Independent accepted predicates and bounded quoted ledger parser
  - phase: 03-16
    provides: Protected comparison and fresh reproducible fixture results
provides:
  - Local library configure, build, test and workflow presets with private numbered observations
  - Every-candidate flat enumeration and bounded execution independent of metadata membership
  - Explicit public updates restricted to seeded CC0 identities and unchanged accepted cases
  - Recurring synthetic preset, privacy, zero-coverage and public-filter integration controls
affects:
  - 03-18
tech-stack:
  added: []
  patterns:
    - Literal dotenv data and environment transport for punctuation in private filenames
    - Operational local orchestration separated from fully counted public repository checks
    - Canonical public titles and revision evidence replacing every local label and evidence string
key-files:
  created:
    - scripts/library.sh
    - scripts/library.cmake
    - scripts/compat-update.sh
    - tests/library/tests.cmake
    - tests/compat/update.cmake
    - tests/inventory/library.txt
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - scripts/compat-common.sh
    - tests/tests.cmake
    - tests/compat/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/hygiene.txt
    - tests/inventory/wasm32.txt
    - tests/policy/repository-checks.txt
    - cmake/CogmasterTests.cmake
    - tests/cmake/budgets.cmake
    - tests/cmake/repository.cmake
    - scripts/check-title.sh
    - .env.example
    - ASSET_POLICY.md
    - docs/testing.md
    - docs/conformance.md
    - .planning/preparation/CONFORMANCE.md
key-decisions:
  - Match fixture expectations only by admitted SHA-256 bytes; names do not grant fixture identity.
  - Run supported metadata misses for 20 frames and keep observations unknown without accepted expectations.
  - Restrict explicit updates to already seeded public cases with unchanged configuration and expected value.
  - Keep per-file details in ignored numbered sidecars and print only aggregate coverage and status counts.
requirements-completed:
  - GAME-06
coverage:
  - id: local-library-enumeration
    description: Flat regular candidates including hidden names and punctuation are probed and attempted while unrelated files, directories and symlinks are excluded
    requirement: GAME-06
    verification:
      - kind: integration
        ref: library.synthetic
        status: pass
    human_judgment: false
  - id: honest-local-statuses
    description: Accepted fixtures, unknown observations, unsupported profiles, malformed input and infrastructure failures remain distinct with explicit empty-library zero coverage
    requirement: GAME-06
    verification:
      - kind: integration
        ref: library.synthetic
        status: pass
    human_judgment: false
  - id: local-preset-and-dotenv
    description: The real library workflow runs an isolated empty fixture directory, missing directories fail configure, and a separate synthetic repository proves literal dotenv parsing
    requirement: GAME-06
    verification:
      - kind: integration
        ref: library.synthetic
        status: pass
    human_judgment: false
  - id: filtered-public-updates
    description: Canonical CC0 titles round-trip while private labels and evidence, unknown identities, fixture changes, new goldens and changed profiles cannot change public cases
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.public_update
        status: pass
      - kind: integration
        ref: compat.ratchet
        status: pass
    human_judgment: false
  - id: proportional-checks
    description: Complete shared parser and public-update dependencies fit the strict repository ratio and existing hosted code budget with exact executed inventories
    requirement: GAME-06
    verification:
      - kind: other
        ref: docs.budgets
        status: pass
      - kind: other
        ref: policy.e2e_budget
        status: pass
    human_judgment: false
actuals:
  tokens: 10339
  tasks: 2
  commits: 4
plan_head_before: 53c31e1205b41d8139fd12b16af1f1f47cd6582b
plan_head_after: 406ea310c556b0d99a6b2774881eaac6b62b62f5
duration: 12min
completed: 2026-10-20
---

# Phase 3 Plan 17: Local Library and Filtered Public Updates Summary

Every local candidate receives bounded classification or execution, while public updates admit only existing CC0 cases with unchanged accepted predicates.

## Accomplishments

The `library` presets register only `library.local`, outside every CI inventory. The wrapper reads literal
`COGMASTER_ROM_DIR` from ignored `.env.local` when the environment has no nonempty override. It traverses
one flat directory deterministically, excludes symlinks and non-images, handles spaces, quotes and semicolons,
probes every candidate and executes supported metadata misses for 20 frames. Accepted predicates retain
their fixed configurations and cap at 600 frames each. Probe and execution subprocesses have host timeouts.
Only exact admitted fixture SHA-256 bytes gain fixture expectations, even after renaming. No save directory
is supplied and no user ROM, save, BIOS or configuration is modified by the tests.

Ignored `build/library/results/` holds the summary CSV, numbered path/probe/observation/error sidecars and
potential public facts. Aggregate output separates candidate, attempted-image, evaluated-image and predicate
counts. An empty library explicitly reports zero evaluated coverage; unknown observations cannot establish
pass/fail or create golden values. Missing directories fail configure. `unsupported`, `malformed_io` and
`infrastructure_error` remain separate. CI exercises these paths with owned synthetic bytes, including the
actual empty-directory workflow and a separate synthetic repository for literal dotenv parsing.

`scripts/compat-update.sh` prints a candidate ledger from local public-fact records. The fully counted shared
parser permits changes only to pre-seeded public CC0 cases with the same normalized identity, configuration,
predicate and expected value. Public titles come from the public catalog, including C-string decoding;
evidence becomes only the current core revision. Unknown identities, private labels and evidence, fixture
changes, new predicates or goldens and changed profiles cannot update cases. Extra columns and invalid
accepted dates reject before output. All six existing statuses survive filtering and row permutations
produce identical output. Owned fixture rows remain managed through their reproducible build path.

No new dependency, guest/core behavior, hosted test code, release claim or owner verification was introduced.
Metadata membership and capped smoke establish neither playable/completable behavior nor sound support.
The libretro info remains experimental. GAME-06 is complete with the preceding predicate and ratchet plans.

## Task Commits

1. Task 1 RED — `fd4bf15`: require the local workflow through a named failing assertion.
2. Task 1 GREEN — `cf36295`: implement bounded private library operation, presets and recurring synthetic controls.
3. Task 2 RED — `89535bc`: require canonical public update behavior through a named failing assertion.
4. Task 2 GREEN — `406ea31`: implement public filtering, hostile-record controls, documentation and budget simplification.

Started 2026-10-20T22:16:28Z; completed 2026-10-20T22:28Z. Twenty-four implementation/test/documentation files
changed. Actual tokens use realized diff characters divided by four, rounded up; commits are measured from
the persisted plan ledger through the final production commit. No files were deleted.
Runtime: Codex. Model: GPT-6.

## Verification

| Command | Exit | Evidence |
|---|---:|---|
| `cmake --workflow --preset ci` | 0 | 341 tests and 4 packaged-output checks passed |
| `cmake --workflow --preset hygiene` | 0 | All 38 tests passed |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | 0 | 528 complete counted check lines / 5,298 source lines, strictly below one tenth |
| `cmake -DDIR=tests/retroarch -DCHECK=e2e_budget -P tests/cmake/repository.cmake` | 0 | 300 hosted helper lines, at most 300 |
| `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -DCHECK=check_junit -P tests/cmake/repository.cmake` | 0 | Executed native inventory matched |
| `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -DCHECK=check_junit -P tests/cmake/repository.cmake` | 0 | Executed hygiene inventory matched |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check evaluation-scope --plan 03-17 --commits-only --raw` | 0 | All four task commits resolved |
| `git diff --check` | 0 | No whitespace errors |

The tracer gate reran the complete CI workflow successfully before Task 2. Verification here is native
Apple Silicon; the registered Windows and cross lanes run in phase CI. No hosted run was started by this
plan, so no hosted run URL is claimed. No optional missing WASI configure retry was attempted. Raw reports
remain ignored local output and are never committed; the workflow upload allowlists exclude the library
directory and private synthetic observations.

## TDD Gate Compliance

Both tasks wrote and ran their named RED assertions before implementation. Task 1 rejected the absent
library workflow; Task 2 rejected the absent canonical public updater. Each CTest run exited 8, produced
fresh unchanged JUnit evidence, and passed `check tdd-red-evidence` with `RED_EVIDENCE_OK`. Semantic inspection
confirmed that the intended assertions ran without syntax, discovery or setup failure. The ignored JSON
records retain the actual command, exit and unchanged report. Both GREEN commits passed their integration
controls and the full CI workflow. Expanded controls pin metadata-miss execution, privacy, all statuses,
invalid dates/columns, stable ordering and no-golden promotion. Global TDD mode remains false.

## Deviations from Plan

- **[Rule 3 - Blocking budget]** The new fully counted update closure initially exceeded the strict ratio.
  Replaced repeated per-source awk invocations with one standard aggregate count, passed checked subprocess
  output directly instead of capturing and reprinting it, removed an unused configurable COST argument while
  preserving COST 1 for every test, and shortened duplicated title-policy commentary. All ratio equality,
  unterminated-line and shared-helper controls pass. No source padding, minified checker or hidden exclusion
  was added. Recorded and marked fixed in the broken-windows ledger. Included in `406ea31`.
- **[Rule 2 - Private filename correctness]** Added a small local CMake orchestration module, with filenames
  passed as inert environment data from POSIX shell enumeration. Numbered sidecars and the summary CSV live
  inside the private ignored results directory. This avoids injecting private names into shell/CMake source
  or public parser output. Included in `cf36295`.

## Known Stubs

None. No skipped test or unrun required verification remains. No owner action is required.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: file-access | scripts/library.sh | Reads private local regular candidates without recursion or symlink traversal and confines results to ignored build output |
| threat_flag: subprocess | scripts/library.cmake | Starts bounded runner probes and execution with paths transported as inert data and diagnostics kept local |
| threat_flag: trust-boundary | scripts/compat-common.sh | Projects local records onto fixed public cases using the public catalog and sanitized revision evidence |

## Next Plan Readiness

Plan 03-18 may consume the complete compatibility/library behavior and exact inventories. The remaining
repository-check headroom is one physical line; any further checker must preserve the strict full-closure
ratio. Continue only within the parent's active execute-phase step.

## Self-Check: PASSED

All six created implementation/test/inventory files and this summary exist. All four task commits are
ancestors of HEAD and resolve through the evaluation-scope query. The summary verifier passed. The native
and hygiene executed inventories match, the full parser/update closure is counted, and the broken-windows
register has zero open entries.
