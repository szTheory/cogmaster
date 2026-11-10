---
phase: 01
review: 01-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: skipped
    title: "Size-tag contract promises zero-fill for short structs, but the core reads and writes caller structs directly"
  - id: WR-02
    severity: warning
    disposition: deferred
    title: "A failed PR file listing yields `release_inputs=false`, so a release-pipeline change can merge unrehearsed"
  - id: WR-03
    severity: warning
    disposition: skipped
    title: "`release.yml` has never been parsed by GitHub, and nothing validates its `concurrency.queue` key"
  - id: WR-04
    severity: warning
    disposition: deferred
    title: "The release App's private key is usable from any same-repository PR branch"
  - id: IN-01
    severity: info
    disposition: fixed
    title: "The comment and docs say the clock is 10,738,635 Hz, but the frame-rate fraction is for 10,738,636.36 Hz"
  - id: IN-02
    severity: info
    disposition: fixed
    title: "The PNG zlib header comment states the wrong window size"
  - id: IN-03
    severity: info
    disposition: fixed
    title: "The nightly comment claims the default shell has no pipefail"
  - id: IN-04
    severity: info
    disposition: deferred
    title: "`publish.sh prepare` matches asset names as regular expressions"
  - id: IN-05
    severity: info
    disposition: skipped
    title: "The runner's and the adapter's frame-size guards run after the core has already written the buffer"
  - id: IN-06
    severity: info
    disposition: skipped
    title: "The CRC-32 table is duplicated in the core and in the runner"
  - id: IN-07
    severity: info
    disposition: deferred
    title: "The state checksum leaves the header's `frame` and `ticks` unprotected, and the fault frame is not cross-checked"
  - id: IN-08
    severity: info
    disposition: skipped
    title: "Rehearsals publish real attestations for PR-built archives on the public repository"
open: 0
total: 12
recorded: 2026-11-10T06:00:00.000Z
---

# Phase 01: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | skipped | no struct has grown yet, so the size-tag gap is vacuous today; the grown-input copy rule belongs to the phase that first appends a field to an input struct (phase 8) |
| WR-02 | warning | deferred | SEED-007: tooling finding (scripts/classify-changes.sh, the listing-failed sentinel counts as a known path), deferred under AGENTS rule 7, not fixed in phase 5 |
| WR-03 | warning | skipped | release.yml has since run successfully on main (run 42741490115), so GitHub parses it and accepts its concurrency key |
| WR-04 | warning | deferred | SEED-007: accepted risk; a reviewer-gated environment would break AGENTS rules 6 and 8 |
| IN-01 | info | fixed | 0227433 (plan 05-08: frame-rate comment and docs/embedding.md state 315/88 MHz x 3) |
| IN-02 | info | fixed | 0227433 (plan 05-08: runner/png.c says a 32 KiB window) |
| IN-03 | info | fixed | 0227433 (plan 05-08: the nightly report comment drops the pipefail claim) |
| IN-04 | info | deferred | SEED-007: tooling finding (scripts/publish.sh, asset names matched as regular expressions), deferred under AGENTS rule 7, not fixed in phase 5 |
| IN-05 | info | skipped | the guards are defensive and the core writes within the declared frame size, so they cannot fire; no behaviour changes |
| IN-06 | info | skipped | the runner is a separate deliverable that uses only the public API and cannot link core internals; a small copy beats a dependency (AGENTS style) |
| IN-07 | info | deferred | SEED-002: settle the header CRC and the fault-frame check with the state format, before a public state API freezes it |
| IN-08 | info | skipped | rehearsal attestations are signed by ci.yml from refs/pull and verify refuses them for a release (policy.publish_ref, policy.publish_signer) |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
