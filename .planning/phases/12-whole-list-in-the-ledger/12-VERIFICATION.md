---
phase: 12-whole-list-in-the-ledger
verified: 2026-11-12T06:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".gitattributes"
  - ".github/workflows/ci.yml"
  - ".planning/phases/12-whole-list-in-the-ledger/12-01-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-01-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-02-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-02-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-03-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-03-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-04-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-04-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-05-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-05-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-06-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-06-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-07-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-07-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-08-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-08-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-09-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-09-SUMMARY.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-10-PLAN.md"
  - ".planning/phases/12-whole-list-in-the-ledger/12-10-SUMMARY.md"
  - "ASSET_POLICY.md"
  - "PROVENANCE.md"
  - "README.md"
  - "THIRD-PARTY-NOTICES.md"
  - "docs/boards.md"
  - "docs/compatibility.md"
  - "docs/conformance.md"
  - "docs/embedding.md"
  - "docs/maintaining.md"
  - "docs/testing.md"
  - "include/cogmaster.h"
  - "libretro/cogmaster_libretro.info.in"
  - "quality.json"
  - "scripts/compat-common.sh"
  - "scripts/compat-seed.sh"
  - "scripts/compat-update.sh"
  - "scripts/library.cmake"
  - "scripts/library.sh"
  - "src/cart.c"
  - "src/cartdb.inc"
  - "tests/api/v14.c"
  - "tests/cart/catalog.cmake"
  - "tests/cart/generator.cmake"
  - "tests/cart/test_cartdb.c"
  - "tests/cart/tests.cmake"
  - "tests/cmake/flag.awk"
  - "tests/cmake/repository.sh"
  - "tests/compat/catalog.tsv"
  - "tests/compat/exit.cmake"
  - "tests/compat/ledger.csv"
  - "tests/compat/migration.cmake"
  - "tests/compat/pin-counts.txt"
  - "tests/compat/ratchet.cmake"
  - "tests/compat/reasons.txt"
  - "tests/compat/tests.cmake"
  - "tests/compat/update.cmake"
  - "tests/compat/whole_list.cmake"
  - "tests/compat/whole_list_common.cmake"
  - "tests/core/test_core.c"
  - "tests/inventory/ci.txt"
  - "tests/inventory/hygiene.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/library/tests.cmake"
  - "tests/policy/claims_cases.cmake"
  - "tests/policy/repository-checks.txt"
  - "tests/policy/tests.cmake"
  - "tests/runner/tests.cmake"
  - "tools/cartdb.c"
covered_digest: "v4:sha256:5d6cfe36277eee1811be1788870796ec874fd56eb17581c390a53a8aafe94d50"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 12: The whole list in the ledger Verification Report

**Phase Goal:** Every SMS and Game Gear entry at the MAME pin has a ledger row with an honest status, and the libretro core drops `is_experimental` when the command over the owner's judged rows passes (LEDG-05 as amended by the owner decision of 2026-11-12: the flag stays this phase; the mechanism that removes it is built and proven).
**Verified:** 2026-11-12T06:00:00Z
**Status:** passed
**Re-verification:** No, initial verification (head 0b33c35)

## Goal Achievement

### Observable Truths

