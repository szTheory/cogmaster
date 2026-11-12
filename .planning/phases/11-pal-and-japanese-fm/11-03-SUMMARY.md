---
phase: 11-pal-and-japanese-fm
plan: "03"
subsystem: compat
tags:
  - ledger
  - parser
  - migration
  - fm
  - audio
status: complete

requires:
  - 11-02
provides:
  - one ledger parser that reads format 1 or format 2 and writes format 2 only (tv column, export or japan region, fm token, audio predicate kind)
  - cases fields tv, fm, video, port1, port2, script and runner_region, read by the library judge and the fixtures check
  - compat.migration, the cross-format ratchet cases and the grammar stated once in docs/conformance.md
affects:
  - 11-04
  - 11-05
  - 11-08

actuals:
  tokens: 9200
  tasks: 3
  commits: 3

plan_head_before: a0e87e294560c1258a42d4250bf39b77df4b2454
plan_head_after: 617b89a93b00bccbb7c895407fc24e240d3d4f46
commits: 3

tech-stack:
  added: []
  patterns:
    - "The header line is the version: exactly two literals, every other header, field count or token exits 2"
    - "A format 1 row is mapped into the ten format 2 slots at read time, so no later code branches on the version"
    - "A literal compared in memory, never a file written with file(WRITE), proves a migration byte for byte"

key-files:
  created:
    - tests/compat/migration.cmake
  modified:
    - scripts/compat-common.sh
    - scripts/library.cmake
    - scripts/library.sh
    - tests/compat/tests.cmake
    - tests/compat/ratchet.cmake
    - tests/compat/update.cmake
    - tests/policy/repository-checks.txt
    - docs/conformance.md
    - docs/compatibility.md
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt

key-decisions:
  - "The header check compares the raw line with the two literals, so a quoted or reordered header exits 2; CRLF is still stripped before the comparison"
  - "A format 1 row is widened to ten slots immediately after the field-count check (region token to export or japan, tv inserted); every later line, the key, the stage logic, emit and the docs printer use the ten-slot positions"
  - "The default fm is resolved in predicate() in one place (japan and sms or sms1 gives on, otherwise off) and the cfg is rewritten to the long form, so an omitted fm and the explicit default share one key"
  - "audio is validated like frame (64 lowercase hex, F 1 to 1,000,000) and is then refused unless the cfg script is none and the id is fixture/ or built/"
  - "The docs printer prints the profile as machine/region/tv/mapper then fm=on or fm=off and nothing else changed, so the page differs from the old one only in its profile and predicate cells"

requirements-completed: []

coverage:
  - id: C1
    description: "One parser reads format 1 and format 2, prints format 2 only, resolves the default fm once, and refuses pal with the Game Gear or a Japanese console, fm on with the Game Gear, fm auto, ON, 1 and empty, an audio row with a 40-hex id or a script, and every wrong header, field count or token"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.migration$' exit 0 (26 controls); compat.csv, compat.device_key and compat.device_rejected exit 0"
        status: pass
    human_judgment: false
  - id: C2
    description: "The released five-row ledger normalizes to its hand-typed format 2 form byte for byte, format 2 is a fixed point, the preserved facts (id, machine, mapper, predicate without cfg, status, evidence, date) are equal before and after, and a flipped status or a dropped row is seen by the multiset check"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.migration$' exit 0; with the default fm rule changed to off for a Japanese Master System the same test exits 8 naming the legacy ntsc-j row, restored exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "A synthetic legacy file maps every old region token: ntsc-u and any to export,ntsc; ntsc-j with a Master System to japan,ntsc with fm on and with the Game Gear or SG-1000 to fm off; pal to export,pal; a quoted title and a device pair survive; the result is a fixed point"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.migration$' exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "A format 1 base against a format 2 head: the same row passes; a changed status, a dropped row, an altered value, a pal row turned ntsc and a Japanese row turned fm off each exit 2; a pal row kept as pal and a Japanese row kept with fm on pass"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.ratchet$' exit 0; with tv removed from the key the pal-to-ntsc cross-format comparison exits 0 instead of 2 and compat.ratchet exits 8, restored exit 0"
        status: pass
    human_judgment: false
  - id: C5
    description: "The audio kind is accepted as audio@F=64 lowercase hex on a fixture id and refused in every other form; fm identity separates rows differing only in fm and makes an omitted fm equal to the explicit default a duplicate"
    requirement: REG-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^compat\\.(csv|device_key|device_rejected)$' exit 0; removing the fixture-id rule exits 8 for compat.csv; widening the fm token grammar exits 8 for compat.device_rejected and compat.migration; both restored exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "Consumers read the parser's fields: no cfg regex remains in library.cmake or the fixtures check, the run passes --region runner_region --fm fm --video-mode video and both ports, public-facts.csv carries the format 2 header, and impossible profiles exit 2 with empty output from compat-update"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(compat\\.|library\\.|policy\\.compat)' exit 0 (compat.fixtures evaluates 4, library.synthetic, compat.public_update); grep -c 'cfg:video=(sms|gg)' scripts/library.cmake tests/compat/tests.cmake prints 0 for both"
        status: pass
    human_judgment: false
  - id: C7
    description: "The grammar is stated once in docs/conformance.md (1,296 bytes replacing the nine-column paragraph, file 30,081 bytes), docs/compatibility.md is regenerated from the unchanged ledger and policy.compat_doc_fresh is green, the committed ledger, include and src are untouched and the counted check code is 852"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md exit 0 (also under mawk in ubuntu:26.04); git diff --quiet origin/main -- include src tests/compat/ledger.csv exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 (852 check lines); cmake --workflow --preset ci exit 0 (628 tests); cmake --workflow --preset hygiene exit 0 (72 tests)"
        status: pass
    human_judgment: false

