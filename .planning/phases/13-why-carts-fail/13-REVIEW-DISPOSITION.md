---
phase: 13-why-carts-fail
review: 13-REVIEW.md
fix_report: 13-REVIEW-FIX.md
fix_rounds_used: 1
fix_rounds_allowed: 2
open: 9
---

# Phase 13 review disposition

One row per finding. `fixed` names the commit. `open` is a decision, with the reason, not an oversight.

| ID | Severity | Disposition | Note |
|----|----------|-------------|------|
| WR-01 | warning | fixed | 2e9a44b states what the diagnose line prints (runner/diag.h, docs/runner.md) |
| WR-02 | warning | open | `ram=live\|still` reads the last bus byte when the guest turns work RAM off in port $3E, so it can count noise. Fixing it changes which rows land in frozen versus alive_still, which makes the owner-run cause note (core 470b058) stale and needs another owner diagnose run. Owner decision: fix in Phase 14 or accept. |
| WR-03 | warning | fixed | 0e6e5bf traps INT, TERM and HUP so the diagnose scratch directory is removed |
| IN-01 | info | open | Failed diagnose leaves the previous diagnose.txt; runner build failure drops compiler output |
| IN-02 | info | open | A tiny file is `unsupported` in diagnose but `malformed_io` in run |
| IN-03 | info | open | Bare literals 600 and 60 in the awk reducer |
| IN-04 | info | fixed | Two wrong comments in scripts/library.sh, fixed inside 0e6e5bf |
| IN-05 | info | open | Wrap guard in cogmaster__diag_order cannot occur (necessity finding) |
| SD-01 | seed | open | Test timeouts each come from one hosted run |
| SD-02 | seed | open | Nothing guards C-table versus awk word-list drift |
| SD-03 | seed | open | diag_channels cannot see scratch outside OUT/diag |
| SD-04 | seed | open | tests/library/diagnose.cmake is 301 lines against the plan's 221 cap; no gate counts it |