| # | Truth (ROADMAP criterion as amended) | Status | Evidence |
|---|---|---|---|
| 1 | One default-profile row for every SMS and GG catalog identity; generator prints the SG-1000 count and the 1,476 = 1,444 + 19 + 13 reconciliation; a test equates the ledger with the printed counts (LEDG-01) | VERIFIED | Regenerated from the pinned lists (`fetch-pin.sh mame-sms`, `mame-gamegear`, `mame-sg1000`, then the generator with `--catalog --counts`): exit 0, `cmp` equal for `src/cartdb.inc`, `tests/compat/catalog.tsv` and `tests/compat/pin-counts.txt`. Stderr: `ledger identities 1463 = 1476 entries - 13 same-image aliases (1444 images + 19 compound); SG-1000 excluded 214 entries (212 images + 2 compound)`. Ledger: 1472 rows = 1463 catalog identities + 9 fixture rows (the 10 rows of origin/main, nine fixture passes and one unknown cart row, are all byte-identical in the head). `compat.whole_list.count` prints `rows=1472 cart_identities=1463 images=1444 compound=19 sg1000=214 fixture_rows=9` and its three failing controls (removed identity, pin counts one low, stray id) were read in the test source and pass. `compat.whole_list.fresh` proves the seed reproduces the ledger bytes. |
| 2 | Every unsupported/skipped row has a closed reason token; the slot table refuses its SHA-1 (exit 77); MAME supported="no" is skipped mame_no; a row without a token fails the parser (LEDG-02) | VERIFIED | Ledger: 5 unsupported (xterminator 2, seojin, janggun, graphic) and 24 skipped (multi_chip 12, mame_no 6, short_dump 3, bad_dump 2, no_rom 1), every one with `reason=<token>` listed in `tests/compat/reasons.txt`. The pinned XML holds supported="no" on 4 SMS + 2 GG entries (sg1000: 2, excluded) = 6 = `mame_no` rows. Control run: replacing `reason=graphic` by `-` in the ledger makes `compat-common.sh normalize` exit 2; deleting `graphic open` from reasons.txt fails `compat.reasons`, `compat.exit` and `compat.whole_list.reason`. Unit loop in `tests/cart/test_cartdb.c` over every board-0 public row (machine != SG-1000) asserts `UNSUPPORTED_BOARD`, slot id and reason, and exactly one Graphic Board row sitting in the `graphic` slot (revision 17). Mutating the Graphic Board row back to board 1 fails `cart.database` and `compat.whole_list.reason`. Chain of proofs, stated honestly: no synthetic image can reach the database path (a hit needs the exact SHA-1 of the one real dump), so the Graphic Board exit 77 is proven as unit loop (slot table refuses) + runner mapping of `UNSUPPORTED_BOARD` to `COGMASTER__EXIT_REFUSED` (77) in `runner/main.c` + runner runs that exit 77 for a board-0 image (`gg-8k.gg`, `janggun-board.sms`, the latter where a signature exists); the supported board control exits 0. |
| 3 | Ratchet R1-R3: unsupported -> pass only with a retired token and a changed catalog; failing controls; the PR adding the rule is judged by the previous rules (LEDG-03) | VERIFIED | `compat.ratchet` and `compat.trusted_judge` pass. Mutations (each restored, tree clean): dropping the R1 line (fail/unsupported/skipped changed or deleted without `rel()`) fails `compat.ratchet`; dropping the head pass/fail-vs-catalog-locked line fails with `r1_unsupported_pass: expected exit 2, actual 0`; dropping `!rel(key)` from the per-key rule fails it. OLD base judge from `git show origin/main:scripts/compat-common.sh` and `compat-ratchet.sh`, base ledger (10 rows) vs head ledger, revisions 16 17: `compat: protected comparison passed`, exit 0; head judge also exit 0; all 10 base rows are byte-identical in the head ledger, so nothing flipped. |
| 4 | `docs/compatibility.md` is aggregates plus capped evaluated rows (fixtures, unsupported, up to 40 fail, up to 40 others, `+K more`) inside its byte budget (LEDG-04) | VERIFIED | The page is the current output of `compat-common.sh docs` (cmp equal), 4,171 bytes; largest doc is embedding.md at 36,521 bytes against the 38,912 cap (more than 2 KiB under). Sections: groups, reasons, fixtures (9), unsupported rows (all 5), fail rows (none), other evaluated rows (none), each with a `+K more` line when over its cap. `compat.page_bound` (700-row hostile ledger) and `compat.page_bound.self_test` (caps raised in a copy) pass; budgets test: 887 check lines / 9403 src lines, limit 940, exit 0. |
| 5 | `compat.exit` exists, is proven on the committed ledger and a stand-in met ledger; a test ties `is_experimental` to the judged-cart floors both ways; every unsupported row names its reason; unowned rows stay unknown with the count printed (LEDG-05 as amended) | VERIFIED | Committed ledger prints `rows=1472 pass=9 fail=0 unknown=1434 unsupported=5 skipped=24`, `cart_pass=0/20 cart_pal_judged=0/1 cart_fm_judged=0/1 judged_cart_floors=unmet flag=present`, `unowned_unknown=1434 cart_pal_rows=0 cart_fm_rows=50`. Floors in `quality.json` (cart_pass 20, cart_pal_judged 1, cart_fm_judged 1, fixture floors 9/1/2) each with a reviewed reason. Tie, unmet direction: removing `is_experimental` from the .info template makes `compat.exit` fail (`is_experimental removed before the judged-cart floors are met`). Tie, met direction: setting the three cart floors to 0 (met) with the flag present makes it fail (`judged-cart floors met: remove is_experimental`). The stand-in met branch is in `compat.exit.self_test` (met 20/1/1 with flag absent exits 0; met with flag present, spaced and spelled variants exit 1; 19 passes stay unmet; twenty fixture or compound passes do not count; one child process runs the command itself). `scripts/library.sh` runs the exit command after the merge, prints its counts and `exit: failed (N)` and never turns its status into the script's own exit status. `is_experimental` and the word experimental stay (owner decision 2026-11-12); SEED-013 carries the owner's own-library run. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `tests/compat/ledger.csv` | 1472 rows | VERIFIED | seed reproduces it; counts equate with pin-counts |
| `tests/compat/catalog.tsv`, `pin-counts.txt`, `src/cartdb.inc` | generator outputs | VERIFIED | byte-identical to a fresh regeneration from the pin |
| `tests/compat/reasons.txt` | closed token list | VERIFIED | open tokens equal the slot table's board-less slots plus the five catalog reasons |
| `tests/compat/exit.cmake` | exit command and self test | VERIFIED | assert and self_test cases pass; negative controls above |
| `scripts/compat-common.sh` | reason rule, R1-R3, bounded page | VERIFIED | mutation controls fail as expected |
| `scripts/compat-seed.sh`, `library.sh` | seed, library exit block | VERIFIED | `compat.whole_list.fresh`, library tests pass |
| `include/cogmaster.h`, `src/cart.c`, `src/cartdb.inc` | Graphic Board slot only | VERIFIED | diff versus origin/main: behaviour revision 16 -> 17, one slot row, one fact row moved to board 0 slot 21, one comment; API 14 and state format 16 unchanged |
| `.github/workflows/ci.yml` | timeouts only | VERIFIED | one changed line: build timeout 6 -> 9 with comment `2 x 252 s, slowest of 40 runs, run 43158408243` |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| ledger.csv | catalog.tsv, pin-counts.txt | `compat.whole_list.count` | WIRED | 3 failing controls |
| ledger status | compiled slot table | `compat.whole_list.reason` | WIRED | fails when the graphic row is moved |
| ledger reason | reasons.txt | parser rule in compat-common.sh | WIRED | missing token exits 2 |
| quality.json floors | .info.in flag | `compat.exit` | WIRED | both directions fail |
| docs page | caps | `compat.page_bound` | WIRED | caps raised in a copy shows the bound is the caps |
| runner | refusal | `load_status` -> exit 77 | WIRED | board-0 runner runs |

