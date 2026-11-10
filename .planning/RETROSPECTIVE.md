# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — SMS and Game Gear games play with sound in RetroArch

**Shipped:** 2026-11-09 (release v0.4.0; phase 4 PR #11, release PR #12, audit PR #13)
**Phases:** 4 | **Plans:** 62 | **Sessions:** 37 Runtime Log rows in STATE.md, this close included

### What Was Built
- Phase 1: the whole delivery path before any emulation: library, `cogmaster-run` and libretro core draw a hashed test card, built and released on six platforms plus big-endian and wasm32 lanes, installed into RetroArch by one README line (v0.1.0).
- Phase 2: an own Z80 that matches the public SingleStepTests vectors on every cycle, the tick-lattice bus, the Sega, no-mapper and Codemasters boards, ports, the Pause NMI and the first-fault trace (v0.2.0).
- Phase 3: VDP mode 4 on both machines, the Game Gear window, two pads, regions, battery saves and the compatibility ledger, with owned fixtures playing in a hosted RetroArch (v0.3.0).
- Phase 4: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile, an exact 801 or 802 samples per frame, `--hash-audio`, and a hosted RetroArch recording against a silent control (v0.4.0).
- 26 of 26 requirements, 399 of 399 tests in the `ci` workflow, frame and audio hashes byte-identical on the eight hosted lanes.

### What Worked
- One pull request, one automated release per phase: four releases (v0.1.0 to v0.4.0) over the 22 days, each merged on green CI, with the audit finding 18 of 18 connections wired and 6 of 6 flows complete.
- Proving the CPU against public vectors before any game ran: the vector suite ran 160,400 executed cases before the VDP existed.
- Owned fixtures with a RAM mailbox and frozen pins in text files, compared on every lane through `hashes.txt`: determinism was shown by equal bytes, not asserted.
- A lockstep oracle for a port: the YM2413 matched the unmodified MIT emu2413 over 2,000,000 seeded samples before it touched the bus, and the run found a real 16-bit overflow in the chip's sum.
- Automation-first verification: the owner verified nothing by hand, and the hosted RetroArch checks stayed inside the 300-line budget.

### What Was Inefficient
- Provisional CI timeouts were set before the code existed. The first hosted run of the phase 4 head failed GCC, MSVC, UBSan and macOS x64 on timeouts, and the windows-arm64 timeout (3 minutes) later failed a main run.
- Review dispositions were not triaged in their phase: phase 1 still has 12 open rows, two of them live warnings (WR-02, WR-04), and phases 2 and 4 carry open info findings.
- Budgets were left at their ceiling: `tests/retroarch` ends at 299 of 300 lines and `docs/testing.md` at 7 bytes of headroom, so the next change must pay for itself.
- Work that was meant to follow a phase did not: the perf gate is still report-only with a `boot_to_play` allow line to 2027-01-07, and nightly lanes have not run on the sound code.
- The README did not move with the scope: its Quick start still covers only the no-content test card.
- The execution chain was one sequential line per phase (62 plans, parallelization off); Phase 4 alone had 15 plans in 15 waves.

### Patterns Established
- Each phase ends in a release: Conventional Commit pull request title, `gh pr merge --auto --squash`, CI run confirmed on the head commit, release pull request confirmed green.
- Evidence is a command, an exit status and a run URL, never a committed log.
- Closed-form unit oracles for chips, owned WLA-DX fixtures for behaviour, frozen hash pins for determinism, and an independent decoder or recording as the control for hosted checks.
- Every repository check ships with a negative control that fails with its named message.

### Key Lessons
1. Measure a CI timeout on the first hosted run of the code it covers before leaving it labelled provisional; a guess from an earlier phase does not hold when the per-frame work grows.
2. Triage review dispositions at the end of the phase that produced them; at milestone close they are debt that has lost its context.
3. When a budget is within a line or a few bytes of its cap, raise or shrink it in the same plan that reaches it, not in the plan that needs one more line.
4. Move the README with the scope in the phase that changes what a user can do, not after the audit.
5. Port third-party logic against a frozen digest of the unmodified original before wiring it to the bus; the oracle found a defect the register tests could not.

### Cost Observations
- Model mix by Runtime Log row: 21 claude-sonnet-5-5 (57%), 8 GPT-6 under Codex (22%), 7 claude-opus-5-5 (19%), 1 claude-fable-5-1 (3%); phase 4 planning used claude-opus-5-5 for the planner.
- Sessions: 37 Runtime Log rows, one GSD step per session.
- Notable: phases 1 and 2 were planned and executed on Opus, phase 3 on Codex and phase 4 on Sonnet; all four passed verification.

### Tech Debt Carried Forward
From `.planning/milestones/v1.0-MILESTONE-AUDIT.md` (status tech_debt, no gaps):
- Phase 1: provisional CI timeouts; untriaged review dispositions (WR-02, WR-04 live); the post-merge CI follow-ups todo (perf gate in report mode, nightly issue path unexercised, no dependabot configuration, wasm32 report-only).
- Phase 2: review findings IN-01 to IN-06 deferred; fuzz targets do not drain audio or the trace log.
- Phase 3: README Quick start covers the test card only.
- Phase 4: `boot_to_play` perf allow line to 2027-01-07; hosted sound check proves non-silence rather than PCM equality; review IN-01 to IN-04 open; be32 and wasm32 audio timings provisional; nightly lanes not yet run on the sound code; `cogmaster_info.audio_channels` and `.fm` read only by tests; third-party SN76489 and YM2413 ROM conformance deferred with no lane.

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Sessions | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.0 | 37 | 4 | One phase, one pull request and one release each; automation-first verification |

### Cumulative Quality

| Milestone | Tests (ci workflow) | Source lines (src and include) | Hashed lanes |
|-----------|---------------------|--------------------------------|--------------|
| v1.0 | 399 | about 7,700 | 8 hosted lanes, byte-identical |

### Top Lessons (Verified Across Milestones)

1. Not yet verified across milestones; the v1.0 lessons above are the candidates.
