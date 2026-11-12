---
phase: 08-sports-pad-paddle-and-3-d-glasses
plan: "16"
subsystem: compatibility-ledger
tags:
  - compat
  - ledger
  - sports-pad
  - paddle
status: complete

requires:
  - 08-13
provides:
  - "The ledger key device token is pad|none|paddle|sports in the normalizer, the library lane, the fixture lane and the conformance doc; auto and the light phaser token stay rejected"
  - "compat.device_key and compat.device_rejected on every lane that registers the compat tests"
  - "Ratchet cases that judge a Sports Pad or Paddle swapped in on either port as a changed key and as an adjacent key"
affects: [08-17]

actuals:
  tokens: 2800
  tasks: 2
  commits: 2

plan_head_before: 0642533b6e91389cf7ff789d33a3b99aa08afc07
plan_head_after: 81f27adffa9fe7a19749461c5b7c2803b2764e55
commits: 2

tech-stack:
  added: []
  patterns:
    - "A rejection case is a script that fails only when every refusal happened, run through expect_fail.cmake, so a grammar that quietly accepted a token ends the script with a different message and the test fails"
    - "An adjacent-key ratchet case (the original row plus the changed row, expected exit 0) is the case a grammar that rejects the new token cannot satisfy, where the changed-key case (expected exit 2) passes either way"

key-files:
  created: []
  modified:
    - scripts/compat-common.sh
    - scripts/library.cmake
    - tests/compat/tests.cmake
    - tests/compat/ratchet.cmake
    - docs/conformance.md
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt

key-decisions:
  - "The changed-key ratchet cases for sports and paddle are paired with adjacent-key cases, because a grammar that rejects the new token also answers exit 2 to a changed key"
  - "The normalizer cases run as script-mode cases of tests/compat/tests.cmake (the file's existing pattern), not as extra code in the counted shell script"

requirements-completed:
  - PAD-06

coverage:
  - id: L1
    description: "The device token of the ledger key is pad, none, paddle or sports on either port in the normalizer, the library lane and the fixture lane, edited in place with equal added and removed lines per file; a row without a prefix keeps port1=pad;port2=pad and the catalog and the ledger are not edited"
    requirement: PAD-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^(compat\\.|library\\.|docs\\.|policy\\.)' exit 0 (41 of 41); git diff --numstat shows 1 and 1 for each of the three files; git diff --quiet origin/main -- tests/compat/catalog.tsv tests/compat/ledger.csv exit 0"
      status: pass
    human_judgment: false
  - id: L2
    description: "All sixteen device pairs normalize as separate keys under one id, each token reaches the case configuration, and the same pair twice is a duplicate"
    requirement: PAD-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^compat\\.device_key$'"
      status: pass
    human_judgment: false
  - id: L3
    description: "auto and the light phaser token on either port are refused with the invalid ledger record failure"
    requirement: PAD-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^compat\\.device_rejected$'"
      status: pass
    human_judgment: false
  - id: L4
    description: "A head row that swaps the device on either port to sports or paddle does not inherit the base row's pass (exit 2), and the swapped row beside the original is a distinct key (exit 0)"
    requirement: PAD-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^compat\\.ratchet$'"
      status: pass
    human_judgment: false
  - id: L5
    description: "The doc names the four tokens and says a row proves a boot or a frame hash and never that motion tracks; the script identity is still none with the empty-script digest; the counted check lines stay at 723"
    requirement: PAD-06
    verification:
      kind: other
      ref: "cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (723 check lines, 8793 source lines); ctest -R '^docs\\.' exit 0"
      status: pass
    human_judgment: false
  - id: L6
    description: "The cases can fail: a grammar that rejects sports fails compat.device_key and compat.ratchet, a grammar that accepts the light phaser token fails compat.device_rejected"
    requirement: PAD-06
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^compat\\.(device_|ratchet)' with scripts/compat-common.sh edited, exit 8 in each of two controls, then restored, ctest -R '^compat\\.' exit 0"
      status: pass
    human_judgment: false

duration: 14min
completed: 2026-11-11
---

# Phase 8 Plan 16: The ledger key device token Summary

**The compatibility ledger key now names a Sports Pad or a Paddle (`pad|none|paddle|sports`, four in-place edits and one doc sentence), so a pad run can never satisfy a row for a device title; sixteen device pairs are separate keys, `auto` and the phaser token are refused, and the counted check lines stay at 723.**

## What was built

