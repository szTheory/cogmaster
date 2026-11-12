---
phase: 11-pal-and-japanese-fm
plan: "08"
subsystem: ledger
tags:
  - compat-update
  - library
  - ledger
  - pal
  - fm
status: complete

requires:
  - 11-07
provides:
  - a narrow adopt path in compat-update that adds a new key only for a catalog id, the catalog machine and mapper, boots at frame 600 under the default configuration, a pass or fail status and a profile the judge emits
  - a library judge that derives profiles from the catalog machine and the probe region, one count-only command, and a merge guarded by the ratchet and the docs cap
  - the fm_sweep_jp owned image (Master System Japan header code) and a synthetic library proof of four adopted rows, a held merge, no leaked names and an ignored results directory
  - REG-04 amended to what is built, and the owner's own-library run recorded as a seed
affects:
  - 11-09

actuals:
  tokens: 36000
  tasks: 3
  commits: 4

plan_head_before: bb0cab9240e3a435e91299b9cf87dd75f9518be9
plan_head_after: 1809f95a87b89f9a61db67eaa2fec294eb1483eb
commits: 4

tech-stack:
  added: []
  patterns:
    - "A public update adds a key only by a closed rule over catalog facts (id, machine, mapper) and a whitelist of judge profiles; everything else is discarded or refused, never fitted"
    - "A judge reads machine and mapper from the catalog and region from the probe, never from a file name, and is independent of fixture admission so an owned image can stand in for a catalog cart"
    - "A run that edits tracked files writes a candidate first, runs the ratchet and the page cap over it and only then copies; tests point it at copies"

key-files:
  created:
    - .planning/seeds/SEED-013-pal-fm-library-run.md
  modified:
    - scripts/compat-common.sh
    - scripts/library.cmake
    - scripts/library.sh
    - tests/library/tests.cmake
    - tests/compat/update.cmake
    - tests/roms/src/header.inc
    - tests/roms/manifest.txt
    - tests/built/expected.txt
    - tests/fixtures/tests.cmake
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/seeds/SEED-010-pad-device-notes.md
    - docs/conformance.md

key-decisions:
  - "The judge looks a cart up by the runner's identity SHA-1, else the headerless SHA-1, else the file SHA-1, because the probe of an owned image has no identity SHA-1; the lookup reads COGMASTER_CATALOG and ignores fixture admission"
  - "library.sh dedupes public-facts.csv by whole line before update, so two copies of one cart in a library cannot make update refuse on a duplicate key"
  - "A merge whose update or ratchet refuses exits 1 with 'merge: held (ratchet refused)' or '(update refused)'; a page at or over the cap holds with exit 0, matching the budget rule that a file equal to the cap is over"
  - "The regenerated page and ledger are copied only when they differ from the tracked files, so a run with nothing to add changes no tracked byte"

requirements-completed:
  - REG-04

