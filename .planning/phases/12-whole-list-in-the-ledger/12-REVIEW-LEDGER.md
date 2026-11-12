---
phase: 12-whole-list-in-the-ledger
reviewed: 2026-11-12T00:00:00Z
depth: standard
files_reviewed: 24
files_reviewed_list:
  - scripts/compat-common.sh
  - scripts/compat-ratchet.sh
  - scripts/compat-update.sh
  - scripts/compat-seed.sh
  - scripts/library.sh
  - scripts/library.cmake
  - scripts/ci-durations.sh
  - .github/workflows/ci.yml
  - tests/compat/ledger.csv
  - tests/compat/catalog.tsv
  - tests/compat/reasons.txt
  - tests/compat/pin-counts.txt
  - tests/compat/exit.cmake
  - tests/compat/whole_list.cmake
  - tests/compat/whole_list_common.cmake
  - tests/compat/ratchet.cmake
  - tests/compat/update.cmake
  - tests/compat/migration.cmake
  - tests/compat/tests.cmake
  - tests/library/tests.cmake
  - tests/policy/claims_cases.cmake
  - tests/cmake/repository.sh
  - quality.json
  - .gitattributes
findings:
  critical: 0
  warning: 7
  info: 6
  total: 13
  necessity: 0
status: issues_found
---

# Phase 12: Code Review Report (ledger, ratchet, update, library judge half)

**Reviewed:** 2026-11-12
**Depth:** standard, security-minded
**Files Reviewed:** 24 (head d1686db against origin/main v0.11.0)
**Status:** issues_found

## Summary

Scope: the parser (reason rule, hardening, catalog loader, ratchet R1-R3, update guard, bounded page), the seed, the library judge,
the exit command, the claims guard and their tests. `.github/workflows/ci.yml` changes exactly one line (the `build` job
`timeout-minutes: 6` to `9` and its comment); nothing else in the file moved, so the base-judge step is the Phase 11 step.

What I ran (commands, all from the repository root):
- The OLD judge from origin/main over the head ledger (`compat-ratchet.sh base.csv ledger.csv allow.txt <today> 16 17`): exit 0,
  `compat: protected comparison passed`. The new judge over the same pair: exit 0. Head against itself: exit 0.
- Under `ubuntu:24.04` (mawk 1.3.4): `normalize | cmp` with the ledger, `docs | cmp` with `docs/compatibility.md`, the seed `| cmp`
  with the ledger, and the ratchet on the ledger against itself: all clean. `=1+1` and U+202E titles are refused (exit 2).
- `ctest --test-dir build/ci -R 'compat\.(exit|whole_list|reasons|hardening|page_bound)|policy\.claims|compat\.csv|compat\.ratchet'`:
  15 of 15 pass, about 2 s wall.
- `cmake -P tests/cmake/budgets.cmake`: 885 check lines against a 940 limit.
- Hand-built mutations of the ledger, catalog and reasons against the new judge (WR-01, WR-02 below), a 30-row hostile ledger
  through `docs` (WR-04), flag spellings through the `file(STRINGS)` regex of `exit.cmake` (WR-05), and wrapped claim text through the
  claims regex (WR-06).

Verdict: no Critical finding. The core of the phase holds: the head ledger passes the old and the new judge, the seed is
reproducible, the reason rule and the update guard do what they claim, and the tie fires in both directions with a non-vacuous
`met` branch. The weaknesses are in what the ratchet is claimed to protect: the new rules read head data (catalog, reasons) with no
independent anchor, R2 is weaker than its stated purpose, the "bounded by construction" page is bounded only for short ids,
mappers and scripts, and two text guards (flag detection, over-claim regex) are narrower than their comments say.

## Warnings

### WR-01: The protected judge reads its catalog and reasons from the head checkout; "complete helper closure" no longer is

**File:** `scripts/compat-common.sh:5,39,44,105,108`, `.github/workflows/ci.yml:137-151,208-211`
**Issue:** R1 (head `pass`/`fail` on a catalog-`unsupported`/`skipped` id), R3 (`rel()`: retired token plus changed catalog status)
and the update guard all read `tests/compat/catalog.tsv` and `reasons.txt` through cwd-relative defaults. In CI the
"Preserve baseline judge and its complete helper closure" step copies only `compat-ratchet.sh`, `compat-common.sh`, the base ledger,
`cogmaster.h` and `quality.json`; the judge runs from `$trusted/scripts/` but with cwd at the head checkout, so both data files are
the candidate's. The step name is now false: the closure has two more members.

