---
phase: 03-games-play-in-retroarch
plan: "16"
status: complete
subsystem: compatibility-ratchet
tags:
  - compatibility
  - trusted-baseline
  - quoted-csv
  - ci
requires:
  - phase: 03-15
    provides: Bounded predicates and one quoted parser separating execution identity from accepted values
provides:
  - Protected passing cases with normalized aliases and expected-value comparison
  - Unique dated exceptions requiring a real behaviour revision bump and inclusive bounded expiry
  - Copied base judge and parser closure in the existing Linux x64 build lane
  - Bidirectional agreement with freshly evaluated eligible own-fixture results
affects:
  - 03-17
  - 03-18
tech-stack:
  added: []
  patterns:
    - One adjacent shared parser for candidate and copied trusted judge execution
    - Supplied calendar date and revisions with no judge clock read
    - Explicit first-introduction validation when the base lacks the ratchet
key-files:
  created:
    - scripts/compat-ratchet.sh
    - tests/compat/allow.txt
    - tests/compat/ratchet.cmake
  modified:
    - scripts/compat-common.sh
    - tests/compat/tests.cmake
    - .github/workflows/ci.yml
    - tests/cmake/budgets.cmake
    - tests/cmake/repository.cmake
    - tests/cmake/repository.sh
    - scripts/hygiene.sh
    - tests/policy/repository-checks.txt
    - tests/hygiene/fixtures/needs.txt
    - tests/inventory/ci.txt
    - tests/inventory/hygiene.txt
    - docs/conformance.md
    - docs/testing.md
key-decisions:
  - Keep comparison in the existing bounded parser; copy the entry point and adjacent parser together before candidate checkout.
  - Scope exceptions to one unique old passing predicate and require the actual new revision, a nonfuture date and an inclusive expiry at most 60 days away.
  - Reuse the existing Linux x64 build job and its required-job failure controls instead of adding a separately skippable job.
  - Keep GAME-06 pending for the private-library slice in plan 03-17.
requirements-completed: []
coverage:
  - id: protected-normalized-passes
    description: Deleted passes, changed frame hashes or RAM bytes, duplicate aliases and substituted profile, board, script or device facts reject independently
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.ratchet
        status: pass
    human_judgment: false
  - id: identity-adjacency
    description: Numeric and hex aliases preserve identity while adjacent frames, addresses and configurations remain distinct; hostile titles and evidence remain data
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.ratchet and compat.csv
        status: pass
    human_judgment: false
  - id: dated-exceptions
    description: Unique old predicates permit scoped revisions through inclusive expiry; expiry boundaries, future dates, invalid dates, unrelated predicates and missing or wrong revision bumps reject
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.ratchet
        status: pass
    human_judgment: false
  - id: trusted-fresh-comparison
    description: Candidate helper and judge replacement cannot weaken a copied base comparison; missing, extra or changed fresh fixture results require ledger agreement in both directions
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.trusted_judge and compat.fixtures
        status: pass
      - kind: integration
        ref: policy.ci_required
        status: pass
    human_judgment: false
actuals:
  tokens: 7268
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 5e175fa830dc473106f84076fdcf03a7e0229964
plan_head_after: 06919dc56d9ae40511827aff48f7f67e5f34772e
duration: 7min
completed: 2026-10-20
---

# Phase 3 Plan 16: Protected Compatibility Ratchet Summary

**Normalized passing compatibility cases now ratchet through a copied base judge, with bounded dated revision exceptions and fresh fixture agreement.**

## Accomplishments

- Added the small POSIX entry point and reused the bounded quoted parser for base/head/result comparison.
  Expected values stay outside identity. Removed passes, expectation edits and profile, board, video,
  device, script-name or script-digest substitutions reject. Equal aliases reject as duplicates; adjacent
  frames, addresses and configurations remain distinct. Title and evidence strings never become code.
- Added calendar-valid exceptions scoped to exactly one old passing predicate. A revision must exceed
  the base and equal the head, the record date cannot be future, and expiry is inclusive and at most
  60 days from the supplied date. Unrelated or ambiguous predicates cannot excuse another case.
  Allow files, records and row counts are bounded; read errors reject.
- The Linux x64 build lane snapshots the base SHA, public revision, ledger, judge and parser before
  candidate checkout. The copied judge resolves its adjacent parser, then compares committed cases to
  eligible freshly evaluated fixture results in both directions. The existing build job and required
  roll-up carry failure. The first introduction explicitly validates with the head judge because the
  base lacks an implementation; protection begins after merge.
- Added recurring synthetic tampering and result-agreement controls. Candidate helper replacement and
  replacement of both candidate scripts accept the altered pass, while the copied trusted judge rejects it.
  Missing/extra results, changed values/statuses and an uncommitted improvement independently reject.
  Results now contain evaluated own fixtures only, without unevaluated CC0 metadata.

