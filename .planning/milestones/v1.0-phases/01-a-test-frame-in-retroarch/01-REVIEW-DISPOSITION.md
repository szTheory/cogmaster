---
phase: 01
review: 01-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: open
    title: "Size-tag contract promises zero-fill for short structs, but the core reads and writes caller structs directly"
  - id: WR-02
    severity: warning
    disposition: open
    title: "A failed PR file listing yields `release_inputs=false`, so a release-pipeline change can merge unrehearsed"
  - id: WR-03
    severity: warning
    disposition: open
    title: "`release.yml` has never been parsed by GitHub, and nothing validates its `concurrency.queue` key"
  - id: WR-04
    severity: warning
    disposition: open
    title: "The release App's private key is usable from any same-repository PR branch"
  - id: IN-01
    severity: info
    disposition: open
    title: "The comment and docs say the clock is 10,738,635 Hz, but the frame-rate fraction is for 10,738,636.36 Hz"
  - id: IN-02
    severity: info
    disposition: open
    title: "The PNG zlib header comment states the wrong window size"
  - id: IN-03
    severity: info
    disposition: open
    title: "The nightly comment claims the default shell has no pipefail"
  - id: IN-04
    severity: info
    disposition: open
    title: "`publish.sh prepare` matches asset names as regular expressions"
  - id: IN-05
    severity: info
    disposition: open
    title: "The runner's and the adapter's frame-size guards run after the core has already written the buffer"
  - id: IN-06
    severity: info
    disposition: open
    title: "The CRC-32 table is duplicated in the core and in the runner"
  - id: IN-07
    severity: info
    disposition: open
    title: "The state checksum leaves the header's `frame` and `ticks` unprotected, and the fault frame is not cross-checked"
  - id: IN-08
    severity: info
    disposition: open
    title: "Rehearsals publish real attestations for PR-built archives on the public repository"
open: 12
total: 12
recorded: 2026-10-19T23:42:51.096Z
---

# Phase 01: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | open | - |
| WR-02 | warning | open | - |
| WR-03 | warning | open | - |
| WR-04 | warning | open | - |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |
| IN-05 | info | open | - |
| IN-06 | info | open | - |
| IN-07 | info | open | - |
| IN-08 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
