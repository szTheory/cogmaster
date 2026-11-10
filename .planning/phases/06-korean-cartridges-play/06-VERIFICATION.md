---
phase: 06-korean-cartridges-play
verified: 2026-11-10T00:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".planning/phases/06-korean-cartridges-play/06-01-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-01-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-02-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-02-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-03-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-03-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-04-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-04-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-05-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-05-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-06-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-06-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-07-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-07-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-08-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-08-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-09-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-09-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-10-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-10-SUMMARY.md"
  - ".planning/phases/06-korean-cartridges-play/06-11-PLAN.md"
  - ".planning/phases/06-korean-cartridges-play/06-11-SUMMARY.md"
  - "docs/boards.md"
  - "include/cogmaster.h"
  - "quality.json"
  - "runner/probe.c"
  - "src/cart.c"
  - "src/cart_scan.c"
  - "src/mapper_ext.c"
  - "tests/cart/test_cart.c"
  - "tests/compat/catalog.tsv"
  - "tests/fixtures/tests.cmake"
  - "tests/mapper/test_mapper_ext.c"
  - "tests/runner/tests.cmake"
  - "tools/cartdb.c"
covered_digest: "v4:sha256:5cb1405e5d2395cdfdb694d35d1f2eb2a659614a298da283ba0775576bf42af0"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 6: Korean cartridges play Verification Report

**Phase Goal:** A collector's Korean-board cartridges in the CC0 lists load and play with no setting, detection never turns a Sega game into a Korean board, unsupported boards say why, and no shipped frame or audio hash moves.
**Verified:** 2026-11-10
**Status:** passed
**Re-verification:** No, initial verification

## Commands re-run by the verifier (repository root, branch phase/06-korean-cartridges-play, HEAD 1e61a12)

| Command | Exit | Result |
| --- | --- | --- |
| `cmake --workflow --preset ci` | 0 | 434 of 434 tests pass, plus 4 release tests |
| `cmake --workflow --preset hygiene` | 0 | |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 | 723 check lines / 7961 src lines, strict one tenth |
| `ctest --test-dir build/ci -R '^policy\.quality$'` | 0 | Passed (the quality check) |
| `git diff --quiet origin/main -- tests/audio tests/testcard_pins.cmake` | 0 | no audio or test card pin moved |
| `git diff origin/main -- tests/built > file; grep -c '^-[^-]' file` | 0 (git) | prints 0; the diff is 3 added lines |
| `gh run view 42787536409` | | conclusion success, head 778376a; hygiene, asan, be32, nofp x2, build x7 legs, perf, wasm32, retroarch, hash-equality, CI required all success; title, cleanup, rehearsal skipped by design |
| `gh run view 42781709144` and `42781713344` (06-04 perf runs: branch and main) | | both success (workflow_dispatch; main at 8c97f8e, which is still origin/main and the merge base) |

## Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Generator prints a count per MAME slot, maps every pinned slot to a board or the unsupported list, fails on an unmapped slot; hash table holds binary SHA-1 rows with titles on the ledger side; cartdb.c.o under 100 KB | VERIFIED | `tools/cartdb.c` has a 20-entry `slots[]` table (board or reason per slot), per-slot counts, and `unmapped slot %s` exit 1; `cart.generator` (in the ci run) covers the unmapped-slot failure and counts (header of `tests/cart/generator.cmake`). Hosted `size.cartdb measured=65168 ceiling=102400` (run 42787536409); `policy.quality` passes locally. Titles live in `tests/compat/catalog.tsv`. |
| 2 | Korean $A000, Hicom, Zemina, Nemesis, korean_nb play through runner and libretro with no option; Janggun or 4PAK plays or its catalog row is unsupported with a reason; Seo Jin and 188-in-1 are unsupported catalog rows naming what fails, runner exits 77 | VERIFIED | Boards exist in `src/mapper_ext.c` (korean, korean_nb, hicom, zemina, nemesis, 4pak), and catalog rows carry them: korean 6, korean_nb 5, hicom 9, zemina 8, nemesis 1, hes_4pak 1. Runner and libretro: `fixtures.mapper_korean`, `fixtures.mapper_zemina`, `fixtures.mapper_hes_4pak` judged through the mailbox, `fixtures.korean_probe` (scan finds each with no header, no setting), and `libretro.host.korean` (no core option set, frame equals the runner's) with a wrong-digest control. Hi-Com, korean_nb, Nemesis, Seo Jin and 188-in-1 are hash-only or register-level by the recorded CONTEXT decision. Janggun is an `unsupported` catalog row with reason "reverse bit 6 vs 7 unresolved (MAME and SMS Power disagree)"; seojin and korean_188in1 rows are `unsupported` with named reasons (`tests/compat/catalog.tsv` lines for seojin, korean_188in1, janggun); `runner.probe.unsupported` and `tests/runner/tests.cmake` lines 372-391 assert exit 77 and the reason text. |
| 3 | `--probe` reports `board_source`; a decoy with valid TMR SEGA header and Korean-looking bytes stays Sega; below threshold or margin does not pick a Korean board; a tie refuses naming the candidate | VERIFIED | `runner.probe.keys` checks `board_source=(db\|header\|heuristic\|default\|none)`, `board_candidate`, `scan_sega`; `tests/cart/test_cart.c` has "decoy", "below threshold", "tie", "tie with Sega", "operand decoy" and "sega margin" cases (`cart.probe.*` pass). |
| 4 | Each new board passes an exhaustive register-write and address-read test under ASan; documented register writes record no fault | VERIFIED | `mapper.ext.korean/korean_nb/hicom/zemina/nemesis/hes_4pak/state` exist and pass; the test sweeps writes and reads and checks the fault record (`fault_of(...).present == 0` for documented writes, `rom_write` and `mapper_range` for others). The asan preset excludes only the abi and release labels, so these run under ASan and UBSan; hosted asan job success on run 42787536409. |
| 5 | Eight-lane equality shows every pre-phase frame and audio hash unchanged | VERIFIED | hash-equality job success (8 `hashes-*` artifacts per 06-11-SUMMARY); local proof above: audio and test card pins unchanged, zero removed or changed lines in `tests/built`; perf `boot_to_play` delta_bp 0 (H=12302847, B=12302839, limit 100). |

**Score:** 5/5 truths verified, 0 behavior-unverified.

## Requirements Coverage

All eight IDs appear in plan frontmatter and in REQUIREMENTS.md (`[x]`, traceability "Phase 6 / Complete"); no orphaned ID.

| Requirement | Source plans | Status | Evidence |
| --- | --- | --- | --- |
| MAP-01 | 06-01, 06-03, 06-05 | SATISFIED | Generator slot table, per-slot counts, `unmapped slot` failure; `cart.generator` |
| MAP-02 | 06-01, 06-03, 06-05 | SATISFIED | Compact rows, titles in `tests/compat/catalog.tsv`; size.cartdb 65168 against 102400; `policy.quality` |
| MAP-03 | 06-02, 06-06, 06-07, 06-08, 06-10 | SATISFIED | Six boards in `src/mapper_ext.c`, scan in `src/cart_scan.c`, fixtures through runner and libretro with no setting |
| MAP-04 | 06-03, 06-07, 06-11 | SATISFIED | 4 PAK plays (`fixtures.mapper_hes_4pak`); Janggun is an `unsupported` catalog row with a reason |
| MAP-05 | 06-02, 06-03, 06-09, 06-11 | SATISFIED | seojin and korean_188in1 `unsupported` rows with reasons; runner exit 77 tests |
| MAP-06 | 06-02, 06-08, 06-09 | SATISFIED | `board_source` probe key; header-outranks-scan decoy, threshold, margin and tie tests |
| MAP-07 | 06-04, 06-06, 06-07 | SATISFIED | `mapper.ext.*` exhaustive tests; asan run covers them; no-fault assertions for documented writes |
| MAP-08 | 06-04, 06-10, 06-11 | SATISFIED | Hash-equality job on hosted run, pins diff empty, 0 removed built lines, perf delta 0 |

## Anti-Patterns and Review

Hygiene (including `hygiene.no_float`) and budgets pass. Code review: 06-REVIEW.md WR-01 (Nemesis from one byte) was fixed in 5ed720b and 7fba1bb with tests; WR-02, WR-03 and IN-01 to IN-03 are seeds in `.planning/seeds/SEED-009-scan-thresholds.md` per rule 7 (test and tooling findings become seeds; WR-02 is a rare headerless edge, mitigated toward refusal). No debt markers were treated as blockers.

## Notes (non-blocking)

- The final hosted run 42787536409 is on head 778376a. Three files changed afterwards by the review fix (`src/cart.c`, `tests/cart/test_cart.c`, `docs/boards.md`; the Nemesis signature rule). The local ci workflow (434 of 434), hygiene, and budgets pass on the current head, and no audio, test card or built pin changed. The hosted equality run on the pull request head is a ship-time confirmation (06-11-SUMMARY items 0 and 1).
- Ship-time confirmations per AGENTS rules 6 and 10 (the PR-run perf line, the hosted size.cartdb follow-up value, auto-merge, release pull request) are listed in 06-11-SUMMARY.md and do not affect this status.
- Real-dump scan threshold measurement is a seed (SEED-009; no private library configured).

## Human Verification Required

None. Every criterion is met by an automated check, command, exit status or run URL.

## Gaps Summary

None.

---

_Verified: 2026-11-10_
_Verifier: Claude (gsd-verifier)_
