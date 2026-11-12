---
phase: 11-pal-and-japanese-fm
plan: "05"
subsystem: compat
tags:
  - ledger
  - audio
  - identity
  - docs
status: complete

requires:
  - 11-04
provides:
  - five own-fixture audio@300 ledger rows whose hashes were copied by script from the frozen transition pins
  - compat.fixtures judging nine own-fixture predicates and checking each row's region, tv and fm against the runner's execution line, plus the core default against the awk default, with controls
  - ratchet audio cases and a page that counts audio predicates and pal rows judged
  - the commercial-digest sentence in the asset policy and the regression-lock wording in the conformance guide
affects:
  - 11-07
  - 11-08
  - 11-09

actuals:
  tokens: 9000
  tasks: 2
  commits: 2

plan_head_before: 674edfdd67ce4473201ccd339f864508f036628e
plan_head_after: 176b417755e7f384e3d015d2b6f33e5a4d6e0e5e
commits: 2

tech-stack:
  added: []
  patterns:
    - "A profile check compares a printed execution line with literal raw-value tables typed in the test, never with the awk or the runner that produced it"
    - "A default that lives in three places is compared in one place: the awk default read through normalize on a ledger that omits it, the core default read from a run without the flag, and the explicit ledger spelling"

key-files:
  created: []
  modified:
    - tests/compat/ledger.csv
    - tests/compat/tests.cmake
    - tests/compat/ratchet.cmake
    - scripts/compat-common.sh
    - docs/compatibility.md
    - docs/conformance.md
    - ASSET_POLICY.md

key-decisions:
  - "Hashes were taken by a throwaway script from tests/audio/transitions.txt (last field of the five named lines); no digit was typed, and a grep loop shows each ledger hash present in that file"
  - "The two export rows spell fm=off and the two japan rows omit fm in the input, so both default branches of the awk are exercised before normalize writes the long form; the committed file is the normalize output"
  - "compat.fixtures reads the awk default by normalizing a temp two-row ledger that omits fm, since the committed ledger spells fm on every row and cannot reach the awk default"
  - "The three identity controls each flip exactly one of region, tv and fm on one real run output"
  - "The page counts pal rows judged by merging the count into the existing predicate loop and print, adding no counted line"

requirements-completed:
  - REG-03

coverage:
  - id: C1
    description: "Five audio@300 rows join the ledger on fixture/psg_sweep (export ntsc fm=off, export pal fm=off, japan ntsc fm=on) and fixture/fm_sweep (japan ntsc fm=on, export ntsc fm=off), status pass, each hash equal to its transitions.txt line, the ledger a normalize fixed point"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "grep -c 'audio@300=' tests/compat/ledger.csv prints 5; for each of the five ledger hashes grep -c of the same 64 characters in tests/audio/transitions.txt prints at least 1 (loop exit 0); sh scripts/compat-common.sh normalize tests/compat/ledger.csv piped to cmp against it exit 0; ctest -R '^compat\\.migration$' exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "compat.fixtures evaluates nine own-fixture predicates; each run passes --region and --fm and the execution line region, tv and fm equal the row's identity through literal raw-value tables"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.fixtures$' -V exit 0, output matches 'evaluated 9 ' and 'identity controls: 3'"
        status: pass
    human_judgment: false
  - id: C3
    description: "A run without --fm for a japan and an export psg_sweep row prints the core default (fm=2 for japan, fm=1 for export), equal to the awk default read by normalizing a temp ledger whose cfg omits fm"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "same compat.fixtures run exit 0; negative control: the awk default flipped so a japan row defaults off makes ctest -R '^compat\\.migration$' exit 8 and ctest -R '^compat\\.fixtures$' exit 8 naming 'identity mismatch default-japan', restored exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "The identity check is not vacuous: copies of a real run output compared with a flipped fm, a flipped region and a flipped tv each report a mismatch; a japan fm_sweep row with fm=on changed to fm=off is rejected by the fixtures judge"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "identity controls: 3 in the compat.fixtures output; the mutated ledger row (python, git diff --stat) makes ctest -R '^compat\\.fixtures$' exit 8 with 'ledger disagrees with fresh fixture predicate fixture/fm_sweep: exit 3', restored exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "compat.ratchet gains six audio cases at format 2: altered value exit 2, dropped row exit 2, a twin differing only in fm, only in tv, and in region and fm each exit 0 as an adjacent key, a pal audio row stays protected exit 2"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet$' exit 0; with tv dropped from the awk key compat.ratchet exits 8 ('protected export,ntsc|export,pal: expected exit 2, actual 0'), restored exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "The generated page counts five audio predicates and prints how many rows were judged under PAL, is fresh, and is identical under mawk; the counted check lines stay at 852 against a ceiling of 858 for this plan"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "sh scripts/compat-common.sh docs tests/compat/ledger.csv piped to cmp against docs/compatibility.md exit 0 and inside ubuntu:26.04 (mawk) exit 0; grep -c 'audio predicates: 5' docs/compatibility.md prints 1; ctest -R '^policy\\.compat' exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (852 check lines)"
        status: pass
    human_judgment: false
  - id: C7
    description: "ASSET_POLICY.md says a digest of a commercial title's emulated frame, RAM or sound is never committed and ledger rows for such titles carry boots predicates only; docs/conformance.md says an audio row is a regression lock and names what proves the sound; no planning identifier is added; include, src and tests/audio are unchanged"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest -R '^(policy\\.|docs\\.)' exit 0 (57 tests); grep -c 'compat-update never adds a digest' ASSET_POLICY.md prints 1; git diff --numstat origin/main -- ASSET_POLICY.md shows 2 added and 0 deleted; wc -c docs/conformance.md 30,977 (cap 31,900); git diff --quiet origin/main -- include src tests/audio exit 0; cmake --workflow --preset ci exit 0 (628 tests); cmake --workflow --preset hygiene exit 0 (72 tests)"
        status: pass
    human_judgment: false

