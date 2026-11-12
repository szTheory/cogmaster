---
phase: 12-whole-list-in-the-ledger
plan: "04"
subsystem: compat-page
tags:
  - compat
  - docs
  - ledger
  - page
status: complete

requires:
  - 12-03
provides:
  - A docs printer whose size is bounded by six caps in one BEGIN assignment, whatever the ledger holds
  - A page of aggregates, a group table, a reason table and four capped row sections, each ending in a +K more line with a link to the ledger
  - Titles escaped once, cut at 64 bytes after escaping, every byte at or above 0x80 printed as a question mark
  - compat.page_bound and compat.page_bound.self_test, a 700-row hostile ledger and a control that raises the caps
affects:
  - 12-05
  - 12-07
  - 12-08

actuals:
  tokens: 4100
  tasks: 2
  commits: 2

plan_head_before: 626b07b9d739482870b04b3290c4a2d230194e63
plan_head_after: e583b83bbeab150ef3f32b853fcc32a56b6c9a04
commits: 2

tech-stack:
  added: []
  patterns:
    - "One put(section, line) helper keeps a per-section counter; rows past the cap only increment it, so the cost of a row past the cap is one counter"
    - "The page caps are named cap_fixture, cap_unsupported, cap_fail, cap_other, cap_groups and cap_reasons in one BEGIN line, so a test can raise them all in a copy"
    - "docs mode reads the catalog through the loader that update and ratchet already use and counts its sg lines; an unreadable catalog is exit 2"

key-files:
  created: []
  modified:
    - scripts/compat-common.sh
    - docs/compatibility.md
    - tests/compat/tests.cmake
    - tests/library/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt

key-decisions:
  - "A fixture row goes to the fixtures section whatever its status; any other row goes to the first of unsupported, fail (fail and infrastructure_error) or other evaluated (pass); skipped and unknown rows appear only in the counts, the group table and, for skipped, the reason table"
  - "The first line of the counts keeps the form Rows: N. Evaluated coverage: E. Own fixtures: F. because an existing test pins it for an empty ledger"
  - "The intro is soft-wrapped at sentence ends (newlines inside one paragraph) so that no page line passes 600 bytes; its length is 884 bytes against the 900 budget"
  - "The page_bound test counts every expected overflow from its own construction and never reads a number back from the page"

requirements-completed: []