### Data-Flow Trace (Level 4)

Not applicable beyond the above: the page is generated from the ledger by the same script the test compares (cmp equal); counts printed by `compat.exit` come from the parser's output over the real file.

### Behavioral Spot-Checks and Commands

| Command | Result | Exit |
|---|---|---|
| `cmake --workflow --preset ci` (twice, once after the mutation controls) | 666 + 5 tests, 100% passed (`ctest -N` 671 = inventory 673 lines minus 2 comments) | 0 |
| `cmake --workflow --preset hygiene` | 84/84 passed incl. hygiene.tree (15.6 s) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 887 check lines / 9403 src lines, strict one tenth (limit 940) | 0 |
| Regeneration from pinned lists, then `cmp` against three files | identical | 0 |
| OLD base judge (origin/main scripts and ledger) 16 -> 17 | `protected comparison passed` | 0 |
| Negative controls (mutate, rebuild or rerun, see red, restore) | flag removed while unmet; floors zeroed with flag present; R1; R3; reason token removed; `reason=` removed from a row; Graphic Board row moved: all red; tree clean after each | n/a |

### Probe Execution

None declared by the phase; no `probe-*.sh` applies.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| LEDG-01 | 12-01, 12-05 | whole-list rows, counts, reconciliation | SATISFIED | truth 1 |
| LEDG-02 | 12-01, 12-02, 12-05 | closed reasons, exit 77, mame_no, parser rule | SATISFIED | truth 2 (Graphic Board exit 77 is a chain of proofs, not an image) |
| LEDG-03 | 12-03 | ratchet R1-R3, judged by previous rules | SATISFIED | truth 3 |
| LEDG-04 | 12-04 | bounded page | SATISFIED | truth 4 |
| LEDG-05 | 12-06, 12-07, 12-08 | exit command, flag tie both directions, SEED-013 hand-off | SATISFIED as amended | truth 5; the flag stays by owner decision; the owner's own-library run is SEED-013, status `not_run`, `human_judgment: true`, never a gate and not UAT |