coverage:
  - id: C1
    description: "compat-update adopts a new key only when the id is a catalog id, machine and mapper equal the catalog's, the predicate is boots@600 under the default configuration, the status is pass or fail and the profile is one the judge emits; the adopted row carries the catalog title and core evidence"
    requirement: REG-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^compat\\.public_update' exit 0: five adoptions (export ntsc, export pal, japan ntsc fm on, a Game Gear export, a Game Gear japan), each checked as the baseline plus exactly that row, all five in either order with equal bytes, the output a normalized ledger"
        status: pass
    human_judgment: false
  - id: C2
    description: "Sixteen discard cases leave the output equal to the baseline alone and together (frame, ram, mailbox, boots at 599 and 601, a non-default cfg, export with fm on, japan with fm off, machine and mapper mismatches, an unknown id, fixture and built ids), the four non-evaluated statuses on a valid new key are discarded, and audio on a catalog id, pal with gg or japan, and fm on with gg exit 2 with empty standard output"
    requirement: REG-04
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^compat\\.public_update' exit 0 (discard counter equals 16; impossible list of five outcomes equals 2 with empty output)"
        status: pass
    human_judgment: false
  - id: C3
    description: "The library judge adds exactly the profiles derived from the catalog machine and the probe region: export ntsc and export pal for an export Master System header, japan ntsc with fm on for a Japan header, one Game Gear row, and no other image adds a row; an image named with the wrong extension is judged by the catalog machine"
    requirement: REG-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^library\\.synthetic$' exit 0: vdp_lines_192 adds two rows, fm_sweep_jp (named .gg) one japan ntsc fm on row, boot_to_play.gg (named .sms) one gg export ntsc row, boot_to_play.sms none; the ledger gains exactly four lines"
        status: pass
    human_judgment: false
  - id: C4
    description: "The merge is applied to copies (four rows added, the ratchet exits 0, the regenerated page equals a fresh generation and is under the real cap) and held with both copies byte-identical when the cap is 100; the run prints library-judge, pal, fm and merge lines as counts only"
    requirement: REG-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^library\\.synthetic$' -V exit 0; the log holds 'merge: rows_added=4 ratchet=ok page_bytes=5718' and 'merge: held (page over the docs cap)'; git diff --quiet HEAD -- tests/compat/ledger.csv docs/compatibility.md exit 0 after the run"
        status: pass
    human_judgment: false
  - id: C5
    description: "Names planted in pieces (a home-path-shaped and an address-shaped string) in the image file names reach none of public-facts.csv, the printed output, results.csv, the merged ledger or the merged page; the results directory is ignored by git"
    requirement: REG-04
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^library\\.synthetic$' exit 0; git check-ignore build/library/results/1.path exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "fm_sweep_jp is built beside fm_sweep from one source with a Japan header code; two clean builds give equal digests; no existing image digest changed; the probe reports region code 3 and a Japan region guess"
    requirement: REG-04
    verification:
      - kind: integration
        ref: "shasum -a 256 of two clean builds equal (63aac9e9...de2be); git diff -U0 origin/main -- tests/built/expected.txt | grep -c '^-[^-]' prints 0; grep -c '^fm_sweep_jp ' tests/built/expected.txt prints 1; ctest -R '^fixtures\\.' 102 of 102"
        status: pass
    human_judgment: false
  - id: C7
    description: "REG-04 reads as built and is complete, the roadmap goal, criterion 4 and top line follow, Phase 12 criterion 5 gains the seed clause, SEED-013 holds the exact request, and docs/conformance.md says how the judge adds rows (+279 bytes, 32,217)"
    requirement: REG-04
    verification:
      - kind: integration
        ref: "grep counts of the plan's acceptance checks (REG-04 marked once, Complete row once, seed clause once, superseded Phase 11 wording absent); ctest -R '^(docs\\.|policy\\.)' exit 0; cmake --workflow --preset ci exit 0 (651 tests plus 5 release); cmake --workflow --preset hygiene exit 0 (72 tests)"
        status: pass
    human_judgment: false
  - id: owner-library-run
    description: "The owner runs sh scripts/library.sh run on their own library and pastes the library-judge block"
    requirement: REG-04
    verification:
      - kind: manual_procedural
        ref: SEED-013
        status: not_run
    human_judgment: true

duration: about 55 minutes
completed: 2026-11-11
---

# Phase 11 Plan 08: The library judge, the narrow adopt path and the owner's hand-off Summary

compat-update now adds a new row for a catalog cart only as `boots@600` under the default configuration on a profile the judge itself emits, the library judge derives those profiles from the catalog and the probe and merges only past the ratchet and under the docs cap, a synthetic library run proves it in CI, and REG-04 and the roadmap say what was built while the owner's real run waits in SEED-013.

## Accomplishments