Reproduced (new judge, base = committed ledger; head edits made together):
- Head ledger changes the `graphic` row (`d04ea11e...`) from `unsupported,reason=graphic` to `pass,core=aaaa...`; head catalog line
  changed to board `sega`, status `unknown`; head `reasons.txt` `graphic open` to `graphic retired`. Result: exit 0, `protected
  comparison passed`. With the ledger edit alone: exit 2 (R1). So the rule is a speed bump that forces three consistent edits, not a
  check against an independent fact.
- `skipped` is weaker still: `rel()` releases a base `skipped` row when the head catalog status is not `skipped` or the id is absent,
  with no reasons edit. The only other tie is `pin-counts.txt` `mame_no 6`, also head data. An empty or status-less head
  `catalog.tsv` reads every id as `unknown` (line 44 maps a missing status to `unknown`), which switches R1 off and releases every
  `skipped` row; the "fails closed" claim covers only an unreadable file (`getline < 0`), not an empty or reshaped one.
- The independent tie for `unsupported` is `compat.whole_list.reason` (catalog against `src/cartdb.inc`), a head test the same PR
  can edit. Nothing in CI independent of the PR anchors the `skipped` status.

Marginal exposure is limited because an `unknown` to `pass` flip needs no evidence anywhere (the base rule protects only base `pass`
rows), so this adds little beyond that known gap; but the control is named "protected" and rule 6 means no human reads these diffs
by default (and `.gitattributes` now collapses the ledger diff, see IN-01).
**Fix:** In a later `ci.yml` edit (not this phase, one edit per phase): copy `tests/compat/catalog.tsv` and `reasons.txt` from the base
into `$trusted/tests/compat/` and run the base judge with `COMPAT_CATALOG`/`COMPAT_REASONS` pointing there for R1, treating an id as
unsupported or skipped if the BASE catalog says so; use the head catalog only for R3's "board arrived" half and require the head
`src/cartdb.inc` fact. Cheaper in the parser now: in ratchet mode refuse a catalog with no `unsupported|skipped` line, so an empty or
status-less file is exit 2 instead of silent. Record the gap in the seed that lists the CI edits.

### WR-02: R2 id continuity lets a `fail` row be deleted whenever the id keeps any other row

**File:** `scripts/compat-common.sh:103,107`; `tests/compat/ratchet.cmake` (`failure_deleted`)
**Issue:** Line 107 tests `!hid[2,ids[key]]`, "the id appears in the head ledger under any key". A base `fail` row is protected per key
by line 103 only when its key still exists in the head (`seen[2,key]`); if the key vanishes, only the id test remains, and every
cart id keeps its seeded default row. Reproduced: base = committed ledger plus a `fail` PAL boots row for `0199c62a...` (Chuck
Rock); head = the committed ledger (the PAL row deleted). New judge: exit 0, `protected comparison passed`. The same row changed to
`unknown` at the same key is refused (exit 2), so a failure is protected from an edit and unprotected from a delete. The phase's
own stated fault is "hiding a `fail` by deleting it". `failure_deleted` deletes the id's only row, so it cannot see this.
For `unsupported` and `skipped` the per-key line 108 does catch the vanish, so the leak is `fail` only.
**Fix:** Make R2 per key for `fail`: `seen[1,key] && old=="fail" && !seen[2,key] && !rel(key) && !allowed[key]` is a violation. Add a
control where the base holds a default row and a `fail` row for one id and the head drops only the `fail` row (expect exit 2); keep
`unknown` rows free to vanish.

### WR-03: The exit floors count rows, but the documented intent and the quality reason count images

**File:** `tests/compat/exit.cmake:171-182`, `quality.json` (`compat.cart_pass` reason)
**Issue:** `cart_pass` increments per judged `pass` row of a catalog image id. An export-header SMS cart legitimately carries two
judged rows (export NTSC default plus the derived PAL row; the update whitelist allows 3 rows per SMS id, 2 per GG id). So the
20-row floor is met by about 10 carts (0.7 percent of 1,444), not "about 1.4 percent of 1,444 images" as the quality reason says,
and the 1 PAL / 1 FM floors are met by the same carts. The floor is the release trigger for the flag; the stated meaning does not
match the computed one, and the printed `cart_pass=N/20` reads as carts.
**Fix:** Count distinct ids for `cart_pass` (one `set(exit_pass_${id} 1)` and a counter, no new loop), or reword the reason and the
printed label to "rows" and drop the percentage. The PAL and FM floors can stay row-based.

