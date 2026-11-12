---
phase: 12-whole-list-in-the-ledger
plan: "03"
subsystem: compat-ratchet
tags:
  - compat
  - ratchet
  - ledger
  - catalog
  - library
status: complete

requires:
  - 12-02
provides:
  - Ratchet rules R1 (no pass or fail on an unsupported or skipped catalog id), R2 (a failure keeps its id) and R3 (an unsupported or skipped row leaves only through a retired token and a catalog change, or a catalog change alone for skipped)
  - A judge that fails closed on an unreadable catalog or reasons file
  - The update guard: a library run never touches a skipped or unsupported id and never writes core=<revision> where a reason belongs
  - library.sh exports of absolute COMPAT_CATALOG and COMPAT_REASONS, so a run from any directory reads the right data
affects:
  - 12-04
  - 12-05
  - 12-06
  - 12-07

actuals:
  tokens: 3800
  tasks: 3
  commits: 3

plan_head_before: 7969aa0187138d206fa863537c33e112c0157b0e
plan_head_after: f05302a77425e779ede799a8a2f8e4b37fbde4ae
commits: 3

tech-stack:
  added: []
  patterns:
    - "One catalog loader in BEGIN serves the update and the ratchet: titles, machines, mappers and the status of every id; a line with no status field reads as unknown"
    - "reasons() loads the vocabulary once for the parser and the ratchet and keeps each token's state (open or retired); an unreadable file is exit 2 in the ratchet"
    - "rel(key) is the single release predicate of R3: a base unsupported row needs a retired token and a catalog that no longer says unsupported, a base skipped row needs a catalog that no longer says skipped"

key-files:
  created: []
  modified:
    - scripts/compat-common.sh
    - scripts/library.sh
    - tests/compat/ratchet.cmake
    - tests/compat/update.cmake
    - tests/compat/tests.cmake
    - tests/library/tests.cmake

key-decisions:
  - "An update candidate that states unsupported or skipped for an existing key is REFUSED (discarded), not kept with a reason: the library judge observes pass and fail only; unsupported and skipped are catalog facts the seed writes from the pin and the slot table, and a candidate row would otherwise be written with evidence core=<revision>, which the reason rule refuses on the next parse"
  - "R2 is stated for fail, unsupported and skipped rows as planned; for the last two the stricter R3 check (key missing or status changed) already covers it, so the live effect of R2 is the fail row; the control that removes R2 is therefore a fail row"
  - "The ratchet loads the catalog and the reasons unconditionally, not lazily: a lazy load that never fires cannot fail closed"
  - "The update mode also fails closed on an unreadable catalog (exit 2) instead of silently discarding every candidate"

requirements-completed: []