coverage:
  - id: C1
    description: "docs mode prints a page bounded by construction: six caps in one assignment, each row section capped with a +K more line and a link to the ledger, groups and reasons capped with +K more groups and +K more reasons"
    requirement: LEDG-04
    verification:
      - kind: unit
        ref: "tests/compat/tests.cmake case page_bound (compat.page_bound): 700 hostile rows give a 22090 byte page below the 36864 ceiling and each more-line equals the number the test counted from its construction"
        status: pass
    human_judgment: false
  - id: C2
    description: "The bound is the caps: a copy of the script with every cap_ value raised to 100000 produces a page over the ceiling and the test fails with the page budget message"
    requirement: LEDG-04
    verification:
      - kind: unit
        ref: "compat.page_bound.self_test (expect_fail, exit 1, regex exceeds the page budget); real-script controls cap_other=100000 (44862 bytes, red) and cap_fail=41 (the fail more-line reads 59, red)"
        status: pass
    human_judgment: false
  - id: C3
    description: "A title is escaped once by md(), cut at 64 bytes after escaping, and the page is ASCII only; no raw markdown character reaches a table row and no line passes 600 bytes"
    requirement: LEDG-04
    verification:
      - kind: unit
        ref: "compat.page_bound (255-byte titles of | & < * and multibyte letters; no [^ -~] byte, no < or * in a table row, no line over 600 bytes) and compat.hardening (three rows with every active markdown character); control: md() dropped from the title cell turns compat.hardening red"
        status: pass
    human_judgment: false
  - id: C4
    description: "A non-default cfg prints only the differing tokens in the profile cell and the default prints none; the profile cell carries machine, region, tv, mapper, fm and the predicate kind and frame"
    requirement: LEDG-04
    verification:
      - kind: unit
        ref: "compat.page_bound row with port1=lphaser: its cell reads sms/export/ntsc/m1 fm=off boots@600 port1=lphaser and no page row holds port2= or script="
        status: pass
    human_judgment: false
  - id: C5
    description: "The planning label rule binds generated prose, not table rows: a catalog title ending in the label of a pinned title reaches a table row with its parentheses escaped and no line outside the tables holds it"
    requirement: LEDG-04
    verification:
      - kind: unit
        ref: "compat.page_bound (unsupported row titled Sega Graphic Board (Japan, prototype v2.0)); grep -v '^|' docs/compatibility.md finds no v2.0, exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "docs mode fails closed on an unreadable catalog and prints the catalog's sg line count as the SG-1000 entries excluded (214 for the tracked catalog, 3 for the synthetic one)"
    requirement: LEDG-04
    verification:
      - kind: other
        ref: "COMPAT_CATALOG=<missing> sh scripts/compat-common.sh docs tests/compat/ledger.csv exits 2; grep -Eq 'SG-1000.*214' docs/compatibility.md exits 0; compat.page_bound asserts SG-1000 entries excluded: 3"
        status: pass
    human_judgment: false
  - id: C7
    description: "The committed page is the generator's output for the unchanged ledger, equal on BWK awk, mawk and gawk; the ledger is unchanged and passes the base judge from origin/main"
    requirement: LEDG-04
    verification:
      - kind: integration
        ref: "sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md exits 0 on macOS BWK awk and in ubuntu:26.04 under mawk 1.3.4; the hostile page is byte-identical under BWK, mawk and gawk 5.3.2; policy.compat_doc_fresh and its self_test pass; base judge and new judge exit 0 on the unchanged ledger"
        status: pass
    human_judgment: false
---

# Phase 12 Plan 04: The bounded page Summary

`docs/compatibility.md` is now generated from aggregates plus capped row sections, so no ledger, however large or hostile, can push it past 36,864 bytes. The committed page for the unchanged 10-row ledger is 2,447 bytes; a synthetic 700-row ledger of 255-byte hostile titles gives 22,090 bytes.

## Performance

- **Tasks:** 2 of 2 (the first is the tracer: the printer end to end, the committed page regenerated)
- **Files changed:** 7 (no new file)
- **Commits:** `90c4b41` (task 1), `e583b83` (task 2)

## Accomplishments

1. **Task 1, the tracer.** `scripts/compat-common.sh` replaces the docs printer: caps in one BEGIN line (24 fixtures, 20 unsupported, 40 fail, 40 other, 60 groups, 32 reasons), one `put()` helper per section, a `cell()` function for the row (short id of 12 hex digits or the whole fixture id, md-escaped title cut at 64 bytes with every byte outside printable ASCII replaced, profile cell with only the non-default cfg tokens and the predicate kind and frame). The intro is 884 bytes and says Experimental, what unknown means, what unsupported and skipped mean, what a compound id is, and that experimental leaves only on the maintainer's own-library evidence. docs mode now reads the catalog and prints the sg line count. `tests/compat/tests.cmake` hardening asserts the escaped title in three rows; `tests/library/tests.cmake` runs its two direct docs calls under the test's catalog and the tracked reasons. The page was regenerated with the generator command.
2. **Task 2.** `compat.page_bound` builds the 700-row ledger in one CMake pass (30 fixtures, 200 cart passes, 100 fails, 100 unsupported, 100 skipped, 170 unknown, 100 mappers, a 40-token stand-in reasons file, a 3-line sg stand-in catalog), runs docs once and checks the size, the six more-lines (6, 80, 60, 160, the groups and reasons overflow, each counted from the construction), the counts line, ASCII only, no line over 600 bytes, no raw `<` or `*` in a table row, the non-default profile cell, and the title with the planning label (in a table row, in no prose line). With `-DWRONG=ON` a copy of the script has every cap raised; the page is then 76,595 bytes and the case fails with `page exceeds the page budget`. Inventories gain `compat.page_bound` and `compat.page_bound.self_test` in `ci.txt`, `wasm32.txt` and `hygiene.txt`.

