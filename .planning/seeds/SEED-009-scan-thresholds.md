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