duration: about 25 minutes
completed: 2026-11-11
---

# Phase 11 Plan 05: Audio rows and the identity check Summary

Five own-fixture `audio@300` rows now sit in the ledger, and the fixtures judge proves that each row's region, tv and fm equal what the runner actually ran, and that the three places which decide the default `fm` (the awk, the core, the runner) agree. REG-03 is complete.

## Accomplishments

- `tests/compat/ledger.csv`: five rows added by a throwaway script kept in the session scratch directory (not committed). It took the last field of the five named lines of `tests/audio/transitions.txt`, wrote format 2 rows (export rows with `fm=off` spelled, japan rows with `fm` omitted), and `sh scripts/compat-common.sh normalize` produced the committed file. Mapper is `none`, the word `cogmaster-run --probe` prints as `mapper_guess` for both built images. Evidence text `regression pin over a transition stream, no sound claim`, date 2026-11-11, status pass.
- `tests/compat/tests.cmake`: `evaluated 9`; a function `identity` compares the execution line `region= tv= fm=` with literal tables typed in the test; a no-flag run for the first japan and the first export `psg_sweep` row compares the core default with the awk default read from a temp ledger that omits `fm`; three controls flip fm, region and tv on a real output and must report a mismatch (printed `identity controls: 3`). The test timeout moved from 30 to 60 with the provisional comment (2.5 s slowest local run).
- `tests/compat/ratchet.cmake`: six audio cases (below).
- `scripts/compat-common.sh`: the docs printer prints `pal rows judged: N` by extending the existing predicate loop and print; no counted line was added.
- `docs/compatibility.md` regenerated (4,489 bytes): Rows 10, Evaluated coverage 9, Own fixtures 4, audio predicates 5, pal rows judged 1.
- `ASSET_POLICY.md`: one bullet of two lines after the traces bullet. `docs/conformance.md`: one paragraph of 420 bytes.

## Task Commits

1. Task 1, the five rows, the identity check, ratchet audio cases and page counts: `033bc96`
2. Task 2, the commercial-digest sentence and the regression-lock wording: `176b417`

## Evidence

All commands from the repository root; scratch files under the session scratch directory; no log committed. No hosted run belongs to this plan (the dispatch is in plan 07).

Hash equality: `grep -c 'audio@300=' tests/compat/ledger.csv` prints 5. The loop `for h in $(grep -o 'audio@300=[0-9a-f]\{64\}' tests/compat/ledger.csv | cut -d= -f2); do grep -c "$h" tests/audio/transitions.txt; done` printed 1, 1, 2, 1, 1 (the export ntsc `psg_sweep` digest `d195b6d2...` appears on two lines, for `sms` and `gg-sms`, which is why the profile lives in the row) and exited 0.

Ledger and page: `sh scripts/compat-common.sh normalize tests/compat/ledger.csv | cmp - tests/compat/ledger.csv` exit 0; `sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md` exit 0; the same two inside `docker run --rm -v "$PWD:/src" -w /src ubuntu:26.04 sh -c '...'` (mawk) exit 0.