duration: about 25 minutes
completed: 2026-11-11
---

# Phase 11 Plan 03: Ledger parser, cases and migration proof Summary

The one ledger parser now reads format 1 or format 2 and always writes format 2 (a `tv` column, `export|japan` regions, an `fm` token in the cfg with its default resolved once, the `audio` predicate kind), the consumers read its `cases` fields instead of their own cfg regexes, and the migration is proved on embedded literals and across the ratchet while the committed ledger stays format 1.

## Accomplishments

- `scripts/compat-common.sh`: the header line is compared with exactly two literals; a format 1 row is widened to ten slots at read time (`ntsc-u` and `any` to export,ntsc, `ntsc-j` to japan,ntsc, `pal` to export,pal); format 2 requires `export|japan` and `ntsc|pal` and refuses every legacy token; `predicate()` takes the row's region, resolves `fm` (on only for a Japanese `sms` or `sms1`), rewrites the cfg to the long form and refuses an fm token under format 1 and `fm=on` with the Game Gear; `pal` with the Game Gear or a Japanese console is refused; `audio` is validated like `frame` and then refused unless the script is none and the id is `fixture/` or `built/`; one `emit()` writes the ten fields for normalize, results and update; `cases` adds `tv`, `fm`, `video`, `port1`, `port2`, `script` and `runner_region`; the docs printer prints `machine/region/tv/mapper fm=on|off`.
- `scripts/library.cmake` and the fixtures case of `tests/compat/tests.cmake` lost their cfg regexes: a case is unsupported unless its script is none (or its cap is over 600 in the judge), and the run passes `--region <runner_region> --fm <fm> --video-mode <video> --port1 --port2`. `scripts/library.sh` and `library.cmake` write `public-facts.csv` in format 2.
- `tests/compat/migration.cmake` (new, class behavior, 26 controls): the released ledger and its hand-typed format 2 form compared in memory; the fixed point; the preservation multiset with a flipped status and a dropped row; the synthetic legacy file; 24 refusals.
- `compat.csv` gained the audio forms, `compat.device_rejected` the four fm tokens (refused count 10), `compat.device_key` the fm identity; `compat.ratchet` gained eight format 1 versus format 2 cases; `compat.public_update` now asserts that four impossible profiles exit 2 with empty output.
- `docs/conformance.md` states the grammar once; `docs/compatibility.md` was regenerated and differs only in profile and predicate cells.

## Task Commits

1. Task 1, the two-format parser, the cases fields and the consumers: `0d29f94`
2. Task 2, `compat.migration`, the extended parser tests and the inventories: `e0e2750`
3. Task 3, the cross-format ratchet cases and the grammar: `617b89a`

## Evidence

All commands from the repository root on this Mac. No hosted run belongs to this plan.

Workflows after each task: `cmake --workflow --preset ci` exit 0 (627, 628 and 628 tests) and `cmake --workflow --preset hygiene` exit 0 (71, 72 and 72 tests). `ctest --test-dir build/ci -R '^(compat\.|library\.|policy\.compat|docs\.)'` exit 0 (23 tests).

Hand runs through throwaway v2 files, each `sh scripts/compat-common.sh normalize FILE`, exit 2 with `compat: invalid ledger record 2`: tv pal with gg; tv pal with region japan; fm on with gg; an audio row with a 40-hex id; an audio row with script input.txt; a legacy ntsc-u under the v2 header. An audio row on `fixture/a` exits 0 and keeps its value. `normalize` of the committed ledger exits 0, its first line is the format 2 header, and `normalize` of that output is byte for byte equal to it (`cmp` exit 0). `git diff --quiet origin/main -- include src tests/compat/ledger.csv` exit 0.

awk portability: with mawk 1.3.4 (`docker run --rm -v "$PWD":/w -w /w ubuntu:26.04 sh -c ...`) `normalize` exits 0, `docs | cmp - docs/compatibility.md` exit 0 and `cases` has the same md5 as on macOS (BWK awk) and in gcc:13 (mawk 1.3.4 20200120) and with gawk 5.3.2 installed in the ubuntu:26.04 container; `cmake -P` of migration.cmake (26 controls), ratchet.cmake, update.cmake, tests.cmake and the device_key case all exit 0 under mawk in that container.

