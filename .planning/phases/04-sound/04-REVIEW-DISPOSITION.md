---
phase: 04
review: 04-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "The trace log is not \"in causal order\", and its ticks do fall within one run"
  - id: WR-02
    severity: warning
    disposition: open
    title: "Impossible-state guard: `if (m->frame > 0u)` in `cogmaster__synth_frame_end`"
  - id: IN-01
    severity: info
    disposition: open
    title: "Wrong PSG step interval in the ring-slot derivation"
  - id: IN-02
    severity: info
    disposition: open
    title: "Impossible-state guard: `a.written > LR_AUDIO_CAPACITY` in the libretro drain"
  - id: IN-03
    severity: info
    disposition: open
    title: "Impossible-state guard: `source >= COGMASTER__SYNTH_SOURCES` in `cogmaster__synth_level`"
  - id: IN-04
    severity: info
    disposition: open
    title: "Loading a state keeps the previous timeline's pending trace records"
open: 5
total: 6
recorded: 2026-11-09T23:18:42.643Z
---

# Phase 04: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 04-REVIEW-FIX.md |
| WR-02 | warning | open | - |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
