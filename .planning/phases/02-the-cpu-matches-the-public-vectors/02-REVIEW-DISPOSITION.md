---
phase: 02
review: 02-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
  - id: WR-02
    severity: warning
    disposition: fixed
  - id: WR-03
    severity: warning
    disposition: fixed
  - id: WR-04
    severity: warning
    disposition: fixed
  - id: WR-05
    severity: warning
    disposition: fixed
  - id: IN-01
    severity: info
    disposition: deferred
  - id: IN-02
    severity: info
    disposition: deferred
  - id: IN-03
    severity: info
    disposition: deferred
  - id: IN-04
    severity: info
    disposition: deferred
  - id: IN-05
    severity: info
    disposition: deferred
  - id: IN-06
    severity: info
    disposition: deferred
open: 0
total: 11
recorded: 2026-10-20T06:20:00.000Z
---

# Phase 02: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 775b29c (02-REVIEW-FIX.md) |
| WR-02 | warning | fixed | 5b0ec0d (02-REVIEW-FIX.md) |
| WR-03 | warning | fixed | 3ec02c1 (02-REVIEW-FIX.md); symmetric write behaviour chosen where SMS-SYSTEM is silent, stated in src/bus.c |
| WR-04 | warning | fixed | f27e619 (02-REVIEW-FIX.md) |
| WR-05 | warning | fixed | 9f9076e (02-REVIEW-FIX.md) |
| IN-01 | info | deferred | fault record PC/opcode for prefixed ED holes and interrupt acceptance; diagnostic precision only, revisit with the phase 3 trace work |
| IN-02 | info | deferred | bus.vdp_at saved but unread; kept for the phase 3 VDP access-timing work |
| IN-03 | info | deferred | 1 MiB + 1..511 byte headerless images pad to 65 pages; memory-safe, size policy to settle with real-ROM admission |
| IN-04 | info | deferred | (int8_t) conversion is implementation-defined before C23 but two's-complement on every supported target; hash-equality lane guards it |
| IN-05 | info | deferred | --probe region_guess on unknown region and ignored --machine/--region; runner UX seed |
| IN-06 | info | deferred | saved fault record accepts values the core never records; tighten with the next state-format change |

Fix round 1 of 2 (project rule 7). Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
