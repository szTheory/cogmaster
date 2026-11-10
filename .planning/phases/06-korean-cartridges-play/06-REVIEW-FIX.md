---
phase: 06-korean-cartridges-play
fixed_at: 2026-11-10T00:00:00Z
review_path: .planning/phases/06-korean-cartridges-play/06-REVIEW.md
iteration: 1
findings_in_scope: 6
fixed: 1
skipped: 5
status: partial
---

# Phase 6: Code Review Fix Report

**Fixed at:** 2026-11-10
**Source review:** .planning/phases/06-korean-cartridges-play/06-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings selected by the orchestrator: 6 (WR-01, WR-02, WR-03, IN-01, IN-02, IN-03)
- Fixed in code: 1 (WR-01)
- Deferred as seed entries: 5 (AGENTS rule 7: test and tooling findings become seeds; WR-02 is a rare headerless-image edge)

## Fixed Issues

### WR-01: Nemesis is recognised from a single byte

**Files modified:** `src/cart.c`, `tests/cart/test_cart.c`, `docs/boards.md`
**Commits:** 5ed720b (fix), 7fba1bb (comment reworded: the `hygiene.no_float` scan read a hex offset written in the comment as a float literal)
**Applied fix:** the heuristic now promotes a 128 KiB Zemina-family image to Nemesis only when bytes 0 to 2 are zero and
`F3 ED 56` (DI; IM 1) starts the last 8 KiB bank. The comment cites SMS-CARTRIDGES-AND-PERIPHERALS.md section 2 (scan
step 5) and the MAME `sega8_nemesis_device` already cited in `src/mapper_ext.c`. Tests: a lone `F3` at the bank start stays
Zemina, `F3 ED 56` with non-zero bytes 0 to 2 stays Zemina, the full signature is Nemesis, and the full signature with
byte 2 non-zero is Zemina. The old one-byte test was reshaped to carry the full signature; no fixture relied on the
one-byte rule (the three WLA-DX fixtures are Korean, Zemina and 4 PAK only). `docs/boards.md` describes the new rule.
Logic change, so: fixed, requires human verification.

## Deferred Issues

All five are appended to `.planning/seeds/SEED-009-scan-thresholds.md` (commit 91eff4b), section "Deferred review
findings (phase 6 code review, round 1)".

- **WR-02** (`src/cart_scan.c:51-58`): headerless Sega image with three or more cartridge-RAM stores to `$8000-$BFFF` can scan as Korean or 4 PAK. Rare edge; mitigations recorded.
- **WR-03** (`tests/cart/test_cart.c`): no test of the DD/FD/ED/CB length decoding (`length_at`, `uses_hl_memory`).
- **IN-01** (`src/cart_scan.c:84-88`): a DD/FD prefix hides a real `LD (nn),A`; undercount only, toward refusal.
- **IN-02** (`src/mapper_ext.c:23-24`, `src/state.c:313`, necessity): the `pages != 0` guards cannot trigger; kept as defence in depth, mirroring `src/mapper.c`.
- **IN-03** (`tools/cartdb.c`): slot table has no check for id of 32 or more, or a name of 16 or more characters.

## Verification

Gates ran in the main checkout (no worktree; the fixer edited the phase branch directly), from the repository root.

| Command | Exit status |
|---|---|
| `cmake --workflow --preset ci` (after the final code change) | 0 |
| `cmake --workflow --preset hygiene` (first run, after 5ed720b) | 8: `hygiene.no_float` flagged the comment in `src/cart.c`; fixed in 7fba1bb |
| `cmake --workflow --preset hygiene` (after 7fba1bb) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 |
| `clang-format --dry-run src/cart.c tests/cart/test_cart.c` (llvm@21) | 0 |
| `git diff origin/main --numstat -- tests/audio tests/testcard_pins.cmake` | empty: no hash moved |
| `git diff origin/main --numstat -- tests/built`, lines removed | none |

---

_Fixed: 2026-11-10_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
