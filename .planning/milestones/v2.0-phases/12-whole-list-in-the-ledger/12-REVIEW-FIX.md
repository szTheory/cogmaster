---
phase: 12-whole-list-in-the-ledger
fixed_at: 2026-11-12T00:00:00Z
review_path: .planning/phases/12-whole-list-in-the-ledger/12-REVIEW-LEDGER.md, .planning/phases/12-whole-list-in-the-ledger/12-REVIEW-CORE.md
iteration: 1
findings_in_scope: 17
fixed: 17
skipped: 0
status: all_fixed
---

# Phase 12: Code Review Fix Report

**Fixed at:** 2026-11-12
**Source reviews:** `12-REVIEW-LEDGER.md` (ledger, ratchet, update, library judge), `12-REVIEW-CORE.md` (core, generator, wording)
**Iteration:** 1 of at most 2

**Summary:**
- Findings in scope: 17 (ledger WR-01 to WR-06 and IN-03; core WR-01, WR-03 to WR-10 and IN-01)
- Fixed: 17, skipped: 0, disputed: 0 (each finding was checked against the code first; every one held)
- Out of scope, appended to `.planning/seeds/SEED-015-whole-list-follow-ups.md` under "Phase 12 review leftovers" (append-only):
  ledger WR-07 (timeouts; the ship pull request run adds samples), IN-01 to IN-06 except IN-03, core WR-02 (quadratic scan),
  IN-02 to IN-06, and the base-copy hardening of ledger WR-01.
- No ROM, BIOS, library file or `.env.local` was opened. Every control uses self-authored XML, synthetic text or stand-in
  ledgers. The behaviour revision (17), `ci.yml`, `src/` and the committed ledger are untouched by this round.
- The `is_experimental` flag and the word "experimental" stay, as the owner decided.

## Fixed Issues

### Ledger WR-01: the protected judge trusts its catalog and reasons

**Files modified:** `scripts/compat-common.sh`, `tests/compat/ratchet.cmake`, `docs/testing.md`, seed
**Commits:** 74c5b55 (code and controls), b054329 (trust model text and seed)
**Applied fix:** in ratchet mode the parser now refuses a catalog that holds no `unsupported` or `skipped` status (an empty or
reshaped file read every id as `unknown`, switched rule R1 off and released every `skipped` row). `ci.yml` is untouched; its
baseline-closure step still copies five files. `docs/testing.md` now says the judge reads the catalog and reasons of the
checkout it judges, so the comparison guards against accidental or unreviewed flips, not against an author who can also edit
the catalog and `ci.yml`. The "copy catalog and reasons from the base" hardening is a SEED-015 line.
**Negative control:** `compat.ratchet` cases `closed_catalog_empty` and `closed_catalog_no_status` expect exit 2; the old script
exits 0 on the first. Also found by the full workflow: the no-verdict stand-in of `library.synthetic` used a status-less
catalog and was refused; its stand-in now holds a `skipped` line (commit 39f3739).

### Ledger WR-02: R2 let a `fail` row be deleted while the id kept another row

**Files modified:** `scripts/compat-common.sh`, `tests/compat/ratchet.cmake`
**Commit:** 74c5b55
**Applied fix:** one per-key rule covers `fail`, `unsupported` and `skipped`: a base row whose head status differs (a vanished
row has none) is a violation, except `fail` to `pass` and a row `rel()` releases. The id-continuity test it subsumes and the
`hid` bookkeeping are removed (one line less). Rows of status `unknown` may still vanish.
**Negative control:** `fail_pal_row_dropped` (a default row and a PAL `fail` row in the base; the head drops only the PAL row)
expects exit 2 and exited 0 on the old script; `unknown_pal_row_dropped` expects exit 0.

### Ledger WR-03: the exit floors counted rows, the stated intent counted images

**Files modified:** `tests/compat/exit.cmake`, `quality.json`
**Commit:** a8b5093
**Applied fix:** `cart_pass` counts distinct catalog image ids (one marker variable per id, no new loop); the PAL and FM floors
stay row based. The printed `cart_pass=N/20` now reads as carts, and the `compat.cart_pass` reason text says one image with a
default and a PAL pass row counts once (value unchanged, 20). `docs/conformance.md` says "20 cart images with a judged pass".
**Negative control:** the self test `twin_rows_one_cart` (twenty pass rows over nineteen images, flag present) must end unmet
with status 0; with the old counting it ends status 1 ("floors met: remove is_experimental") and the self test fails.