## Evidence (commands, exit statuses; no log committed)

- `cmake --preset ci` then `cmake --build --preset ci` then `ctest --test-dir build/ci -R '^(compat|policy\.compat|library|docs\.)'`: exit 0, 27 tests passed (after task 2).
- `cmake --workflow --preset ci`: exit 0 (657 tests, 5 release tests); `cmake --workflow --preset hygiene`: exit 0 (78 tests). Both after task 2.
- `ctest --test-dir build/ci -R '^compat\.page_bound$' -V`: exit 0, log line `page_bound: 22090 bytes of 36864`; slowest of 4 local runs 0.20 s. `ctest --test-dir build/ci -N -R '^compat\.page_bound'` prints `Total Tests: 2`.
- `sh scripts/compat-common.sh docs tests/compat/ledger.csv | cmp - docs/compatibility.md`: exit 0 (macOS BWK awk 20200816); the same command in `docker run ... ubuntu:26.04` (mawk 1.3.4): exit 0. The hostile page of the test ledger is byte-identical under BWK, mawk and gawk 5.3.2 (`cmp`, exit 0; gawk installed with apt in the throwaway container only).
- `LC_ALL=C grep -c '[^ -~]' docs/compatibility.md` prints 0; `awk '/^# Compatibility/{p=1;next} /^Rows:/{exit} p' docs/compatibility.md | wc -c` prints 884; `grep -v '^|' docs/compatibility.md` holds no `v2.0` (exit 0); `grep -Eq 'SG-1000.*214' docs/compatibility.md` exit 0; `grep -q 'Experimental\.' docs/compatibility.md` exit 0.
- `COMPAT_CATALOG=<missing> sh scripts/compat-common.sh docs tests/compat/ledger.csv`: exit 2 (`invalid ledger record 0`).
- `awk '/compat-(common|ratchet)\.sh/ && !/COMPAT_CATALOG=/ {bad=1} END {exit bad}' tests/library/tests.cmake`: exit 0.
- `git diff --quiet origin/main -- tests/compat/ledger.csv`: exit 0 (ledger unchanged). Base judge from `git show origin/main:scripts/...` over the origin/main ledger and the head ledger: exit 0, `compat: protected comparison passed`; the new judge with the same pair: exit 0.
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake`: exit 0, `879 check lines / 9403 src lines`.
- `test "$(wc -c < docs/compatibility.md | tr -d ' ')" -le 38912`: exit 0 (2,447 bytes).
- `git diff --numstat origin/main -- tests/inventory/{ci,wasm32,hygiene}.txt`: 5, 5 and 4 added, 0 deleted. This plan adds exactly two lines in each (plan 01 added `cart.generator.catalog` to `ci.txt` and `wasm32.txt`, plan 02 two names to each).
- Hosted dispatch: none in this plan (the first is after plan 5).

## Negative controls (each: python edit, rerun, red, restore, green)

| Control | Mutation | Result |
|---|---|---|
| Escaping | `md()` dropped from the title cell | `compat.hardening` red; restored green |
| Caps removed (copy) | every `cap_` value raised to 100000 in a copy | `compat.page_bound.self_test` passes (the case exits 1 with `page exceeds the page budget: 76595 bytes`) |
| One cap raised (real script) | `cap_other=100000` | `compat.page_bound` red, `page exceeds the page budget: 44862 bytes`; restored green |
| One cap moved by one (real script) | `cap_fail=41` | `compat.page_bound` red, `the page must say +60 more fail rows, not '59'`; restored green |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] BWK awk splits a single-character separator at newlines too**
- **Found during:** Task 1
- **Issue:** the two table headers were built with `split(..., hd, "@")`; on macOS awk (20200816) a one-character separator also splits at every newline, so the header and its rule line landed in different array elements.
- **Fix:** the headers are assigned one by one; no split input holds a newline.
- **Files modified:** scripts/compat-common.sh
- **Commit:** 90c4b41

**2. [Rule 1 - Bug] An awk program in single quotes cannot hold an apostrophe**
- **Found during:** Task 1 (planning the intro)
- **Issue:** the intro wording of the plan has the possessives "MAME's" and "maintainer's".
- **Fix:** the intro says "the MAME verdict about the dump" and "the own-library evidence of the maintainer".
- **Commit:** 90c4b41

**3. [Rule 1 - Bug] The 600-byte line check caught the intro**
- **Found during:** Task 2
- **Issue:** the intro paragraph was one 776-byte line.
- **Fix:** soft line breaks at three sentence ends (one paragraph in a reader, four lines in the file); the byte total is unchanged at 884.
- **Commit:** e583b83

### Plan text and acceptance wording

**4. [Planner reading] The profile cell carries the predicate kind**
- The page spec puts `kind@F` in the profile cell, so the cell reads `sms/export/ntsc/none fm=off frame@14`; the predicate and the evidence are not printed.

**5. [Planner reading] infrastructure_error rows list with the fail rows**
- The fail section holds `fail` and `infrastructure_error` rows (the heading says fail rows). None exists in the ledger today and the synthetic ledger holds none; the section and its cap apply to both.

**6. [Plan text] Unknown rows leave the page rows**
- Crazy Faces (unknown) no longer appears in a table row, as the plan states (unknown rows appear in the counts and the group table only). The committed page lists the nine fixture rows.

**7. [Plan text] The hardening assertion**
- The old assertion expected one 100-byte escaped title in a cell; a 64-byte cut would cut it. The hardening case now writes the active characters in three rows that each fit and asserts each cell.

**8. [Plan text] The direct docs calls in the library test**
- Both calls were already under `cmake -E env COMPAT_CATALOG=...` (plan 02 and 03); this plan adds `COMPAT_REASONS` to them. The merged page and a fresh generation carry the same sg count (0).

**9. [Plan text] Page size of the hostile ledger**
- The plan expected about 28 KB; the page is 22,090 bytes because a row is about 150 bytes (the plan sized 170) and the profile cell no longer carries the evidence or the predicate text. The ceiling check is unchanged and the raised-cap copy (76,595 bytes) shows the margin is the caps.

**Total deviations:** 9 (3 auto-fixed, 6 plan text or acceptance wording). **Impact:** none on scope; the ledger is unchanged.

## Known Stubs

None.

## Counted check code and docs bytes

- Counted check code: **879** (cap for this plan 888, at most 12 above 876; the reserve was not used, so every later plan keeps its own increment above the printed figure with the absolute ceiling 905). `scripts/compat-common.sh` is 129 lines; `src` is 9,403 lines (limit 940). The page block (with its fault comment) is 11 lines against the 12 it replaces and the plan adds the caps line (1), `put` (1) and `cell` (2); the catalog sg read costs no line (the existing loader condition gained `docs`).
- Docs bytes: `docs/compatibility.md` 2,447 bytes (cap 38,912); the intro 884 bytes (budget 900); the synthetic page of `compat.page_bound` 22,090 bytes (ceiling 36,864); the raised-cap copy 76,595 bytes.

## Self-Check: PASSED

- `scripts/compat-common.sh`, `docs/compatibility.md`, `tests/compat/tests.cmake`, `tests/library/tests.cmake` and the three inventories exist and are committed; commits `90c4b41` and `e583b83` are ancestors of HEAD.
