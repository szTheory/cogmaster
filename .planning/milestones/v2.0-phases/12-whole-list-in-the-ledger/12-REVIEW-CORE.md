---
phase: 12-whole-list-in-the-ledger
reviewed: 2026-11-12T00:00:00Z
depth: standard
files_reviewed: 22
files_reviewed_list:
  - include/cogmaster.h
  - src/cart.c
  - src/cartdb.inc
  - tools/cartdb.c
  - tests/api/v14.c
  - tests/cart/catalog.cmake
  - tests/cart/generator.cmake
  - tests/cart/test_cartdb.c
  - tests/cart/tests.cmake
  - tests/compat/catalog.tsv
  - tests/compat/pin-counts.txt
  - tests/compat/reasons.txt
  - tests/compat/whole_list_common.cmake
  - tests/runner/tests.cmake
  - libretro/cogmaster_libretro.info.in
  - README.md
  - docs/compatibility.md
  - docs/conformance.md
  - docs/maintaining.md
  - docs/testing.md
  - docs/boards.md
  - docs/embedding.md
findings:
  critical: 0
  warning: 10
  info: 6
  total: 16
  necessity: 2
status: issues_found
---

# Phase 12 (core, generator, wording half): Code Review Report

**Reviewed:** 2026-11-12
**Depth:** standard, with cross-file reasoning
**Files Reviewed:** 22 (plus ASSET_POLICY.md, PROVENANCE.md, THIRD-PARTY-NOTICES.md, .gitattributes read for wording)
**Status:** issues_found

## Summary

No blocker was found. I re-derived the generator's output rather than trusting the plan:

- I fetched the three pinned lists with `scripts/fetch-pin.sh` (sha256 matched `pins.txt`) and ran a fresh `cc` build of `tools/cartdb.c` over them. The catalog, `src/cartdb.inc` and `pin-counts.txt` are byte-identical to the committed files (`cmp`, exit 0).
- The counts hold. 653 / 823 / 214 entries; `grep -o 'sha1="..."' | uniq -c` gives exactly 13 duplicate SHA-1s, none involving the SG-1000 list.
- The ledger identities are 1,463 = 1,444 images + 19 compound. The catalog has 24 `skipped` (6 mame_no, 12 multi_chip, 3 short_dump, 2 bad_dump, 1 no_rom) and 5 `unsupported` (graphic, janggun, seojin, 2 xterminator). The six `supported="no"` entries (`ar`, `excdizzy`, `game188`, `sonicc630`, `wondrkid`, `par`) are all `skipped`/`mame_no`. The mixed `excdizzy` case is `skipped`, and `partial` stays `unknown`.
- The compound id literal `20608ba2...` equals `printf 'mame/sms/astropita' | sha1`.
- CRLF versions of all three lists produce an identical catalog.
- gcc:13 `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` is clean on `tools/cartdb.c`, `tests/cart/test_cartdb.c`, `src/cart.c`, `src/cartdb.c` and `tests/api/v14.c`. clang with `-Wsign-conversion -Wshadow` adds nothing; `clang --analyze` reports one pre-existing `fgetc` note.
- The `build/ci` ctest subsets `cart|api|abi|core|state|compat|policy|docs|libretro.info|hygiene` pass.

The Graphic Board slot is sound. The one row now has board 0 and slot 21. No owned fixture or other test references that SHA-1, and `owned_fixture()` cannot admit a code-4 row. The board-0 facts left in the table are SG-1000 rows, the five SMS/GG refusals plus `korean_188in1` and `mgear`, and no board-0 SMS/GG row without a slot exists. `status_of` fails the run if one appears. The behaviour revision is 17 in the header, `>= 16` in `v12`-`v14`, and compared to the macro in `v1` and `core`. No doc, fuzz seed or test hard-codes 16 as the current revision. `src/state.c:34` "behaviour revision 16" is a byte offset, not a revision. The state header writes the revision as a fixed field, so revision-16 states are refused as the docs say.

The defects are in the generator's hostile-input handling (silent truncation, quadratic time, an incomplete format-character list), a vacuous negative assertion in the new catalog test, a catalog-to-table tie that covers less than the docs imply, and several user-facing sentences that over-state or contradict the code.

## Warnings

### WR-01: Generator exits 0 and prints a consistent reconciliation when the input holds a NUL byte

**File:** `tools/cartdb.c:701-708`
**Issue:** `fread` fills `input`, then `input[n] = 0`, and every later scan uses `strstr`/`strchr`. A NUL byte anywhere in a list ends the parse there. I placed one right after a `</software>` in the pinned `sms.xml`. The run exited 0 and printed `ledger identities 328 = 328 entries - 0 same-image aliases (325 images + 3 compound)`, half the list, with no complaint. The reconciliation cannot catch this, because it counts only records the parser saw (see IN-03). The "bounded converter" and the maintaining doc say hostile input stops the run. This input silently shrinks the denominator instead.
**Fix:** After the read, reject embedded NULs:
```c
if (strlen(input) != n)
    fail("input holds a NUL byte");
```