Negative controls (python edit, `git diff --stat` showed one line, ctest, `git checkout -- scripts/compat-common.sh`, ctest again):

- default fm for a Japanese Master System changed to off: `ctest -R '^compat\.migration$'` exit 8 with "legacy row fixture/t_j_sms: a legacy ntsc-j Master System row is japan,ntsc with fm on"; restored exit 0.
- the fixture-or-built id rule removed from the audio check: `compat.csv` exit 8; restored exit 0.
- the fm token grammar widened to any word: `compat.device_rejected` exit 8 and `compat.migration` exit 8; restored exit 0.
- `tv` removed from the key: `compat.ratchet` exit 8 (first at the older pal case); by hand, base `sms,pal,pass` against a head `export,ntsc` row: `sh scripts/compat-ratchet.sh ...` exit 2 before, exit 0 with the mutation, exit 2 restored. This is the cross-format pal-to-ntsc comparison the new case guards.

Budgets: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, 852 check lines and 9,402 src lines (845 before: scripts/compat-common.sh 95 to 102 lines, +7 against the cap of +36; absolute ceiling 884). Docs bytes: docs/conformance.md 29,837 to 30,081 (cap 30,800; the new paragraph is 1,296 bytes, cap 1,300); docs/compatibility.md 2,411. Planning-identifier grep over added docs lines prints 0.

Inventories: `git diff --numstat origin/main -- tests/inventory/ci.txt tests/inventory/wasm32.txt tests/inventory/hygiene.txt` shows 1 added and 0 deleted line in each. `cmake --preset wasm32` and `cmake --preset be32` cannot configure here (no compiler for those toolchains), so the wasm32 inventory is proved at the first hosted run; the line has the same shape as its ci.txt twin.

## Deviations from Plan

### Interpretations (no behaviour lost)

- **Moved into this plan.** library.sh, library.cmake, update.cmake and docs/compatibility.md changed here because the parser's new `cases` output and refusals force them (as the plan's output section states); format 1 rows carrying an fm token and format 1 rows with an `export` or `japan` region are refused, the planner's reading of the format rules.
- **Quoted header refused.** The plan's truth list says a quoted header exits 2; the parser compares the raw line with the literals, which also removes the old field-count check on the header row. No existing test used a quoted header.
- **Mutation evidence for task 1.** The plan allowed deferring the default-fm mutation to task 2's migration literal because no task 1 test exercises a Japanese Master System row; it was recorded there (exit 8, then 0).
- **Preservation test shape.** The facts multiset is computed in CMake by a small regex field splitter (semicolons become text first so a cfg cannot split a list); the plan left the shape open.
- **device_rejected fm family uses the format 2 header.** Under the format 1 header any fm token is refused for the header, so the four tokens would have passed for the wrong reason.
- **Task 3 control order.** With tv removed from the key the older `protected ntsc-u|pal` case fails first in `compat.ratchet`; the new cross-format pal-to-ntsc case was shown separately by hand (exit 2, 0, 2).
- **No `state advance-plan`.** It fails with `plan_position_unreadable`; the Plan line was set by hand as plans 11-01 and 11-02 did.

**Total deviations:** 0 rule-driven fixes; 7 interpretations above. **Impact:** none on the plan's outcome.

## Known Stubs

None.

## Threat Flags

None. T-11-08 (two header literals, every other header, count or token exits 2) is covered by the 24 refusals of `compat.migration`; T-11-09 (omitted versus explicit default fm) by the duplicate controls in `compat.migration` and `compat.device_key`; T-11-10 (audio on catalog ids) by `compat.csv`; T-11-11 (cases fields) by the unchanged regex validation before printing and the hostile-title cases; T-11-12 (a migration moving a status) by the literal comparison, the preservation controls and the cross-format ratchet cases.

## Notes for the next plans

- Plan 04 can run `sh scripts/compat-common.sh normalize tests/compat/ledger.csv > new && mv new tests/compat/ledger.csv`, add the live-file assertions to `compat.migration` (header, `normalize(live) == live`), move `ratchet.cmake` and `update.cmake` to the v2 header, and extend the docs printer's intro. The `audio` predicate count line was left out of the page on purpose (the page may differ only in profile cells until the intro is rewritten).
- Plan 05 reads the execution line `region= tv= fm=` and adds the five `audio@300` rows; `compat.fixtures` still evaluates 4.
- Plan 08's adopt path needs the (machine, region, tv, fm) whitelist in `predicate()`'s caller; `library.cmake` already passes `--fm` and the public-facts tv column.
- `compat.migration` carries a provisional 60 s timeout (about 1 s locally); plan 09 settles it.

## Self-Check: PASSED

- Commits `0d29f94`, `e0e2750` and `617b89a` are ancestors of HEAD (`git rev-list --count a0e87e2..HEAD` prints 3) and each message ends with the attribution trailer.
- `tests/compat/migration.cmake` exists, is classified in `tests/policy/repository-checks.txt` once, and `compat.migration` appears in tests/inventory/ci.txt, wasm32.txt and hygiene.txt.