### Ledger WR-04: the page was bounded only for short ids, mappers and script tokens

**Files modified:** `scripts/compat-common.sh`, `tests/compat/tests.cmake`
**Commit:** ce07fdf
**Applied fix:** a non-hash id is cut at 40 bytes, a mapper (table cells and group lines) at 32, a script token at 24 and a
reason token at 32, like titles.
**Negative control:** `compat.page_bound` now builds 900-byte fixture ids, 900-byte mappers, an 850-byte script token and a
300-byte reason token, keeps the construction-derived counts, and asserts each cut. The page measures 28,689 bytes against the
36,864 ceiling; the old script prints 208,450 bytes and the test fails. The real page is byte identical to a fresh generation.

### Ledger WR-05 (and the flag reading both tests share): exact-spelling flag detection

**Files modified:** `tests/cmake/flag.awk` (new, 3 counted lines), `tests/cmake/repository.sh`, `tests/compat/exit.cmake`,
`tests/policy/repository-checks.txt`, `tests/policy/claims_cases.cmake`
**Commit:** 0646b3b
**Applied fix:** one awk reader, `tests/cmake/flag.awk`, is called by `compat.exit` and by `policy.claims_scoped`. The last
non-comment `is_experimental` line wins as the core's config reader takes it; spacing, key and value case, quotes and a trailing
comment are tolerated; any value but an explicit `false` is present (so `0` and an empty value count as present).
**Negative control:** `compat.exit.self_test` runs value `1`, upper case, a trailing comment, unquoted and two-line templates
(true last, false last) in both tie directions; `policy.claims_scoped.self_test` runs `1`, upper case with a comment, unquoted
and a true-then-false pair against the wording check. With the old exact matchers both self tests fail on the first spelling.

### Ledger WR-06: the over-claim tripwire did not match across a hard wrap

**Files modified:** `tests/cmake/repository.sh`, `tests/policy/claims_cases.cmake`
**Commit:** 869f451
**Applied fix:** the README, the `.info` template and the page's non-table lines are joined with `tr` before the grep.
**Negative control:** `over_claim_wrapped` builds a README whose claim is wrapped over four lines and expects the over-claim
failure; the old line passes it (the self test failed with the old line).

### Ledger IN-03: more invisible and line-breaking characters in a title

**Files modified:** `scripts/compat-common.sh`, `tests/compat/tests.cmake`
**Commit:** 50e29cc
**Applied fix:** the one hardening regex now also refuses U+2028, U+2029, U+2060 to U+2064, the C1 controls (U+0080 to U+009F,
which includes U+0085), U+00AD and U+061C.
**Negative control:** `compat.hardening` refuses a title with each (nine cases) and accepts U+2027 and U+00A0 beside them;
the old rule accepts the first (`title line_separator: expected exit 2, got 0`). The committed ledger is still a fixed point.

### Core WR-01: an embedded NUL truncated a list and exited 0

**Files modified:** `tools/cartdb.c`, `tests/cart/catalog.cmake`
**Commit:** 77d08d6
**Applied fix:** after the read, `strlen(input) != n` stops the run with "input holds a NUL byte".
**Negative control:** `cart.generator.catalog` builds a list with a NUL after a record (through `sh` and `printf`, size
asserted) and expects exit 1 matching "NUL byte"; with the check removed it fails.

### Core WR-03: the title format-character filter was incomplete

**Files modified:** `tools/cartdb.c`, `tests/cart/catalog.cmake`
**Commit:** 77d08d6
**Applied fix:** the generator refuses the same set as the ledger parser: the C1 controls (U+0080 to U+009F), U+00AD, U+061C,
U+2028, U+2029 and U+2060 to U+2064, besides the previous ranges. The comment says the two rules are the same set.
**Negative control:** nine titles, one per added character class, expect "format character", and U+2027 passes; with the old
filter the control fails.

### Core WR-04: the catalog test's table assertion could never fail