- `scripts/compat-common.sh` (+7 counted lines): the catalog loader keeps machine and mapper beside the title; in the update stage a key already in the baseline keeps the equal-predicate rule and a new key is adopted only through the six conditions, then appended to the order array so the final sort places it.
- `tests/compat/update.cmake`: the impossible-profile controls stay and gain audio on a catalog id; five adoption cases compare the output with the baseline plus exactly the expected row (catalog title, status, `core=<rev>`, the profile fields); sixteen discard cases and four non-evaluated statuses leave the baseline intact, alone and together; the five adopted rows in either order give equal bytes and a normalized ledger.
- `scripts/library.cmake`: after the existing runs, a block looks the cart up in `COGMASTER_CATALOG`, takes machine and mapper from the catalog row and the region from the probe, derives one Game Gear profile, two export Master System profiles or one Japan profile with FM on, runs each as `boots@600` through the runner and appends a format 2 row to `public-facts.csv` and one name-free line to `profiles.csv`.
- `scripts/library.sh`: prints `library-judge 1`, the `pal:` and `fm:` count lines (or `skipped: not configured`) and one `merge:` line, also written to `summary.txt`; the merge writes a candidate ledger through compat-update, runs the ratchet, regenerates the page and copies both over the tracked paths only when the ratchet passes and the page is under the cap (`COGMASTER_LEDGER`, `COGMASTER_PAGE`, `COGMASTER_DOCS_CAP` and `COGMASTER_CATALOG` override the sources, so the test never touches a tracked file).
- `tests/roms/src/header.inc`, `tests/fixtures/tests.cmake`: `FIXTURE_REGION_CODE` and the `fm_sweep_jp` rule (one build rule, one `expected.txt` line, one manifest line).
- `tests/library/tests.cmake`: the synthetic proof (four owned images with a temporary catalog; the Game Gear image named `.sms` and the Japan image `.gg`; planted names built in pieces; applied and held runs on copies; the ignore control). Registration timeout 120 s with the provisional comment; 7.4 s locally.
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, SEED-010 (one added sentence), SEED-013 (new) and `docs/conformance.md` (the adopt-rule sentence replaces the old discard list wording).

## Evidence

All commands run from the repository root; exit statuses are the shell's.

