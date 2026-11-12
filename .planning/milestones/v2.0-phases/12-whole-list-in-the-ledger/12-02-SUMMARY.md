---
phase: 12-whole-list-in-the-ledger
plan: "02"
subsystem: compat-parser
tags:
  - compat
  - ledger
  - reasons
  - hardening
  - test-cost
status: complete

requires:
  - 12-01
provides:
  - The closed reason rule in every mode of the shared parser (unsupported or skipped needs reason=<listed token>)
  - tests/compat/reasons.txt, the reviewed vocabulary, and compat.reasons tying it to the catalog and the slot table
  - Title hardening (format characters, formula first byte), the exact 8,192-row cap, and one-pass markdown escaping of every active character
  - COMPAT_LEDGER and COMPAT_CATALOG overrides and stand-ins so no test parses the live ledger per call
affects:
  - 12-03
  - 12-04
  - 12-05
  - 12-06
  - 12-07

actuals:
  tokens: 4800
  tasks: 3
  commits: 4

plan_head_before: a9f3b11
plan_head_after: 9320b59badb694f53e8b00a9e07d9b30d7aee6ee
commits: 4

tech-stack:
  added: []
  patterns:
    - "A lazily loaded vocabulary file: the parser reads reasons.txt on the first unsupported or skipped row only, so a ledger without those statuses never reads it"
    - "Markdown escaping walks the string once and looks each byte up in one table, so an entity made by an earlier step is never escaped again"
    - "Octal byte escapes under LC_ALL=C for the format characters, identical on BWK, mawk and gawk awk"
    - "Stand-in ledgers: a one-regex extract of the fixture rows (plus one typed catalog row) replaces a copy of the live ledger in the slow-lane tests"

key-files:
  created:
    - tests/compat/reasons.txt
  modified:
    - scripts/compat-common.sh
    - scripts/compat-update.sh
    - tests/compat/tests.cmake
    - tests/compat/update.cmake
    - tests/compat/ratchet.cmake
    - tests/compat/migration.cmake
    - tests/library/tests.cmake
    - docs/compatibility.md
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt

key-decisions:
  - "The title rule refuses = + @ - only when another byte follows, because the library judge writes a lone dash as the title of every fact it hands to the update (the catalog title replaces it); a formula needs a second byte"
  - "md() is one pass over the title with a single lookup table, not a chain of substitutions, so the numeric entities of the new characters cannot be escaped twice and the three awks agree"
  - "compat.reasons runs its controls in process on throwaway copies of reasons.txt and reuses the answers of its two live awk spawns, so a control costs no extra process"
  - "The vocabulary check compares the open non-catalog tokens with the board-0 non-SG-1000 slot names, so retiring a token (a board arrives) needs the slot table to change in the same commit"

requirements-completed: []

coverage:
  - id: C1
    description: "A row whose status is unsupported or skipped must carry evidence reason=<token> with a token of tests/compat/reasons.txt, in every mode of the parser; free text, an empty token, a wrong case and an unlisted token each exit 2"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case csv (compat.csv): 2 statuses x 7 evidence forms refused, listed token accepted in normalize, docs and cases"
        status: pass
    human_judgment: false
  - id: C2
    description: "A ledger with no unsupported or skipped row never reads the reasons file (the file path points at a missing file and the parse exits 0)"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case csv (compat.csv), lazy-load control"
        status: pass
    human_judgment: false
  - id: C3
    description: "reasons.txt lists the eleven opening tokens; every token the catalog uses is listed and open, and the open slot reasons equal the board-0 non-SG-1000 slots of the cartridge table"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case reasons (compat.reasons), controls: graphic retired, xterminator missing, no_rom missing"
        status: pass
    human_judgment: false
  - id: C4
    description: "A title with a bidirectional or zero-width format character, a byte order mark or a formula first byte exits 2; the characters just outside each range, accents, quotes, commas and parentheses still parse"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case hardening (compat.hardening)"
        status: pass
    human_judgment: false
  - id: C5
    description: "The row cap is exactly 8,192 data rows: 8,192 are accepted and the 8,193rd exits 2; the committed ledger is accepted and its headroom is printed"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case hardening (compat.hardening), log line compat.hardening: live rows N of 8192"
        status: pass
    human_judgment: false
  - id: C6
    description: "The page escapes & < > | and also backslash backtick * _ ~ [ ] ( ) ! # as numeric entities, each source character once; the regenerated page differs from origin/main only by the nine added entities"
    requirement: LEDG-02
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case hardening (md case) and policy.compat_doc_fresh"
        status: pass
      - kind: other
        ref: "sed undo of the nine entities | cmp against origin/main page: exit 0"
        status: pass
    human_judgment: false
  - id: C7
    description: "BWK awk, mawk 1.3.4 and gawk 5.3.2 give identical bytes: the same 22 title verdicts, the same page, the same 8,192-row normalization and the same refusal at 8,193"
    requirement: LEDG-02
    verification:
      - kind: other
        ref: "docker run ubuntu:26.04 sh with mawk and with apt-installed gawk, and the system BWK awk, over the same files; md5 sums and exit statuses equal"
        status: pass
    human_judgment: false
  - id: C8
    description: "The update, fixture, migration and library tests no longer parse the live ledger per call: stand-ins and one extract replace it, the cost of compat.public_update is flat in the number of ledger rows, and a missing stand-in fails the test"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "tests/compat/update.cmake, tests/compat/tests.cmake case fixtures, tests/compat/migration.cmake, tests/library/tests.cmake; ctest -R '^(compat|policy\\.compat|library)' exit 0"
        status: pass
    human_judgment: false