**Files modified:** `tests/cart/catalog.cmake`, `tests/cart/generator.cmake`
**Commit:** 77d08d6
**Applied fix:** the table prints SHA-1 bytes as `0xNN,`, so the check now converts the compound ids and the short-dump chip
to that form and requires them absent, requires the not-working single-chip image present, and counts exactly 11 table rows (the
authored lists have 18 catalog lines, 7 of them compound). The same dead pattern in `generator.cmake` is corrected to the byte form.
**Negative control:** with the `if (f->kind) continue;` skip removed from the table loop, `cart.generator.catalog` and
`cart.generator` both fail.

### Core WR-05: the catalog-to-table tie checked machine and board only

**Files modified:** `tests/compat/whole_list_common.cmake`, `tests/compat/whole_list.cmake`
**Commit:** 6cc4fe3
**Applied fix:** the one table awk (still one parse) also compares slot (through the inc's slot table), region and port1/port2
(derived from the fact the way the generator derives them), and reports a public Master System or Game Gear fact with no catalog
line. The real catalog and table agree (1,444 compared).
**Negative control:** `compat.whole_list.reason` runs a slot, a region and a port edited away from the fact, and an extra public
fact over the stand-in table (4 new controls, 11 in all); the old awk accepts the first. Real-file mutations under mawk and gawk
print the same refusals.

### Core WR-06: `docs/maintaining.md` said the tests regenerate from the lists

**Files modified:** `docs/maintaining.md`
**Commit:** c91a642
**Applied fix:** "the tests check them against each other, not the lists, so `cmp` them" (19,862 bytes, was 19,866; the
preceding sentence was shortened to pay for it).
**Negative control:** none (prose).

### Core WR-07: "Skipped is the MAME verdict" was false for 18 of 24 skipped rows

**Files modified:** `scripts/compat-common.sh`, `docs/compatibility.md` (regenerated), `docs/conformance.md`
**Commit:** 5560594
**Applied fix:** skipped now means "no check is run for the entry (MAME marks it not working or a bad dump, or its record is not
one image this table can identify)". The longest generated line is 424 bytes (limit 900). `policy.compat_doc_fresh` and the
page-equality under mawk and gawk are green.
**Negative control:** `policy.compat_doc_fresh` already fails on a page that differs from the generator.

### Core WR-08: README Mark III coherence

**Files modified:** `README.md`
**Commit:** 14cd586
**Applied fix:** the owner's scope line (`REQUIREMENTS.md:8`) says no Mark III profile and an SMS and Game Gear denominator; the
Japan cartridges of `sms.xml` are the Mark III software and are in the ledger. The sentence "Mark III software is not tracked"
is replaced by "Mark III software is the Japanese Master System: its Japan cartridges are listed under Master System".
`policy.claims_scoped` stays green.
**Negative control:** none (prose, and the claims check is unchanged).

### Core WR-09: README claimed sound in the headless runner

**Files modified:** `README.md`
**Commit:** 14cd586
**Applied fix:** "with sound in RetroArch. The runner has no window or speaker; it reports picture and audio hashes that are
the same on every platform."
**Negative control:** none (prose).

### Core WR-10: ASSET_POLICY mis-described unknown rows

**Files modified:** `ASSET_POLICY.md`
**Commit:** 69f9945
**Applied fix:** "A ledger row is public catalog metadata and, only for a row the maintainer judged, what our core did with a
dump the maintainer owns; no dump byte, no digest of our output." The same inaccuracy in `docs/conformance.md` ("a row says what
this core did with a dump") is corrected in commit b054329.
**Negative control:** none (prose).

### Core IN-01: behaviour revision 17 pinned by nothing

**Files modified:** `tests/core/test_core.c`
**Commit:** b8feae4
**Applied fix:** one exact check, `CHECK_EQ_U64(v.behaviour, 17u)`, beside the check that already compares the reported
versions with the macros. `tests/api/v14.c` stays `>= 16`.
**Negative control:** with the header set to 16, `core.info` fails (and passes again with 17).

## Verification

Run in the main checkout (`workflow.use_worktrees` is false), from the repository root.