### WR-02: Quadratic scan; a 2.4 MB list takes 24 s (about 70 s at the 4 MiB cap)

**File:** `tools/cartdb.c:411-418`
**Issue:** For each `<dataarea name='rom'>` the code runs `strstr(t, "</dataarea>")`, then `for (r = t; (r = strstr(r, "<rom ")) != NULL && r < ae; ...)`. When a rom-named dataarea contains no `<rom `, that `strstr` scans on to the next `<rom ` in the record, possibly the end. A record with 60,000 `<dataarea name='rom' size='1'></dataarea>` followed by one real area (2.46 MB) ran 24.5 s of CPU on this machine. The reproducer is a generated scratch file, not kept. Input is sha256-pinned and capped at 4 MiB, so this is bounded and not an attack on the core. It does contradict the phase's "bounded time on hostile XML" claim, and no test covers it. The test only covers CRLF-free, small records.
**Fix:** Bound the rom search to the area by copying `*ae` to NUL for the loop (`char save = *ae; *ae = 0; ...; *ae = save;`), or `memchr`-limit the search to `ae`. Add a hostile-record case to `tests/cart/catalog.cmake` with a timeout.

### WR-03: The title filter's "bidirectional format character" class is incomplete

**File:** `tools/cartdb.c:247-249`
**Issue:** The reject list is U+200B-200F, U+202A-202E, U+2066-2069 and U+FEFF. Unicode's `Bidi_Control` set also contains U+061C (ARABIC LETTER MARK). The filter also lets through U+0085, U+2028/U+2029, U+2060 and U+00AD. I ran all five through the generator, each as `Ab<char>c`, and each returned rc=0. The comment at 223-225 and `docs/testing.md` promise that a title cannot "spoof direction" or "hide characters". The committed titles are clean, so nothing ships wrong. A pin bump could add one and the generator would accept it silently. U+2028/2029 and U+0085 are line separators to many tools that read the ledger.
**Fix:** Add to the reject condition: `cp == 0x61cu || cp == 0x85u || cp == 0xadu || cp == 0x2028u || cp == 0x2029u || (cp >= 0x2060u && cp <= 0x2064u)`. Add one negative case for U+061C to `tests/cart/catalog.cmake`.

### WR-04: Vacuous negative assertion in the new catalog test (and it names the wrong record)

**File:** `tests/cart/catalog.cmake:129`
**Issue:** `if(got MATCHES "${h2}" OR got MATCHES "${gg_id}")` is meant to fail when "a record the table cannot hold reached the table". `got` is the generator's stdout, where SHA-1s print as `0x22,0x22,...`, never as 40 contiguous hex digits. Neither regex can ever match, so the assertion cannot fail. `grep -cE '2{40}'` on the authored run's output returns 0. `h2` is also the `notworking` record, which is single-chip and legitimately in the table (`grep -c 0x22,0x22,0x22` returns 1). Had the regex been correct, the test would fail. The same flaw is in the older `generator.cmake:74` (`33333333|44444444`), which is the pre-existing shape this was copied from. So the compound-exclusion-from-table fault (`if (f->kind) continue;`, `tools/cartdb.c:732`) is untested by this file.
**Fix:** Match the table's byte form for the compound ids and the multi-chip chips. Assert a literal that must be present too (the h2 row) so the check cannot pass vacuously:
```cmake
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${gg_id}")  # compound id as it would print
if(got MATCHES "0xaa,0xaa,0xaa" OR got MATCHES "0xbb,0xbb,0xbb")  # chips of cmulti (ha, hb)
  message(FATAL_ERROR "a record the table cannot hold reached the table")
endif()
if(NOT got MATCHES "0x22,0x22,0x22")
  message(FATAL_ERROR "a not-working single-chip record is missing from the table")
endif()
```

### WR-05: The catalog-to-table tie checks only machine and board; nothing ties the catalog to the pinned lists in CI

