---
phase: 15
review: 15-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "Fuzz job is still `continue-on-error` after its stated exit condition was met"
  - id: WR-02
    severity: warning
    disposition: open
    title: "Ledger-format migration and first-introduction branches in the compat step can no longer run"
  - id: WR-03
    severity: warning
    disposition: fixed
    title: "Nightly runs on pull requests with no concurrency group, and overlaps CI's macOS jobs"
  - id: WR-04
    severity: warning
    disposition: fixed
    title: "Nightly `apt-get` calls have no retries; every `ci.yml` call has them"
  - id: WR-05
    severity: warning
    disposition: skipped
    title: "The new ledger stamp names an unpushed, docs-only commit that a squash merge will leave outside `main`"
  - id: IN-01
    severity: info
    disposition: open
    title: "Comments on the jobs left alone cite measurements the live data no longer supports"
  - id: IN-02
    severity: info
    disposition: open
    title: "`gh pr list` in the digest silently stops at 30 pull requests"
  - id: IN-03
    severity: info
    disposition: open
    title: "The generated page cuts an HTML entity in half in two rows"
open: 4
total: 8
recorded: 2026-11-13T21:21:43.137Z
---

# Phase 15: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 15-REVIEW-FIX.md |
| WR-02 | warning | open | - |
| WR-03 | warning | fixed | 15-REVIEW-FIX.md |
| WR-04 | warning | fixed | 15-REVIEW-FIX.md |
| WR-05 | warning | skipped | 15-REVIEW-FIX.md |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