coverage:
  - id: C1
    description: "A head pass or fail row for a catalog id whose status is unsupported or skipped is refused (exit 2); the same row with the catalog status unknown is accepted"
    requirement: LEDG-03
    verification:
      - kind: unit
        ref: "tests/compat/ratchet.cmake cases r1_unsupported_pass, r1_skipped_fail, r1_catalog_says_unknown (compat.ratchet)"
        status: pass
    human_judgment: false
  - id: C2
    description: "An unreadable catalog or reasons file is exit 2 in ratchet mode for a pair that passes with readable data, and no protected-comparison-passed line is printed"
    requirement: LEDG-03
    verification:
      - kind: unit
        ref: "tests/compat/ratchet.cmake cases closed_catalog_missing, closed_reasons_missing (compat.ratchet)"
        status: pass
      - kind: other
        ref: "COMPAT_CATALOG=<missing> sh scripts/compat-ratchet.sh <base> <head> tests/compat/allow.txt 2026-11-12 16 17 exits 2, the same pair with the tracked catalog exits 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "An unsupported row becomes a pass (re-keyed under a board mapper) only when its token is retired and the catalog no longer says unsupported; a skipped row changes or vanishes only when the catalog no longer says skipped or no longer lists the id; an unchanged unsupported row and a vanishing unknown row are accepted"
    requirement: LEDG-03
    verification:
      - kind: unit
        ref: "tests/compat/ratchet.cmake cases retired_rekey, unsupported_unchanged, unknown_may_vanish, pin_bump_status, pin_bump_dropped (compat.ratchet)"
        status: pass
    human_judgment: false
  - id: C4
    description: "The same moves are refused when the catalog still says unsupported, when the token is still open, when an unsupported row vanishes with an open token, when a skipped row becomes a pass while the catalog says skipped, when a skipped row vanishes, when a fail row vanishes, and for a pass turning unsupported"
    requirement: LEDG-03
    verification:
      - kind: unit
        ref: "tests/compat/ratchet.cmake cases still_unsupported_but_passing, retired_token_catalog_unchanged, catalog_changed_token_open, unsupported_vanished_token_open, skipped_to_pass, failure_deleted, skipped_deleted, pass_to_unsupported (compat.ratchet; 18 catalog cases in all)"
        status: pass
    human_judgment: false
  - id: C5
    description: "The update discards every candidate for an id whose catalog status is not unknown, refuses a candidate stating unsupported or skipped for an existing key, still updates an unknown id and still adopts a new boots row from a catalog of the old five fields"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/update.cmake guard block and the status loop (compat.public_update)"
        status: pass
    human_judgment: false
  - id: C6
    description: "library.sh exports absolute catalog and reasons paths; a run from a subdirectory of the repository merges as from the root, and the direct ratchet call of the synthetic test names its catalog and reasons"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "tests/library/tests.cmake (library.synthetic): run from docs expects rows_added=0 ratchet=ok"
        status: pass
    human_judgment: false
  - id: C7
    description: "The committed ledger is unchanged and passes both the base judge (origin/main) and the new judge against the origin/main ledger"
    requirement: LEDG-03
    verification:
      - kind: other
        ref: "base judge from git show origin/main:scripts and the origin/main ledger against tests/compat/ledger.csv: exit 0 and the protected-comparison-passed line; the new judge with the same pair: exit 0"
        status: pass
    human_judgment: false
---

# Phase 12 Plan 03: Ratchet rules and the update guard Summary

The ratchet now moves an unsupported row to pass only through evidence (a retired token and a catalog that no longer says unsupported), cannot be starved of its catalog or its reasons, and cannot lose a failure by deletion; the library update can no longer turn a skipped or unsupported row into a pass or overwrite its reason. The committed ledger and page are unchanged.

## Performance

- **Tasks:** 3 of 3 (the first is the tracer: R1 end to end with the fail-closed judge)
- **Files changed:** 6 (no new file)
- **Commits:** `3ca4984` (task 1), `c0bd7f5` (task 2), `f05302a` (task 3)

## Accomplishments

1. **Task 1, the tracer.** `scripts/compat-common.sh` loads the head catalog (id to status, field 6, a missing field reads unknown) and the reasons (token to state) for ratchet mode in BEGIN and `reasons()`; an unreadable file is exit 2 (`invalid ledger record`). R1 refuses a head pass or fail row for an unsupported or skipped id. `scripts/library.sh` exports absolute `COMPAT_CATALOG` and `COMPAT_REASONS`; `library.synthetic` runs once from `docs/`; its direct ratchet call runs under `cmake -E env` with the test's catalog and the tracked reasons. `compat.ratchet` now writes small stand-in catalogs (unknown, unsupported, skipped, a board-arrived copy, a pin-bumped copy, a copy without the skipped id) and reasons (open and retired) into its output directory; the older cases run against them unchanged. Timeout 150 s, provisional.
2. **Task 2.** R2 and R3 with one release predicate, `rel(key)`; the existing change rule also yields to it, so a released row may change its status. 13 cases (five accepted, eight refused); `compat.ratchet` prints `compat.ratchet: 18 catalog cases`.
3. **Task 3.** The update guard (`cs[id]!="unknown"` discards in stage 2) and the decision on the plan 02 hand-off (below). `update.cmake` gains a ten-field stand-in catalog with an unsupported, a skipped and an unknown id and a five-field catalog case.

## Decision on the plan 02 hand-off

