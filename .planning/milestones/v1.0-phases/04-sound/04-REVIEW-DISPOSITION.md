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
    disposition: fixed
    title: "Impossible-state guard: `if (m->frame > 0u)` in `cogmaster__synth_frame_end`"
  - id: IN-01
    severity: info
    disposition: fixed
    title: "Wrong PSG step interval in the ring-slot derivation"
  - id: IN-02
    severity: info
    disposition: skipped
    title: "Impossible-state guard: `a.written > LR_AUDIO_CAPACITY` in the libretro drain"
  - id: IN-03
    severity: info
    disposition: skipped
    title: "Impossible-state guard: `source >= COGMASTER__SYNTH_SOURCES` in `cogmaster__synth_level`"
  - id: IN-04
    severity: info
    disposition: deferred
    title: "Loading a state keeps the previous timeline's pending trace records"
open: 0
total: 6
recorded: 2026-11-10T06:00:00.000Z
---

# Phase 04: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 04-REVIEW-FIX.md |
| WR-02 | warning | fixed | 04-REVIEW-FIX.md |
| IN-01 | info | fixed | 0227433 (plan 05-08: the SYNTH_EVENTS comment gives the corrected premise and cites its sources) |
| IN-02 | info | skipped | impossible-state guard kept as a bound on audio_pcm[] (AGENTS rule 4: every call stays bounded); it costs one compare |
| IN-03 | info | skipped | impossible-state guard kept as a bound on the width table (AGENTS rule 4); it costs one compare and a stray source is ignored, not written |
| IN-04 | info | deferred | SEED-002: clear the pending trace records with the state apply branch, before a public state load ships |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