**File:** `tests/compat/whole_list_common.cmake:73-92` (and `tools/cartdb.c:568-575`)
**Issue:** `compat.whole_list.reason` compares each sms/gg catalog line with its `cartdb.inc` fact on sha1, machine and board name only. The seed copies catalog fields 4 (slot), 8 (region), 9 and 10 (ports) into the ledger profile (region, `port1`, `port2`), and nothing compares them with the fact's `slot`, `region`, `ctrl1`, `ctrl2`. A hand edit that changes a catalog `region` or `port1` passes every test. `compat.whole_list.fresh` then reproduces the ledger from the edited catalog, so the two stay consistent while both are wrong. The awk also checks one direction only. A table fact (sms/gg, flags&1) with no catalog line is never reported, and `nfact` is printed but not compared to the catalog count. A pin bump that regenerates `cartdb.inc` and forgets the catalog would add unledgered identities. A hand-edited catalog status (`unknown` to `skipped`) is caught only by `pin-counts.txt` for the `mame_no` count, and `pin-counts.txt` is itself hand-editable. I confirmed that no workflow or CI test runs `cogmaster-cartdb` over fetched lists (`.github/workflows/*.yml` has no `cartdb` or `mame-` fetch). The tie to the pin is therefore a manual step, which I reproduced and found correct today.
**Fix:** In `wl_table_awk`, also compare slot (name from the slot table at the top of `cartdb.inc`), region, and the two ports mapped from `ctrl1`/`ctrl2`. Add the reverse check (count of sms/gg facts with `flags = 1u` equals the catalog's non-compound sms/gg lines). A nightly job that fetches the pin and `cmp`s the generator output is already in SEED-015, so the tie stays manual until it lands.

### WR-06: `docs/maintaining.md` says tests compare a regeneration with the committed files; no test does

**File:** `docs/maintaining.md:229`
**Issue:** "A pin bump regenerates all three in one run, and the tests compare them with the committed files." No test regenerates from the lists. The tests compare the committed ledger with the committed catalog, and the catalog with `cartdb.inc` in part (WR-05). The sentence reads as a CI guarantee that a stale catalog or `pin-counts.txt` is caught. It is not, and `maintaining.md` is the maintainer's reference for this procedure.
**Fix:** "A pin bump regenerates all three in one run; compare the three files with `cmp` before committing. The tests check them against each other, not against the lists."

### WR-07: "Skipped is the MAME verdict" is false for 18 of the 24 skipped rows

**File:** `docs/compatibility.md:6` (generated by `scripts/compat-common.sh:124`) and `docs/conformance.md:117`
**Issue:** The page says "Skipped is the MAME verdict about the dump". Only `mame_no` (6) is MAME's verdict that the dump does not work, and `bad_dump` (2) is MAME's own `status="baddump"` flag. `multi_chip` (12), `short_dump` (3) and `no_rom` (1) mean this project's table cannot hold the record. `conformance.md:117` says it outright: "an entry MAME marks as not working, or whose record is not one image ... is `skipped`, which is the MAME verdict". That tells a reader MAME judged 18 compilation and multi-chip entries, and it will read as such once the flag leaves.
**Fix:** "Skipped means no check is run for the entry: MAME marks it not working or a bad dump, or its record is not a single image this table can identify; it is never recorded as a pass." Change both places (the generator text is in `scripts/compat-common.sh`, regenerate the page).

### WR-08: README says Mark III software is not tracked while claiming the core covers the Mark III

**File:** `README.md:12-13` (with `README.md:4`)
**Issue:** Line 4 says "One core covers the Mark III". The new status paragraph says "The page covers Master System and Game Gear cartridges only, so Mark III software is not tracked there." The Mark III is the Japanese Master System. Its software is the `(Japan)` entries of `sms.xml` (the list's own comments name it, and 360 catalog titles carry `(Japan`), and those rows are in the ledger with `region=japan`, `fm=on` profiles. The sentence is false as written. Read as "SG-1000/SC-3000 on a Mark III" it is unexplained, and `docs/controllers.md:17` ("the Mark III is not a profile") adds a third position. The wording task named exactly this coherence.
**Fix:** Delete the sentence (SG-1000 is already in "Not supported"), or say "Japanese Master System (Mark III) cartridges are listed under Master System."

### WR-09: README claims sound "in the runner"

**File:** `README.md:7`
**Issue:** "Runs Master System and Game Gear cartridges with sound in RetroArch and the runner". The runner is headless and has no audio device (project rule 2). `docs/runner.md:10` says the chips "are heard through `--hash-audio`", which prints a hash. The previous text said "Games play with sound" in the RetroArch context. The new text claims audible sound in a tool that emits none.
**Fix:** "Runs Master System and Game Gear cartridges with sound in RetroArch; the runner reports the same picture and audio hashes, identical on every platform."

### WR-10: ASSET_POLICY sentence is wrong for the 1,434 unknown rows

**File:** `ASSET_POLICY.md:25`
**Issue:** "A ledger row says what our core did with a dump the maintainer owns". After this phase 1,463 of 1,472 rows are metadata only. Their evidence is `CC0 metadata only src/cartdb.inc` or `reason=<token>`, and the core did nothing with any dump. This is the policy text that licenses committing about 1,650 commercial titles. Its basis (CC0 MAME metadata, no bytes, no output digest) is correct and is stated in `PROVENANCE.md` and `THIRD-PARTY-NOTICES.md`. This sentence mis-describes the rows it is there to justify.
**Fix:** "A ledger row is public catalog metadata plus, for rows the maintainer judged, what our core did with a dump the maintainer owns; it holds no dump byte and no digest of our output."

## Info

### IN-01: Behaviour revision 17 is pinned by no test

**File:** `tests/api/v14.c:13`, `include/cogmaster.h:16`
**Issue:** `v14.c` now accepts `>= 16`, as the `v13.c` precedent does, and `v1.c` and `core` compare to the macro. Nothing asserts that the revision is at least 17 or ties it to the Graphic Board refusal. Reverting the header to 16 passes every test; only the behavioural `cart.database` loop would still catch a revert of the table row. The state header would then accept revision-16 states that ran the dump as a pad.
**Fix:** One line in a new or existing test: `CHECK(COGMASTER_BEHAVIOUR_REVISION >= 17u)` beside the board-0 loop in `tests/cart/test_cartdb.c`.

### IN-02: New runner assertions duplicate existing ones and do not exercise the Graphic Board path (necessity)

**File:** `tests/runner/tests.cmake:959-963`
**Issue:** `expect_exit(77 ... --machine gg gg-8k.gg)` repeats `tests/runner/tests.cmake:985` (`expect_exit(77 "unsupported" --frames 1 --machine gg ${SEEDS}/gg-8k.gg)`), and `expect_exit(0 "^$" ... di-halt.sms)` repeats the `di-halt.sms` runs at 489 and 849. Neither goes through the table refusal. The 8 KiB image is refused by the `CART_GG_DEVICE` heuristic in `guess_board`, which is a different path from `cart_fact_to_info`. No runner test can reach the Graphic Board SHA-1 without its bytes, so the "runner exit 77 chain" for that row rests on the unit loop in `test_cartdb.c` plus the generic `load_status` case. The comment claims a fault prevented that the added lines do not prevent.
**Fix:** Drop the two lines (the unit loop already pins the table path), or rewrite the comment to say they cover the heuristic refusal. Category: necessity.

### IN-03: The "fail-closed" reconciliation cannot fail on any input

**File:** `tools/cartdb.c:598-600`
**Issue:** Each `<software ` record yields exactly one fact, and each merge increments `merged_*`. So `entries - merged == images + comps` holds by construction, and the NUL-truncation run in WR-01 passes it. It guards against a coding slip in `coalesce`, not against bad data. The independent evidence is the printed line compared with `pin-counts.txt` in `compat.whole_list.count`, which is hand-maintained. The docs call the arithmetic fail-closed.
**Fix:** Describe it as a self-check, or (better) have `write_counts` read the previous `--counts` file when it exists and fail on a difference unless `--update` is given.

### IN-04: Dead tab/CR/LF guard in `write_catalog` (necessity)

**File:** `tools/cartdb.c:563-564`
**Issue:** `check_title` rejects every byte below 0x20 (`tools/cartdb.c:231`), including tab, CR and LF, for every title that reaches `facts[]` through `parse`. Fixture facts, whose `title` comes from `fscanf` unchecked, have `public_identity` 0 and are skipped at line 561. `strpbrk(f->title, "\t\r\n")` can therefore never fire. Category: necessity.
**Fix:** Remove the check, or keep it with a comment saying it is a second defence for the catalog's line format.

### IN-05: A future 8 KiB Game Gear image stops the generator with no way forward

**File:** `tools/cartdb.c:442-443, 545-546`
**Issue:** `if (f.machine == 2u && f.bytes == 8192u) f.board = 0u;` leaves `slot` 0. `status_of` then calls `fail("a Master System or Game Gear record has board 0 and no slot")`. No pinned entry takes this path (the run passes), so it is fail-closed and correct today. A pin bump that adds one needs a slot-table row first, and the message does not say so. `fail` is also not marked noreturn, so `slots[f->slot - 1u]` on the next line is a `-1u` index in the analyzer's view.
**Fix:** Extend the message: "add a slot to the table in tools/cartdb.c". Optionally `static void fail(const char *why) __attribute__((noreturn))` behind a portable macro.

### IN-06: Small doc inaccuracies around the new fields

**File:** `docs/conformance.md:117`, `docs/boards.md:112`
**Issue:** The catalog description lists the status values as `unknown`, `unsupported` or `skipped`. The catalog also carries `excluded` for SG-1000 lines, and a `board` value `compound`. `boards.md:112` explains the new row by what it "once read" as, which is history, in a reference list whose other items state only the present reason.
**Fix:** Add `excluded` and `compound` to the field description. Drop "where it once read as a plain pad" (it is in `embedding.md:61`, where a behaviour revision belongs).

---

_Reviewed: 2026-11-12_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
