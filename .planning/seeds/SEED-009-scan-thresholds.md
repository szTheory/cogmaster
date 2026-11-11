---
id: SEED-009
status: dormant
planted: 2026-11-10
planted_during: phase 6, plan 08
trigger_when: a private library is configured (COGMASTER_ROM_DIR names a directory), or any real Korean, Zemina or 4 PAK dump is probed and the scan refuses or misplaces it
scope: small
---

# SEED-009: measure the board scan thresholds on real dumps

## Why This Matters

The board scan picks a Korean, Zemina, Nemesis or 4 PAK board for an image the table does not hold. Every family
starts at a conservative threshold of three store sites, with margins of twice the Sega count plus two and twice
the runner-up's count plus two. Those numbers are a starting point, not a measurement: no private library was
configured when the scan landed, so no real dump has been counted.

## When to Surface

**Trigger:** the first time a private library is configured, or a real image the scan refuses, or one it sends to
the wrong board.

## Scope Estimate

**Small** — numbers only:
- Run `sh scripts/library.sh` with `COGMASTER_ROM_DIR` set and record only the command, its exit status and the
  per-source counts (`board_source`, `scan_sega`, `scan_candidate`, `scan_runner_up`); no file names, no bytes.
- Keep a threshold at three if every real Korean-family image clears it with room; raise one only on evidence.
- Judge the Sega margin against the Sega games in the library: none may become a Korean-family board, and few
  should land in the doubt zone.

## Breadcrumbs

- `src/cart.c` `scan_board`, `src/cart_scan.c`, `docs/boards.md` (how a board is chosen).
- `tests/cart/test_cart.c` `probe.scan` (the decoys and family images).
- `.planning/phases/06-korean-cartridges-play/06-CONTEXT.md`, "Detection and probe".

## Deferred review findings (phase 6 code review, round 1)

Each is a heuristic-quality or test-and-tooling item, so it waits here instead of becoming a requirement.

- **Headerless Sega image counted as Korean or 4 PAK (review WR-02).** A headerless or bad-checksum Sega image of
  64 KiB or more with three or more cartridge-RAM stores to `$8000-$BFFF` (`LD ($A000),A`, `LD ($BFFF),A`) can scan
  as Korean or 4 PAK when it reaches its mapper through `LD (HL),A` or `LD ($FFFC)` stores that count few Sega
  sites. A rare edge: database and header-valid images never reach the scan. Mitigations to try on real dumps: do
  not count `$A000` or `$BFFF` stores that follow a store of a Sega `$FFFC` value with bit 3 set (cartridge RAM
  enable), or raise the margin for those two targets.
- **Prefix length decoding has no test (review WR-03).** Add images that place `32` as an operand of `DD 36 d n`,
  `DD 21 nn nn`, `DD CB d op`, `ED 43 nn nn`, `FD 34 d` and `CB` forms and assert zero counts, each with a
  positive control (a real `LD ($A000),A` right after) that is counted. Targets `length_at` and `uses_hl_memory`
  in `src/cart_scan.c`.
- **A `DD` or `FD` prefix hides a real `LD (nn),A` (review IN-01).** `DD 32 nn nn` is `LD (nn),A` on the Z80 but the
  sweep skips it as one prefixed span, so it is not counted; `ED ED 43 ...` has the same shape. This only
  undercounts toward refusal. Count `next == 0x32` in the `DD`/`FD` branch, or note it in the function comment.
- **`pages != 0` guards for the extension boards (review IN-02).** `ext_bank` in `src/mapper_ext.c` and the limit in
  `src/state.c` cannot see `pages == 0` for a board of 4 or more. Keep them as defence in depth, mirroring
  `bank_of` and `bank_max` in `src/mapper.c`; revisit only if the three older boards drop theirs.
- **Slot table limits in `tools/cartdb.c` (review IN-03).** `check_table` does not reject `id >= 32` (the
  `counted` mask is 32 bits) or a name of 16 or more characters (the struct field is 16 bytes). The longest name is
  13 characters and there are 20 slots, so nothing breaks today; add both conditions to the
  "slot table row is not consistent" failure when a slot is added near either limit.

## Phase 7 review (saves)

- **Fixture-facts cap misreports a well-formed file (review IN-01).** In `tools/cartdb.c` the row-count cap is checked
  before the next row is read, so a file that has exactly the cap's number of rows fails with "missing fixture facts".
  Move the check after the read, or compare against `cap + 1`. No fixture file is near the cap today.
- **`--save-interval` argument edges (review IN-02).** `--save-interval 0` without `--save-dir` is accepted (0 means
  off) though `docs/runner.md` says the option needs `--save-dir`, and a repeated `--save-interval` is last-wins
  where `--expect` rejects a repeat. Pick one rule, document it in the option row, and add the case to
  `runner.usage.save_interval`.