- Task 1: `(pad|none)` became `(pad|none|paddle|sports)` once each in `scripts/compat-common.sh` (the normalizer grammar), `scripts/library.cmake` and `tests/compat/tests.cmake` (the two lanes that pass the captured token to `--port1` and `--port2` unchanged), and `docs/conformance.md` names the four tokens, says a device title's row names its device in its prefix, a row proves a boot or a frame hash and never that motion tracks, and that `auto` and the light phaser token are not accepted. The runner already accepted `paddle` and `sports`. The default for a row without a prefix is still `port1=pad;port2=pad`, `script=none` is still the only script a ledger row may carry, and `catalog.tsv` and `ledger.csv` are untouched.
- Task 2: `tests/compat/ratchet.cmake` gained, beside `device_substitution`, a changed-key case (expected exit 2) and an adjacent-key case (original row plus changed row, expected exit 0) for sports and paddle on each port. `tests/compat/tests.cmake` gained two script-mode cases and their registrations: `compat.device_key` (all sixteen device pairs normalize as separate keys under one id, the cases output carries the tokens, the same pair twice is a duplicate) and `compat.device_rejected` (the four refusals of `auto` and `lphaser`, run through `expect_fail.cmake` so a grammar that accepted one fails the test). Both are listed in `tests/inventory/ci.txt`, `wasm32.txt` (`compat`) and `hygiene.txt` (`compat,hygiene`), labels `${compat_labels}`, TIMEOUT 30 as their neighbours, with the fault-prevented comment.

## Negative controls

Each was applied with a python edit of `scripts/compat-common.sh`, confirmed with `git diff --stat scripts/compat-common.sh` (1 line changed), run, and restored with `git checkout -- scripts/compat-common.sh`.

- Grammar without `sports` (`|none|paddle|sports` to `|none|paddle` at both ports): `ctest --test-dir build/ci -R '^compat\.(device_|ratchet)'` exit 8; `compat.device_key` and `compat.ratchet` failed, `compat.device_rejected` passed. `compat.ratchet` fails on the adjacent-key cases, which a rejecting grammar cannot satisfy; the changed-key cases alone would have passed on exit 2.
- Grammar that also accepts `lphaser`: the same command exit 8; only `compat.device_rejected` failed.
- Restored: `ctest --test-dir build/ci -R '^compat\.'` exit 0, 7 of 7 passed (the eighth under hygiene).

## Verification

- `cmake --preset ci && cmake --build --preset ci && ctest --test-dir build/ci -R '^(compat\.|library\.|docs\.|policy\.)'`: exit 0 after task 1 (41 of 41).
- `ctest --test-dir build/ci -R '^compat\.'`: exit 0 after task 2; `ctest --test-dir build/ci -N -R '^compat\.device_'` lists the two names.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, prints `723 check lines / 8793 src lines`, unchanged (limit 735).
- `cmake --workflow --preset hygiene`: exit 0 (50 tests passed, the hygiene inventory check passed at configure). `cmake --workflow --preset ci`: exit 0 (544 tests passed).
- `grep -c 'lphaser' scripts/compat-common.sh` prints 0; `git diff --numstat` of the three regex files for task 1 shows 1 added and 1 removed each; the greps of the acceptance criteria and the inventory lines print the stated counts.
- `cmake --preset wasm32` could not configure here (clang-21 is not installed on this machine); its inventory lines match the ci list for the compat tests and the hosted wasm32 lane carries the check.
- No hosted run was made and nothing was pushed; the plan names no dispatch. The last hosted evidence is run 42973268415 at `60c97a6`.

## Task Commits

1. `4f6e49a` feat(08-16): ledger key names the Sports Pad and Paddle devices
2. `81f27ad` test(08-16): device pairs are distinct ledger keys and auto or a phaser are refused

## Deviations from Plan

**1. [Rule 1 - Mechanism] The different-key cases are paired with adjacent-key cases**
- **Found during:** Task 2 negative control design
- **Issue:** The plan's changed-key cases expect exit 2, which a grammar that rejects `sports` also produces, so the control it asks for would not have failed `compat.ratchet`.
- **Fix:** Each substitution also has an adjacent-key case (original plus changed, expected exit 0) that only a grammar accepting the token and keying it separately satisfies; `compat.device_key` itself holds the all-pairs and duplicate cases in the normalizer.
- **Files modified:** tests/compat/ratchet.cmake, tests/compat/tests.cmake
- **Commit:** 81f27ad

**2. The cases are script-mode cases of `tests/compat/tests.cmake`.** The plan allows this ("the file's existing helpers"); they add no counted check lines, which stay at 723.

**Total deviations:** 1 (test shape), no behaviour change from the plan's truths.

## Known Stubs

None.

## Threat Flags

None. The change widens a regex alternation inside an existing allow-list and adds tests; the shell script still reads only bounded files and the rejected tokens stay rejected.

## Issues Encountered

- A `$` anchor in the expect_fail regex failed against the trailing newlines of CMake's error output; the regex now ends with `[^0-9]`.
- `docs/controllers.md` still does not exist; no plan before this one wrote it and this plan assigns no doc beyond `docs/conformance.md`, so the user-facing device prose stays for 08-17.

## Next Phase Readiness

Ready for 08-17. The branch holds the 08-13 to 08-16 commits beyond the pushed `60c97a6`; nothing was pushed here.

## Self-Check: PASSED

- The eight modified files exist; commits `4f6e49a` and `81f27ad` are ancestors of HEAD.
- `git rev-list --count 0642533b6e91389cf7ff789d33a3b99aa08afc07..HEAD` printed 2 before this SUMMARY was committed.