### WR-04: The page is not "bounded by construction": only titles are bounded, ids, mappers and script tokens are not

**File:** `scripts/compat-common.sh:19-21,127`; `tests/compat/tests.cmake` (`page_bound`, 255-byte titles, 100 short mappers)
**Issue:** `cell()` prints a non-40-hex id in full (a `fixture/` id may be about 1,000 bytes), the mapper (`[a-z][a-z0-9_-]*`, up to
1,024), and for a non-default `cfg` the differing tokens including `script=` (up to about 900 bytes before the 1,024 predicate
limit). Group lines print `machine/mapper/status`. Reproduced: a 30-row ledger of fixture rows with 900-byte id, 900-byte mapper
and 850-byte script, accepted by `normalize`, makes `docs` print 94,839 bytes against the 38,912 docs cap and the 36,864 test
ceiling, from the 24 fixture rows alone; adding the 20 + 40 + 40 other-section rows and 60 groups would reach several hundred KB.
`compat.page_bound` fixes ids to `f<n>`, mappers to `m<0-99>` and scripts to `none`, so it passes while the claim is false.
The practical effect is a red `docs.budgets` on a self-evidently hostile PR, not an exploit, but the comment "Fault prevented: a
generated page that grows with the ledger past the docs cap" and the D9 claim promise more.
**Fix:** Cut every free-width cell, not only the title: `substr(id,1,40)`, `substr(mapper,1,32)`, `substr(t[5],1,24)` for the script
token, and cut each group line; then add 255-byte mappers, 255-byte fixture ids and a long script to the `page_bound` hostile ledger
so the ceiling is exercised.

### WR-05: Flag detection in `compat.exit` and in `policy.claims_scoped` is exact-spelling and the two disagree