## Task Commits

1. Task 1 RED — `df5e3b8`: assert protected compatibility cases.
2. Task 1 GREEN — `7147451`: protect normalized compatibility passes.
3. Task 2 — `06919dc`: judge fresh results with protected base helpers.

The measured task range contains three commits and a 29,070-character realized diff, or 7,268
chars/4 tokens rounded up. Sixteen implementation/test/doc files changed. No files were deleted.
Runtime: Codex. Model: GPT-6.

## Verification

| Command | Exit | Evidence |
|---|---:|---|
| `cmake --workflow --preset ci` | 0 | 339 tests and 4 packaged-output checks passed; exact inventory matched |
| `cmake --workflow --preset hygiene` | 0 | All 37 tests passed, including both new compatibility suites |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | 0 | 529 counted check lines / 5,298 source lines, strictly below one tenth |
| `sh scripts/compat-ratchet.sh tests/compat/ledger.csv tests/compat/ledger.csv tests/compat/allow.txt 2026-10-20 10 10 build/ci/tests/compat-fixtures/results.csv` | 0 | Real fresh fixture results agree with committed eligible cases |
| `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -DCHECK=check_junit -P tests/cmake/repository.cmake` | 0 | Native executed inventory matched |
| `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -DCHECK=check_junit -P tests/cmake/repository.cmake` | 0 | Hygiene executed inventory matched |
| `actionlint -ignore 'label "ubuntu-26\.04(-arm)?" is unknown' .github/workflows/ci.yml` | 0 | Workflow syntax passed after filtering only outdated local runner-label metadata |
| `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check evaluation-scope --plan 03-16 --commits-only --raw` | 0 | All three task commits resolved |
| `git diff --check` | 0 | No whitespace errors |

The tracer feedback gate reran the entire CI preset successfully before Task 2. Required native checks
ran fully. No hosted run was started by this plan, so no hosted run URL is claimed; the phase CI run
will report actual base/head SHAs and its URL from the comparison command. The initial public base has
no ratchet, which is explicitly reported rather than represented as a trusted-baseline execution.
No optional missing WASI compiler retry, package install or owner verification was introduced.
Build reports remain ignored local output, never committed logs.

## TDD Gate Compliance

Task 1 RED executed `compat.ratchet` and failed its unchanged-case exit assertion because the judge was
unavailable. CTest exited 8 and emitted fresh unchanged JUnit evidence. The classifier returned
`RED_EVIDENCE_OK`; semantic inspection confirmed the intended assertion ran, with no syntax, setup,
discovery or unrelated failure. GREEN supplied the judge and passing comparison. Expanded negative
controls prove values and substitutions independently, including SHA-256 changes at the same script
name. Task 2 is an auto task with recurring synthetic integration controls. No separate refactor commit
was necessary. Global TDD mode remains false.

## Deviations from Plan

- **[Rule 3 - Blocking budget]** Adding the fully counted judge initially exceeded the strict ratio.
  Replaced the bespoke newline-counting CMake function with standard `awk` record counting, preserving
  the unterminated-last-line boundary tests. Replaced captured help/readback code with checked direct
  output and consolidated locale initialization. All helpers remain counted in full; no exclusions,
  source padding or hidden/minified checker were introduced. Included in `7147451`.
- **[Rule 2 - Fresh-result correctness]** Added a filtered parser output mode so the result file contains
  only evaluated eligible own fixtures. The public unknown seed is metadata, not fresh execution.
  Included in `06919dc`.
- The installed actionlint does not know the pre-existing Ubuntu 26.04 runner labels. That observation
  is recorded in `deferred-items.md`; no unrelated workflow labels were changed.

## Known Stubs

None. No skipped test or unrun required verification remains. Private-library execution is explicitly
owned by the following plan; GAME-06 remains unchecked until that slice is complete.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: file-access | scripts/compat-common.sh | Host-side base/head/results and allow-file parsing, with byte/record/row bounds and inert free text |
| threat_flag: trust-boundary | .github/workflows/ci.yml | The base judge and complete current parser dependency are copied before candidate checkout; head script replacement has a recurring negative control |

## Self-Check: PASSED

All three created implementation/test files and this summary exist. All three task commits are ancestors
of HEAD and resolve through the evaluation-scope query. Native and hygiene exact inventories passed.
The whole shared dependency closure remains in the repository-check numerator.

## Next Plan Readiness

Plan 03-17 can use the shared parser and `scripts/compat-ratchet.sh` directly. Its optional seventh
argument is the fresh eligible fixture results file. The first six arguments are base ledger, committed
head ledger, candidate allow file, supplied date, base behaviour revision and head behaviour revision.
The check budget is 529/5,298 with less than one counted line of headroom, so additional checker code
requires a real simplification. Continue only inside the parent's active execute-phase step.