---

# Phase 12 Plan 02: Reason rule, input hardening and flat-cost test plumbing Summary

The shared parser now refuses an unsupported or skipped row that names no listed reason, refuses spoofing and formula titles, caps a ledger at exactly 8,192 rows and escapes every active markdown character once; and the update, fixture, migration and library tests read ten-row stand-ins, so their cost no longer grows with the ledger. The committed ledger is byte-identical to origin/main.

## Performance

- **Tasks:** 3 of 3, plus one follow-up commit (a quote-free awk argument for Windows)
- **Files changed:** 12 (one new)
- **Commits:** `81c544a` (task 1), `57448c9` (task 2), `1a964d0` (task 3), `9320b59` (follow-up)

## Accomplishments

1. **Reason rule (task 1, the tracer).** `scripts/compat-common.sh` requires `^reason=[a-z0-9_]+$` and a listed token for `unsupported` and `skipped` in every mode; the file is loaded lazily on the first such row. A prelude line gives `COMPAT_CATALOG` and `COMPAT_REASONS` their working-directory-relative defaults. `tests/compat/reasons.txt` holds the eleven opening tokens, all open. The literal rows of the older compat tests moved to `reason=janggun`, and each script exports the two paths absolutely so no case depends on the working directory.
2. **`compat.reasons`.** Two live awk spawns (the catalog and `src/cartdb.inc`, no quote character in either program) feed an in-process verdict: every catalog token listed and open, the five pin reasons listed, the open slot reasons equal to the six board-0 non-SG-1000 slot names. Three controls on throwaway copies: graphic retired, xterminator missing, no_rom missing.
3. **Hardening (task 2).** Octal byte regex under `LC_ALL=C` for U+200B to U+200F, U+202A to U+202E, U+2066 to U+2069 and U+FEFF, plus a formula first byte; row cap `FNR>8193`; `md()` rewritten as a single pass over one lookup table. `compat.hardening` covers eight hostile titles, four accepted neighbours (including the characters just outside each range and a lone dash), the 8,192 and 8,193 row boundary, the escaping of every active character and the live ledger headroom (`compat.hardening: live rows 10 of 8192`). The page was regenerated with the generator command.
4. **Flat-cost plumbing (task 3).** `compat-update.sh` honours `COMPAT_LEDGER` and `COMPAT_CATALOG` (today's paths by default; still four lines). `update.cmake` builds `standin.csv` once with one `file(STRINGS)` regex. The fixture case reads a fixture extract and compares its count with an independent awk count of the live file. `migration.cmake` normalizes the live ledger once and damages only the header and first row. `library.synthetic` builds its stand-in ledger and page from the fixture rows and one typed Crazy Faces row, generates the page through `-E env COMPAT_CATALOG=<its temporary catalog>`, and compares every "tracked" check against the stand-in variables.

## Evidence (commands, exit statuses; no log committed)

- `cmake --workflow --preset ci`: exit 0 (655 tests plus 5 release tests) after tasks 1, 2 and 3; `cmake --workflow --preset hygiene`: exit 0 (76 tests) after tasks 1 to 3.
- `ctest --test-dir build/ci -R '^(compat|policy\.compat|library)' --output-on-failure`: exit 0 (13 tests). `ctest --preset ci -R '^(compat|library)'`: exit 0.
- `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, prints `869 check lines / 9403 src lines`.
- `git diff --quiet origin/main -- tests/compat/ledger.csv`: exit 0 (unchanged).
- Page: `sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md` exit 0; the nine-entity `sed` undo of the page `| cmp - <origin/main page>` exit 0; `git diff --quiet origin/main -- docs/compatibility.md` exit 1 (changed, as expected); `grep -c '&#95;'` prints 9, `grep -c '&#40;'` prints 1.
- By hand through throwaway files (repository root, system awk): unsupported with evidence `janggun`, `reason=`, `reason=Janggun`, `reason=unlisted_token` and an empty cell: exit 2 with `invalid ledger record`, for both statuses; `reason=janggun`: exit 0; a pass-only ledger with `COMPAT_REASONS=/nonexistent`: exit 0; a skipped row with the same missing path: exit 2.
- Awk portability: the same 22-title script (`hostile.sh`, kept in the session scratch, not committed) gives 22 of 22 expected exits under macOS BWK awk 20200816, under mawk 1.3.4 (`docker run --rm ... ubuntu:26.04`) and under gawk 5.3.2 (the same image after `apt-get install gawk`). `sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md` exit 0 under all three; the docs output of the escaping ledger has the same md5 (`4627bf0d...`) and the normalization of the 8,192-row file the same md5 (`ac63c999...`); the 8,192-row file exits 0 and the 8,193-row file exits 2 under all three.
- Flat cost: `cmake -P tests/compat/update.cmake` run from a detached worktree with a 1,472-row ledger: origin/main 12.9 s, this branch 2.2 s. On the 10-row ledger, four interleaved runs each: origin/main 2.40 to 3.76 s, this branch 2.24 to 3.62 s (the ctest wall time of `compat.public_update` at HEAD is 3.2 to 3.4 s on this loaded machine).
- Hosted dispatch: none in this plan (the first is after plan 5).

## Negative controls (each: python edit, rebuild, red, restore, green)

| Control | Mutation | Result |
|---|---|---|
| Reason pattern | `if(f[9] !~ /^reason=.../) bad()` made `if(0) bad()` | `ctest -R '^compat\.(csv\|reasons)$'` exit 8; restored exit 0 |
| Token lookup | `if(!(substr(f[9],8) in ok)) bad()` made `if(0) bad()` | same ctest exit 8; restored exit 0 |
| Retired token | `graphic open` became `graphic retired` in reasons.txt | `compat.reasons` exit 8; restored exit 0 |
| Bidi range | `\252-\256` became `\252-\255` | `compat.hardening` exit 8; restored exit 0 |
| Row cap | `FNR>8193` became `FNR>4097` | `compat.hardening` exit 8; restored exit 0 |
| Markdown brackets | `[]` in the lookup string became `{}` | `compat.hardening` exit 8; restored exit 0 |
| Formula first byte | first-byte clause removed | `compat.hardening` exit 8; restored exit 0 |
| Missing stand-in | `COMPAT_LEDGER` pointed at a missing file in `update.cmake` | `compat.public_update` exit 8; restored exit 0 |
| Fixture extract | regex narrowed to `fixture/[a-o]` | `compat.fixtures` red (`extract has 5 rows but the ledger holds 9`); restored green |

## Live-ledger readers per lane (D13: at most 8)

| Test | Reads of the live ledger |
|---|---|
| `policy.compat_doc_fresh` | 1 parser spawn (docs) |
| `policy.compat_doc_fresh.self_test` | 1 parser spawn (docs) |
| `compat.migration` | 1 parser spawn (normalize) plus one 4 KiB read |
| `compat.hardening` | 1 parser spawn (normalize) |
| `compat.fixtures` | 1 awk spawn (fixture line count) plus one `file(STRINGS)` extract, no parser spawn |
| `compat.public_update`, `library.synthetic` | one `file(STRINGS)` extract each, no spawn |

Sum of spawns that read the whole live ledger: 5 (4 parser, 1 awk). The cost of every other ledger-touching test is flat.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The first-byte rule broke the library judge's facts**
- **Found during:** Task 2 (`library.synthetic` failed after the title rule landed)
- **Issue:** `scripts/library.cmake` writes a lone dash as the title of every fact it hands to the update (the catalog title replaces it). A rule refusing every first byte in `= + @ -` refused all of them, so a library run would have held its merge.
- **Fix:** the rule is `^[=+@-].` (the character and at least one more byte), which still refuses every title that can be a formula; `compat.hardening` gained a control that a lone dash parses. `library.cmake` is untouched.
- **Files modified:** `scripts/compat-common.sh`, `tests/compat/tests.cmake`
- **Commit:** `57448c9`

**2. [Rule 3 - Blocking] The old 4,097-row control moved**
- **Found during:** Task 2
- **Issue:** `compat.csv` refused 4,097 data rows; with the cap at 8,192 that input is valid.
- **Fix:** the case was removed from `compat.csv` and replaced by the exact 8,192 accepted / 8,193 refused pair in `compat.hardening`.

**3. [Plan text] Controls of `compat.reasons` run in process, and a third control was added**
- **Issue:** the plan says each control asserts exit 1. A spawned process per control would add spawns on the slowest lanes for no extra proof, so the controls call the same verdict function on throwaway copies and assert its message names the token. A third control (a pin reason removed) was added because the five pin reasons are part of the equality.

**4. [Plan acceptance] Inventory line counts against origin/main**
- `git diff --numstat origin/main` shows 3 added lines in `ci.txt` and `wasm32.txt` and 2 in `hygiene.txt`: the extra line in the first two is `cart.generator.catalog` from plan 01 (already on this branch). This plan added exactly two lines to each of the three files.

**5. [Rule 3 - Blocking] `compat-update.sh` keeps four lines by sharing the `set -eu` line**
- The overrides need the repository root twice; it is assigned on the `set -eu` line (`set -eu; top=...`) so the counted total does not rise. The script also defaults `COMPAT_REASONS` to the root file so the update stage finds it from any directory.

**6. [Plan text] The Windows quoting risk of an awk argument**
- The fixture count awk first held escaped double quotes; commit `9320b59` replaced it with `/^\042fixture\//` so no quote character reaches the Windows command line.

**Total deviations:** 6 (1 bug, 2 blocking, 3 plan-text). **Impact:** none on scope; the ledger is unchanged.

### Hand-off to plan 03

An update candidate that states `unsupported` or `skipped` for a key already in the ledger is written with evidence `core=<rev>`, which the new reason rule refuses on the next parse. The library judge only produces pass and fail facts, so nothing reaches this today, but the update guard of plan 03 (discard a candidate whose catalog status is not `unknown`) does not cover an `unknown` id and a candidate saying `unsupported`. `update.cmake` still asserts that statuses survive the filter (with `reason=janggun` as the candidate evidence); plan 03 should decide whether an update stage 2 row with those statuses is refused or keeps its reason.

## Counted check code and docs bytes

- Counted check code: **869** (cap for this plan 870, at most 10 above the previous 860; task 1 ended at 867, task 2 at 869, task 3 changed nothing). `compat-common.sh` is 119 lines, `compat-update.sh` stays 4. `src` is 9,403 lines (limit 940).
- `docs/compatibility.md` 4,549 bytes (was 4,489); no other document changed. Largest: `docs/embedding.md` 36,521, cap 38,912.
- Inventories: `ci.txt` 662 lines, `wasm32.txt` 622, `hygiene.txt` 78 (two added to each by this plan).
- Provisional timeouts recorded in `tests/compat/tests.cmake`: `compat.reasons` 30 s (0.04 s slowest of 4 local runs), `compat.hardening` 90 s (2.18 s slowest of 4); plan 10 settles them from the hosted runs.

## Threat model

T-12-05 to T-12-08 mitigated as listed (reason rule in every mode, octal title regex and first-byte rule under three awks, the exact 8,192-row cap plus the unchanged 4 MiB file cap, one-pass markdown escaping). The insertion sort stays quadratic on an unsorted hostile file (stated in the plan); a committed or generated ledger is sorted and pays nothing. No package install, no new endpoint, no threat flags.

## Self-Check: PASSED

- `tests/compat/reasons.txt` present; commits `81c544a`, `57448c9`, `1a964d0`, `9320b59` are ancestors of HEAD.
- Plan verify commands ran green (see Evidence); `compat.reasons` and `compat.hardening` registered once each in the three inventories.