**File:** `tests/compat/exit.cmake:209` (`^[ \t]*is_experimental[ \t]*=[ \t]*"?true"?[ \t]*$`), `tests/cmake/repository.sh` (`grep -qx 'is_experimental = "true"'`)
**Issue:** Verified with `file(STRINGS)`: `is_experimental = "1"` (RetroArch's config reader accepts `1`), `"TRUE"`, a trailing
`# comment`, and an `is_experimental="true"` line followed by `is_experimental = "false"` are classified as absent (the first three)
or present (the last) differently from what RetroArch will read. In the `met` state a template that still sets the flag in one of
those spellings is read as `absent` and the tie passes with the flag live: the tie's second direction is vacuous for any spelling
the regex does not know. `claims_scoped` uses an even stricter exact-line match, so `is_experimental = true` (unquoted) is present
for the exit command and absent for the claims guard. Today's template is the exact spelling, so nothing is red; this is the failure
the tie exists to prevent, one spelling away.
**Fix:** Treat any non-comment line whose key is `is_experimental` as present regardless of value (`^[ \t]*is_experimental[ \t]*=`),
share one regex between `exit.cmake` and `repository.sh` (the claims check can call the same awk), and add the `"1"` and trailing-comment
spellings to the `exit.cmake` self test.

### WR-06: The over-claim tripwire does not match across a line wrap, and the README is hard-wrapped

**File:** `tests/cmake/repository.sh` (`claims_scoped`, the `grep -Eqi 'plays (all|most|every)|...'` line)
**Issue:** `grep` is per line and every pattern contains a literal space, while `README.md` wraps at about 110 columns. Verified:
"It plays\nmost games and is compatible with\nevery title. 900\ngames." matches nothing, whereas the same text on one line matches. The
claim guard exists to stop exactly this class of README edit, and the likeliest edit (reflowing a paragraph) defeats it. Wording
variants such as "plays nearly everything", "100 percent compatible" or "supports all cartridges" also pass; the docs say it is a
literal tripwire, but the wrap blind spot is not stated.
**Fix:** Test the unwrapped text: `tr '\n' ' ' < "$f" | grep -Eqi ...` (README and `.info.in` description only; keep the page on its
non-table lines). Add a wrapped control to `claims_cases.cmake`.

### WR-07: Several test timeouts are exactly 2 x one or two samples on the slowest, jitteriest lane

**File:** `tests/library/tests.cmake:291` (160 s, S 79.87, 1 run), `tests/compat/tests.cmake:729` (142 s, S 70.79, 2 runs), `:799`
(104 s, S 51.56, 2 runs), `:743` (45 s, S 22.21, 2 runs)
**Issue:** The 2xS rule gives a 2.00x to 2.03x margin from one or two windows-arm64 samples for the four tests whose cost is process
spawns (15-25x native there). `library.synthetic` grew from 54.22 s (4 runs) to 79.87 s (1 run) this phase; the new samples were
taken on one dispatch (run 43158408243 at 291d5f5, not head). A single slow runner (the one-in-a-few the earlier phases saw
under shared load) fails the PR run for a reason unrelated to the change, and rule 7 allows only two fix rounds. The `build` job limit
is 9 minutes against a measured 252 s (2.14x), also one run. This is a flake risk, not a defect in logic.
**Fix:** Before the PR run, take two more samples (`sh scripts/ci-durations.sh tests <run id>` on the next green push) and set these
four to `max(2 x S, S + 60)` with the run ids; or keep the rule and accept a rerun policy. Also lower the cost: `library.synthetic` makes
about eight `library()` calls and now one extra `cmake -P exit.cmake` child per call (`library.sh:84-87`).

## Info

### IN-01: `linguist-generated=true` collapses the ledger diff in pull requests

**File:** `.gitattributes:10-13`
**Issue:** The ledger is the file whose row flips (unknown to pass, fail to deleted) are not independently proven by the ratchet
(WR-01, WR-02). GitHub collapses `linguist-generated` files by default, so the 1,462-line diff and any later one-row flip need a click
to see. Reasonable for the first commit, a quiet loss of review surface afterwards.
**Fix:** Keep it for `catalog.tsv` and `src/cartdb.inc`; drop it for `ledger.csv`, or accept and note it in `docs/conformance.md`.

### IN-02: Markdown escaping neutralises `[]()<>` but not GFM bare autolinks

**File:** `scripts/compat-common.sh:18` (`md`)
**Issue:** A title containing `www.example.org`, `https://host/x` or a mail address is rendered as a link on GitHub: `.`, `:`, `/`
and `@` are not escaped and the `<` that would block `<url>` is escaped but bare URLs need no `<`. Titles are catalog data and
today's 1,444 are clean; a pin bump or a hand edited row is the path. The comment says escaping exists so a title cannot "become a
link". The 64-byte cut is applied after escaping and can leave a dangling `&#9` (renders literally, harmless).
**Fix:** Also entity-encode `:` and `@` (`&#58;`, `&#64;`) in `md()` (two list entries) or render titles inside a code span.

### IN-03: Parser hardening misses other invisible and line-breaking code points

**File:** `scripts/compat-common.sh:61`
**Issue:** The list covers U+200B-200F, U+202A-202E, U+2066-2069, U+FEFF. Accepted: U+2028/2029 (a line break in JavaScript and some CSV
readers), U+2060-2064 (invisible operators), U+00AD, U+061C, U+180E, U+034F, variation selectors, tag characters, and C1 controls
(`[[:cntrl:]]` under `LC_ALL=C` does not cover 0x80-0x9F). The page prints `?` for every byte >= 0x80, so only the CSV and PR diffs are
exposed. Raw invalid UTF-8 is also accepted by the ledger parser (only the generator checks it).
**Fix:** Add `\342\200[\250\251]`, `\342\201[\240-\244]`, `\302[\200-\237\255]` to the one regex; leave the rest.

### IN-04: `compat-update.sh` now honours `COMPAT_CATALOG` and `COMPAT_LEDGER` from the environment

**File:** `scripts/compat-update.sh:3-4`
**Issue:** Before this phase the catalog path was hard-wired ("immutable CC0 identities", `repository-checks.txt`); now an exported
variable in the maintainer's shell redirects the identity source and the ledger, and `library.sh` itself exports `COMPAT_CATALOG`.
It exists for test cost (D13) and the owner is the only caller, so exposure is low, but a production script gained a test seam.
**Fix:** Keep the seams, and have `compat-update.sh` print `compat-update: catalog <path>` to stderr when `COMPAT_CATALOG` is not the
tracked one, or read the seams only when `COGMASTER_TEST=1`.

### IN-05: The exit block of `library.sh run` judges the pre-merge ledger when the merge is held

**File:** `scripts/library.sh:81-89`
**Issue:** The `exit:` block always reads `$ledger`; the merge copies `next-ledger.csv` over it only on success. When the merge is held
(page over the cap, ratchet refusal) the block reports the unchanged ledger's counts (`cart_pass=0/20 ... unmet`) next to a
`merge: held` line, which a reader could take as "the run reached nothing". The line `exit: failed (N)` plus its first message line
is also derived from CMake's wrapped `FATAL_ERROR` text; a long message may be cut at the wrap.
**Fix:** Run the command over `$out/next-ledger.csv` when the merge was held, or print `exit: over the unmerged ledger` in that case.

### IN-06: Hygiene and cosmetics

**File:** `scripts/library.cmake:130-139`, `tests/compat/exit.cmake`
**Issue:** (a) The `pal:` and `fm:` count lines in the library summary include runs for ids whose facts `update` discards (`skipped`
carts still run), so the printed counts can exceed what reaches the ledger; the synthetic test asserts `pal: carts=2 ran_pal=2` with a
skipped image in the set, which pins that behaviour. (b) A `reasons.txt` line with a trailing comment parses in the awk (`ok[rs[1]]=rs[2]`)
but is dropped by `exit.cmake`'s `file(STRINGS ... REGEX "...(open|retired)$")`, so the two readers disagree; fail-closed, but
confusing. (c) The cap on `reasons.txt` (40 lines, D2) is not enforced anywhere in the parser. No planning IDs, personal paths or
e-mail addresses appear in the changed code, tests or `quality.json`.
**Fix:** (a) say "runs" not "carts" in the label or exclude discarded ids; (b) reject any non-matching non-comment line in the parser's
`reasons()`; (c) `NR>40` check in `reasons()` or a line in `compat.reasons`.

## Checked and found sound

- **R1 pass/fail on unsupported/skipped ids:** refused (exit 2) with the head catalog honest; the stand-in cases cover both statuses. `allow.txt` cannot exempt R1, R2 or R3 (lines 105-108 never read `allowed[]`); it exempts only the existing base-`pass` rules and requires a behaviour-revision bump.
- **R2/R3 per-key on `unsupported` and `skipped`:** a vanished row is refused unless `rel()` holds; `still_unsupported_but_passing`, `retired_token_catalog_unchanged`, `catalog_changed_token_open` and `unsupported_vanished_token_open` are real negative controls (the harness also asserts a failing control prints no `passed` line).
- **Compound ids and counts:** the `sha1("mame/<list>/<name>")` ids are checked by CMake `string(SHA1)`; `wl_count` compares id sets both ways with the pin arithmetic; `wl_fresh` is non-vacuous (a changed title regenerates the row with the 1970 date). Row cap 8,192 x about 330 B is 2.7 MB, under the 4 MiB file cap; the exit command's own cap is half of it (4,096), above the 3,567 library worst case.
- **Update guard and library adoption:** candidates for catalog ids whose status is not `unknown`, with a machine or mapper differing from the catalog, a non-pad port, a non-`boots@600` predicate or a profile outside the 3-per-SMS/2-per-GG whitelist are discarded; the title and `core=<rev>` evidence come from the catalog and the environment, so no path or private string reaches a row. `ran` correctly stops a second default run and still derives the PAL profile.
- **Exit command:** floors use `LESS`, so 20 is met and 19 is unmet; `exit: failed` is printed with its leg and never changes the script status; a missing floor key is an error, not zero; the `met` branch is taken in the self test; compound ids and `fixture/` ids do not count. A floor lowered in `quality.json` needs a new `reasons` entry (`quality.cmake` base comparison).
- **Portability:** the octal byte classes behave the same on BWK (macOS, the suite), mawk 1.3.4 (Docker, verified) and gawk (the Windows legs of run 43158408243 are green); CRLF ledgers normalise (tested by `compat.csv`); `.gitattributes` forces LF so a CRLF catalog does not reach the seed.
- **ci.yml:** the only change is the `build` job timeout; no step, trigger or release input moved.

---

_Reviewed: 2026-11-12_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