Verify command: `cmake --preset ci > /dev/null && cmake --build --preset ci && ctest --test-dir build/ci -R '^(compat\.|policy\.compat|runner\.hashes_file)' --output-on-failure && cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0 (11 tests passed, 852 check lines / 9,402 src lines). `cmake --workflow --preset ci` exit 0 (628 tests passed, 5 release tests) and `cmake --workflow --preset hygiene` exit 0 (72 tests) after task 2. `git diff --quiet origin/main -- include src tests/audio` exit 0.

compat.ratchet audio cases and their asserted exits (base is an embedded audio row on a fixture id, hash 64 of one letter, never a real digest):

| Case | Change from the base row | Asserted exit |
|---|---|---|
| audio_value_altered | hash replaced | 2 |
| audio_row_dropped | head has no rows | 2 |
| audio_adjacent_fm | base plus a twin with fm=on | 0 |
| audio_adjacent_tv | base plus a twin at pal | 0 |
| audio_adjacent_japan | base plus a twin at japan and fm=on | 0 |
| audio_pal_value_altered | a pal row, hash replaced | 2 |

Negative controls (python edit, `git diff --stat`, ctest status, restore, ctest status):

1. Fixtures judge: the `fixture/fm_sweep` japan row with `fm=on` changed to `fm=off` in the ledger (the edit was made against the uncommitted five-row ledger; the restore was a copy of the saved file): `ctest -R '^compat\.fixtures$'` exit 8 with `ledger disagrees with fresh fixture predicate fixture/fm_sweep: exit 3` (the unit off cannot give the pinned stream); restored, exit 0.
2. Default fm: `?"on":"off"` changed to `?"off":"off"` in the awk default line of `scripts/compat-common.sh` (`git diff --stat`: 3 insertions, 3 deletions including the page line and this one): `ctest -R '^compat\.migration$'` exit 8; `ctest -R '^compat\.fixtures$'` exit 8 with `identity mismatch default-japan: row japan ntsc off, run 2 1 2`; restored, both exit 0. As the plan says, the committed ledger spells fm on every row, so only these two tests catch this.
3. Ratchet key: `tv` dropped from the awk key: `ctest -R '^compat\.ratchet$'` exit 8 (`protected export,ntsc|export,pal: expected exit 2, actual 0`); restored exit 0.

Budgets: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, **852 check lines** and 9,402 src lines (unchanged from the end of plan 04; this plan added 0 counted lines against its cap of +6; ceiling 858 and absolute 894). Docs bytes: conformance.md 30,557 to 30,977 (+420, paragraph cap 550, file cap 31,900), compatibility.md 2,758 to 4,489, ASSET_POLICY.md +2 lines and 0 deleted (+226 bytes, sentence cap 400). Planning-identifier grep over added lines in docs and the policy prints 0.

## Deviations from Plan

### Interpretations (no behaviour lost)

- **Page counts.** The audio predicate count already existed from plan 04, so only the `pal rows judged` line is new; it was merged into existing counted lines, 0 added.
- **Ratchet case set.** The fifth listed case ("a pal row stays protected") is a pal audio row with an altered hash; the japan twin is a sixth case in the same six-invocation cap.
- **Control scope.** Each of the three identity controls flips exactly one field on a real run output, so each field has its own failing control.
- **Row id parse.** `compat.fixtures` selects the first japan and first export-ntsc audio row of `psg_sweep` by scanning the sorted cases; it fails if either is missing, so a removed row cannot silently skip the default check.
- **No `state advance-plan`.** The Plan line was set by hand as in plans 11-01 to 11-04.

**Total deviations:** 0 rule-driven fixes; 5 interpretations. **Impact:** none.

## Known Stubs

None.

## Threat Flags

None beyond the register. T-11-16 (audio only on fixture ids; the policy sentence), T-11-17 (identity check, no-flag run, three controls, two recorded negative controls) and T-11-18 (hashes copied by script, grep loop) are covered above; T-11-SC accepted (no install).

## Phase notes

- The no-flag identity check reads the pinned hashes from `tests/audio/transitions.txt` at test time, so a regeneration of those pins moves the check and the ledger rows together only through a recorded ledger edit.
- `compat.fixtures` now makes seven more runs than before (eleven in all); slowest local run 2.5 s; plan 09 settles the 60 s timeout from hosted durations.

## Self-Check: PASSED

- Commits `033bc96` and `176b417` are ancestors of HEAD (`git rev-list --count 674edfd..HEAD` printed 2) and each message ends with the attribution trailer.
- `tests/compat/ledger.csv`, `tests/compat/tests.cmake`, `tests/compat/ratchet.cmake`, `docs/compatibility.md`, `ASSET_POLICY.md` exist; `grep -c 'audio@300=' tests/compat/ledger.csv` prints 5.