- Task 1 verify: `cmake --preset ci` and `cmake --build --preset ci` exit 0; `ctest --test-dir build/ci -R '^(compat\.|policy\.compat|library\.)'` exit 0 (10 compat tests, 2 policy, 1 library); `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0.
- By hand (acceptance): a v2 candidate with one whitelisted new key (`export,pal`) through `sh scripts/compat-update.sh` exit 0, output holds the row once, `sh scripts/compat-common.sh normalize <out> | cmp - <out>` exit 0; a candidate with `export,ntsc` and `fm=on` through compat-update exit 0 and `sh scripts/compat-common.sh normalize tests/compat/ledger.csv | cmp - <out>` exit 0.
- Task 2 verify: `ctest --test-dir build/ci -R '^(library\.synthetic|compat\.|policy\.compat|fixtures\.)' -j4` exit 0; `ctest -R '^library\.synthetic$' -V` exit 0 and the log matches `rows_added=4` and `merge: held` (`grep -Eq` exit 0 each); `git check-ignore build/library/results/1.path` exit 0; `git diff --quiet HEAD -- tests/compat/ledger.csv docs/compatibility.md` exit 0 after the test; `git diff --quiet origin/main -- include src` exit 0.
- `git diff -U0 origin/main -- tests/built/expected.txt | grep -c '^-[^-]'` prints 0, `grep -c '^fm_sweep_jp ' tests/built/expected.txt` prints 1; the image was built, removed with its object and built again: `shasum -a 256` gave `63aac9e9ccec9eebaa8fb38ccf6e725e18bf8e05bbc15012a829dba4589de2be` both times.
- Final: `cmake --workflow --preset ci` exit 0 (651 tests plus 5 release) and `cmake --workflow --preset hygiene` exit 0 (72 tests), run after the last commit before this one. No hosted run (plan 09 dispatches).

### Negative controls (mutation by python, `git diff --stat` showed the edit, red, restore from a saved copy, green)

1. Adopt path, condition (c) dropped (`|| parameter!="boots@600"` removed): `ctest -R '^compat\.public_update'` exit 8, red at the existing hostile-row case (a `frame@600` row on the Game Gear catalog id is adopted); restored, exit 0. Narrowed to the plan's wording (`parameter!~/^boots@/`, any boots at any frame): exit 8 at the `boots@599` row; restored, exit 0.
2. Adopt path, whitelist dropped (`|| !index(w, ...)` removed): exit 8 at the export row with fm on; restored, exit 0.
3. Judge machine taken from the file extension: `ctest -R '^library\.synthetic$'` exit 8; the run itself exits 1 (the mutated Japan image, named `.gg`, becomes a Game Gear row with FM on, which the parser refuses, so the merge is held as refused and the test fails at the operation); restored, exit 0. The exact-key assertions were not reached by this mutation, they are the second line.
4. Merge without the cap test (`if [ "$bytes" -lt "$cap" ]` replaced by `if true`): exit 8 at the held assertion (the stand-in ledger changed); restored, exit 0.
5. Candidate path printed in the summary (`echo "$COGMASTER_ROM_DIR"` and `ls`): exit 8 at the privacy assertion; restored, exit 0.

## Budgets

- Counted check code: `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` printed 852 check lines before and prints `859 check lines / 9402 src lines` now: +7 (`scripts/compat-common.sh`, cap +12); the phase ceiling of 906 holds. `library.*`, `tests/` and the fixtures count 0.
- Docs: `docs/conformance.md` 31,938 to 32,217 bytes (+279, plan limit +300, file limit 33,000, docs cap 38,912); no other doc changed. `include/` and `src/` are byte-identical to origin/main. No C file was touched, so the gcc:13 container check does not apply.

## Deviations from Plan

**1. [Rule 1 - Bug] The judge cannot use the probe's `identity_sha1` for owned images**
- **Found during:** Task 2
- **Issue:** the plan says the judge looks up the probe's `identity_sha1`, but the probe leaves it empty for any image outside the built-in table, so an owned image could not stand in for a catalog cart.
- **Fix:** the lookup key is `identity_sha1`, else `sha1_headerless`, else `sha1`, read against `COGMASTER_CATALOG` (default `tests/compat/catalog.tsv`) and independent of fixture admission, as the plan's truth list says.
- **Files modified:** `scripts/library.cmake`
- **Commit:** f588ae2

**2. [Rule 2 - Missing critical] public-facts rows are deduplicated before update**
- **Found during:** Task 2
- **Issue:** two files of the same cart in one library append identical rows and compat-update refuses a duplicate key, so a real run could never merge.
- **Fix:** `awk '!seen[$0]++'` over `public-facts.csv` before the update; no counted line.
- **Files modified:** `scripts/library.sh`
- **Commit:** f588ae2

**3. [Rule 1 - Bug] `wc -c` padding on macOS put spaces in the printed byte count**
- **Fix:** the byte and line counts use shell arithmetic. Commit f588ae2.

**4. Stale acceptance count for REG-01 to REG-03**
- The plan expects `grep -c '^- \[ \] \*\*REG-0[123]\*\*'` to print 3; plans 04 and 05 already marked REG-01 and REG-03 complete, so it prints 1 (REG-02 alone, which plan 09 marks). Nothing was un-marked.

**5. The control the plan names for dropping condition (c) fails at an earlier case**
- Removing the exact-parameter test is caught first by the `frame@600` hostile row. The `boots@599` case is proven by the narrower mutation (accept any boots frame), recorded as control 1.

**6. A test addition not in the plan**
- `library.synthetic` prints the applied and held count blocks with `message(STATUS ...)` so the acceptance grep over `ctest -V` has something to read; commit 1809f95. The `ratchet` call inside the test uses an empty allow list and revision 16 for both sides.

## Known Stubs

None. `fm_sweep_jp` boots at 600 gives `fail` (a static display) in the synthetic run, which is a recorded status, not a stub.

## Threat Flags

None. The new surface (a catalog lookup and a merge into tracked files) is the one the plan's threat model names (T-11-28 to T-11-32), each with a control above.

## Issues Encountered

None blocking. `docs/compatibility.md` and `tests/compat/ledger.csv` stay as plan 05 left them: no real row is merged in this phase.

## Self-Check: PASSED

- Files: scripts/compat-common.sh, scripts/library.cmake, scripts/library.sh, tests/library/tests.cmake, tests/compat/update.cmake, tests/roms/src/header.inc, tests/built/expected.txt, .planning/seeds/SEED-013-pal-fm-library-run.md found.
- Commits e441ed2, f588ae2, 0efdf9b and 1809f95 are ancestors of HEAD.
