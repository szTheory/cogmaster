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
    disposition: fixed
  - id: IN-04
    severity: info
    disposition: skipped
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
| IN-01 | info | deferred | SEED-004: fault record PC/opcode for prefixed ED holes and interrupt acceptance; diagnostic precision only, revisit with the trace and hook work |
| IN-02 | info | deferred | SEED-002: bus.vdp_at is saved but unread; drop it or use it when the state visitor is rewritten |
| IN-03 | info | fixed | d9f3a2e (plan 05-07: cogmaster__cart_probe refuses a body over 1 MiB after the copier-header strip) |
| IN-04 | info | skipped | two's complement on every supported target, and the eight-lane hash equality guards it; (int8_t) is implementation-defined only on paper |
| IN-05 | info | deferred | SEED-001: --probe region_guess on unknown region and ignored --machine/--region; rides the phase 6 plan that touches probe.c |
| IN-06 | info | deferred | SEED-002: the saved fault record accepts values the core never records; tighten with the next state-format change |

Fix round 1 of 2 (project rule 7); remaining findings settled in the v2.0 tune-up. Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