No orphaned requirements: REQUIREMENTS.md maps LEDG-01 to LEDG-05 to Phase 12 and each appears in a plan.

### Anti-Patterns Found

None blocking. TBD/FIXME/XXX scan of the changed non-planning files: no hit. No planning IDs in code, tests or user docs. The only `v2.0` in user prose is a game title in the generated page (`Sega Graphic Board (Japan, prototype v2.0)`); `policy.claims_scoped` guards the milestone wording. No ROM, BIOS or personal path (hygiene passed).

### Hosted Evidence

| Dispatch | Run | Head | Result |
|---|---|---|---|
| D1 | 43153144670 | b2d6993 | success, all jobs incl. CI required |
| D2 | 43158408243 | 291d5f5 | success, all jobs incl. CI required |
| D3 | 43163239085 | 0b33c35 (this head) | success when read (status completed, conclusion success; all jobs green incl. be32, wasm32, retroarch, hash-equality, CI required) |

### Advisory Notes (not gaps)

1. Hosted timeouts are exactly 2 x S from one or two windows-arm64 (or macOS) samples. The largest are compat.ratchet 142 s (2 x 70.79 s), the 160 s test (2 x 79.87 s, one sample), and the 104 s test (2 x 51.56 s). Each runs at half its limit on the slowest lane by construction, locally they take seconds. A doubling on that lane would trip them; the pull request run adds samples and the weekly digest will show it.
2. The build timeout moved 6 -> 9 min (2 x 252 s, windows-arm64, run 43158408243); D3 took 201 s on that lane, D1 214 s.
3. The page caps unsupported rows at 20 (`cap_unsupported`) with a `+K more` line; the requirement text says "every unsupported row". At the pin there are 5, all listed, so it holds today; the cap is the bound-by-construction guard and `compat.page_bound` proves it.
4. `cart_fm_rows=50` with `cart_pal_rows=0` in the exit block: the seeded default profiles include Japan/FM rows but none PAL, which is the intended gap the owner's run fills (SEED-013).
5. The ROADMAP v2.0 progress row (`0/8`, `0/TBD`) is stale for all phases; it is a planning table, not a Phase 12 matter.

### Human Verification Required

None. The owner's own-library run is the carried hand-off SEED-013: `not_run`, `human_judgment: true`, never a gate and not part of this verification.

### Gaps Summary

No gaps. All five roadmap criteria, as amended, are met by code, tests and hosted evidence.

---

_Verified: 2026-11-12T06:00:00Z_
_Verifier: Claude (gsd-verifier)_