| Command | Exit |
|---|---|
| `cmake --workflow --preset ci` (first run, before 39f3739) | 8: only `library.synthetic` failed (the status-less stand-in catalog, see ledger WR-01) |
| `cmake --workflow --preset ci` (after 39f3739) | 0 (666 of 666; `ci-release` 5 of 5) |
| `cmake --workflow --preset hygiene` | 8: only `hygiene.tree` failed, on an address-shaped string in the untracked ledger review file (rewritten to prose, as in the Phase 11 round); 83 of 84 passed |
| `sh scripts/hygiene.sh --tree` after that rewrite and after this report was written | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (887 check lines, 9,403 src lines; limit 940) |
| gcc:13 container, `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `tools/cartdb.c`, `tests/core/test_core.c` | 0 each |
| `clang-format` 21 `--dry-run -Werror` over the same two files | 0 each |
| `normalize` of the ledger, `docs` of the ledger and `docs` of the 900-byte hostile ledger under mawk 1.3.4 and gawk 5.2.1 (ubuntu:24.04 container; the macOS awk is the suite's) | ledger and page byte identical to the committed files; hostile page digest identical on all three awks |
| `flag.awk` over six spellings and a two-line template under mawk and gawk | same present or absent answer as the macOS awk |
| the table awk over the real inc and catalog, and over a mutated catalog (region, port, slot, a removed line), under mawk and gawk | `facts=1660 compared=1444`; the same four refusals |
| old judge: `git show origin/main:scripts/compat-common.sh` and `compat-ratchet.sh`, origin/main's ledger as base, the head ledger, `2026-11-12 16 17` | 0, `compat: protected comparison passed` |
| the head judge over the same pair | 0, `compat: protected comparison passed` |
| `git diff d1686db..HEAD --stat -- tests/compat/ledger.csv .github src include` | empty: no ledger, workflow, core or header edit in this round |
| `git push origin phase/12-whole-list-in-the-ledger` | 0 (d1686db..39f3739); no CI dispatched, no pull request touched |

Budgets after the round: counted check code 887 (was 885; ceiling asked 925, hard limit 940): `compat-common.sh` 128 lines (was 129),
`repository.sh` 210, new `tests/cmake/flag.awk` 3. Docs bytes: `docs/conformance.md` 35,015 (was 35,075; cap 35,075),
`docs/testing.md` 33,801 (was 33,817; cap 33,817), `docs/maintaining.md` 19,862 (was 19,866), `docs/compatibility.md` 4,171 (was
4,072), `README.md` 4,939 (was 4,908; cap 12,288); `docs/embedding.md` at 36,521 is the largest, under 38,912. No test was added
or removed, so the inventories and the `quality.json` floors (`tests.ci` 669, `tests.wasm32` 627) stand: the registered count
is 671 and the floor is registered minus 2. The new controls live inside existing tests, so the ledger-row cost on the slow
lanes is unchanged (stand-in ledgers only; `exit.cmake` spawns one extra `awk` per template-bearing control, about twenty in the
self test).

Timeouts: no limit changed. The seed records that the four sample-based limits (`library.synthetic`, `compat.ratchet`,
`compat.public_update`, `compat.csv`) are re-settled after the ship pull request run.

Deviations:
- Findings that share a file share a commit where their hunks interleave: ledger WR-01 and WR-02 (74c5b55), core WR-01, WR-03 and
  WR-04 (77d08d6), core WR-08 and WR-09 (14cd586).
- Ledger WR-01's trust-model text went into `docs/testing.md` only, where the protected comparison is already described;
  `docs/conformance.md` and `docs/testing.md` were at their plan caps, so two redundant sentences were removed to pay for the
  additions: in `testing.md`, "On a recent Mac a clean run takes a couple of seconds" and "Independent fixture oracles and
  wrong-pixel controls remain separate"; in `conformance.md`, "The flag leaves on the owner's evidence, never on a pass rate"
  (the generated page intro and the same paragraph already say it).
- The flag reader is a new classified check file (`tests/cmake/flag.awk`, 3 counted lines) rather than a function in one of the
  two callers, because one is CMake and the other POSIX sh.
- `0` and an empty value read as present (only an explicit `false` is absent), by the review's wording; the core's own reader
  would take `0` as false, which only makes the tie stricter.
- Ubuntu 26.04 is not available locally: the awk checks ran under ubuntu:24.04 (mawk 1.3.4 and gawk 5.2.1) and the macOS awk.
- The untracked review files held an address-shaped string (ledger) and a scratch path (core); both were rewritten to prose so
  `hygiene.tree` passes, as in the Phase 11 round. The review files and this report are not committed here.
- The second full workflow run was needed because ledger WR-01 broke the status-less stand-in catalog of `library.synthetic`;
  the control and the fix are in 39f3739.

---

_Fixed: 2026-11-12_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