An update stage 2 candidate whose status is `unsupported` or `skipped` is **refused**, not kept with its reason. The library judge observes pass and fail only; `unsupported` and `skipped` are catalog facts that the seed writes from the slot table and the pin. A candidate stating one for an existing key (an unknown id: the guard of the catalog status does not cover it) would be written with evidence `core=<revision>`, a ledger the next parse refuses; keeping a private candidate's reason text instead would let a library run assert a catalog fact. The clause sits in the existing-key branch of stage 2 (`f[8] ~ /^(unsupported|skipped)$/`, no extra line). The control is in `compat.public_update`: five candidates (pass or fail for the unsupported id, pass for the skipped id, unsupported with a reason and skipped with a reason for the unknown id) leave the output equal to the baseline; the status loop that used to assert "the status survives" now asserts the refusal for those two statuses. Removing the clause turns the test red (control below).

## Evidence (commands, exit statuses; no log committed)

- `cmake --workflow --preset ci`: exit 0 (655 tests plus 5 release tests) after each of the three tasks; `cmake --workflow --preset hygiene`: exit 0 (76 tests) after each.
- `ctest --test-dir build/ci -R '^(compat|policy\.compat|library)' --output-on-failure`: exit 0 (13 tests after task 1; 11 match `^(compat|library)`); `ctest --test-dir build/ci -R '^compat\.ratchet$' -V`: exit 0, log line `compat.ratchet: 18 catalog cases`.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, `872 check lines / 9403 src lines` after task 1, `876 / 9403` after tasks 2 and 3.
- `git diff --quiet origin/main -- tests/compat/ledger.csv`: exit 0; `git diff --quiet HEAD -- docs/compatibility.md`: exit 0 after every task (the page differs from origin/main only by plan 02's escaping change; this plan edits neither).
- By hand (throwaway files in the session scratch, repository root): `COMPAT_CATALOG=<missing> COMPAT_REASONS=tests/compat/reasons.txt sh scripts/compat-ratchet.sh <base> <head> allow 2026-11-12 16 17` exit 2 (`invalid ledger record 0`); the same pair with the tracked catalog exit 0; with a missing reasons file and the tracked catalog exit 2.
- `grep -c 'export COMPAT_' scripts/library.sh` prints 1; `awk '/compat-ratchet\.sh/ && !/COMPAT_CATALOG=/ {bad=1} END {exit bad}' tests/library/tests.cmake` exit 0 and `grep -c 'compat-ratchet.sh' tests/library/tests.cmake` prints 1.
- Base-judge property (CONTEXT D13 command, here a no-op pair because the ledger is unchanged): the judge from `git show origin/main:scripts/...` with the origin/main ledger and `tests/compat/ledger.csv`: exit 0, `compat: protected comparison passed`; the new judge with the same pair: exit 0. To be repeated by plan 5 on the whole-list ledger.
- Awk portability: BWK awk 20200816 (macOS) runs the whole suite. In `ubuntu:26.04` with `cmake` and `git` installed, `cmake -DROOT=/src -DOUT=... -P tests/compat/ratchet.cmake` and `update.cmake` exit 0 under mawk 1.3.4 (18 catalog cases) and under gawk 5.3.2 (18 catalog cases); `docker run ... ubuntu:26.04 sh -c 'sh scripts/compat-common.sh normalize tests/compat/ledger.csv | head -n 1'` exit 0.
- Hosted dispatch: none in this plan (the first is after plan 5).

## Negative controls (each: python edit, rebuild or rerun, red, restore, green)

| Control | Mutation | Result |
|---|---|---|
| R1 | the R1 condition made `if(0) bad()` | `ctest -R '^compat\.ratchet$'` exit 8; restored exit 0 |
| Fail closed | `getline` return below zero no longer calls `bad()` (catalog and reasons loaders read `>0`) | exit 8 (the two closed_* cases); restored exit 0 |
| Library exports | `COMPAT_CATALOG` and `COMPAT_REASONS` export removed from `library.sh` | `library.synthetic` red (the catalog run no longer sees its stand-in catalog); restored green |
| Direct ratchet call | `-E env COMPAT_CATALOG=... COMPAT_REASONS=...` removed from `tests/library/tests.cmake` | `library.synthetic` red (`the merged ledger must pass the ratchet: 2`); restored green |
| R3 token | release on the catalog alone | exit 8, case `catalog_changed_token_open`; restored exit 0 |
| R3 catalog | release on the token alone | exit 8, case `retired_token_catalog_unchanged`; restored exit 0 |
| R2 | the R2 line made `if(0) bad()` | exit 8, case `failure_deleted`; restored exit 0 |
| Update status guard | `|| cs[f[1]]!="unknown"` removed | `compat.public_update` exit 8; restored exit 0 |
| Update existing-key clause | `|| f[8] ~ /^(unsupported|skipped)$/` removed | `compat.public_update` exit 8; restored exit 0 |
| Status default | `cs[...]=t[6]` without the unknown default | `compat.public_update` red (five-field catalog); restored green |

## Deviations from Plan

### Auto-fixed Issues

None.

### Plan text and acceptance wording

**1. [Plan text] The compat.ratchet timeout and the library.sh exports moved here**
- The library exports and the `compat.ratchet` timeout (98 s to 150 s, provisional comment) are this plan's work as written; recorded here as the plan's own deviation note asks (the exports moved from the library plan, because the judge now fails closed).

**2. [Plan acceptance] `docs/compatibility.md` against origin/main**
- `git diff --quiet origin/main -- docs/compatibility.md` is exit 1 because plan 02 (already on this branch) changed the page (nine markdown entities). This plan changes neither the ledger nor the page: `git diff --quiet HEAD -- docs/compatibility.md tests/compat/ledger.csv` is exit 0 and the ledger against origin/main is exit 0.

**3. [Plan text] Stand-in reasons list three tokens, and the update also fails closed**
- The stand-in reasons file of `compat.ratchet` holds `graphic`, `mame_no` and `janggun` (the older cases name `janggun`); a retired copy marks `graphic` retired. The update mode treats an unreadable catalog as exit 2 (the plan names the ratchet only): the old behaviour silently discarded every candidate.

**4. [Plan text] The `export COMPAT_` line is its own line**
- `scripts/library.sh` keeps `export COGMASTER_CATALOG` and adds a separate `export COMPAT_CATALOG=... COMPAT_REASONS=...` line, so the acceptance `grep -c 'export COMPAT_'` matches.

**5. [Plan text] The R2 control is a fail row**
- For a base unsupported or skipped row the R3 check is stricter than R2, so R2 alone is observable only for a fail row; the controls are named accordingly.

**Total deviations:** 5 (all plan text or acceptance wording). **Impact:** none on scope; the ledger and page are unchanged.

## Known Stubs

None.

## Counted check code and docs bytes

- Counted check code: **876** (cap for this plan 882, at most 12 above 870; task 1 ended at 872, task 2 at 876, task 3 changed nothing). `scripts/compat-common.sh` is 126 lines; `src` is 9,403 lines (limit 940).
- Docs: no document changed. `docs/compatibility.md` 4,549 bytes; largest `docs/embedding.md` 36,521 (cap 38,912).
- Inventories unchanged (`ci.txt` 662, `wasm32.txt` 622, `hygiene.txt` 78): no test was added, only cases inside existing tests.
- Provisional timeout recorded: `compat.ratchet` 150 s (18 catalog cases ran in 7.5 s locally on BWK awk; plan 10 settles it from the hosted runs).

## Threat model

T-12-09 (catalog or reasons edited so an unsupported row passes) is mitigated by R1 and R3 needing the retired token and the catalog change together; T-12-10 (silent skip when data is unreadable) by the fail-closed loaders, with two controls; T-12-11 (a failure hidden by deletion) by R2, with a control. No package install, no new endpoint, no new file access beyond the two data files the judge is told about; no threat flags.

## Self-Check: PASSED

- Commits `3ca4984`, `c0bd7f5`, `f05302a` are ancestors of HEAD; all six files of the plan exist and carry the changes.
- The plan's verify commands ran green (see Evidence); the three tasks each left both presets green.
