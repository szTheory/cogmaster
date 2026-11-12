---
title: "Product roadmap (rolling)"
summary: "Immediate / Near / Mid / Long direction for cogmaster with the evidence and promotion trigger for each horizon; ROADMAP.md is the execution contract, seeds hold the source narratives."
read_when: "At every new-milestone and complete-milestone step, and when a finding changes priorities."
updated: 2026-11-12
---

# Product roadmap (rolling)

`ROADMAP.md` (created at kickoff) is GSD's execution contract for the current milestone. This file carries the
direction beyond it; each horizon names the seed that holds its narrative and the trigger that promotes it.
Planning milestones (v1.0, v2.0, …) are not package versions: packages follow pre-1.0 semver and every phase that
adds a capability publishes a new minor release. Every milestone names a behaviour a player can see (sibling
lesson XFER-047); governance and evidence work rides inside the tune-up phase, never as a milestone of its own.

**Refresh rule.** At each milestone boundary the agent reconciles shipped and partial work, keeps unresolved debt
visible, re-ranks the horizons, records why the order changed or stayed, presents the preferred next scope with
alternatives, and moves only owner-approved scope into Immediate. The file is snapshotted as
`milestones/vX.Y-PRODUCT-ROADMAP.md` at close. Future horizons stay less detailed than the current one.

## Shipped — v1.0 "SMS and Game Gear games play with sound in RetroArch" (v0.1.0 to v0.4.0)

Shipped 2026-11-09 with 26 of 26 requirements, audit status tech_debt and no gaps (`milestones/v1.0-MILESTONE-AUDIT.md`). Four phases, each merged by one pull request and released automatically: (1) a test frame in RetroArch with the
whole delivery path (library, runner, libretro core, six-platform CI, big-endian and wasm32 lanes, attested
releases, install line); (2) the Z80 matches the public SingleStepTests vectors on every cycle, with the bus,
the Sega, no-mapper and Codemasters boards, ports and the Pause NMI; (3) games play in RetroArch: VDP mode 4 on both machines (SMS field
and Game Gear window with its 12-bit palette), two pads, regions, battery saves, self-authored SMS and GG
fixtures with boot-to-play checks, the owner's carts through a local preset, the compatibility ledger;
(4) sound: the PSG with Game Gear stereo and the YM2413 for the Japanese profile, hashes identical on every
platform. Requirements: `milestones/v1.0-REQUIREMENTS.md`. Evidence: `.planning/research/SUMMARY.md`.

**Unresolved debt carried into the next milestone** (none blocking; SEED-007 holds the tune-up): provisional CI timeouts (windows-arm64 failed one main run), the perf gate still in report mode with the `boot_to_play` allow line to 2027-01-07, open review dispositions from phases 1, 2 and 4, the README's missing ROM step, nightly lanes not yet run on the sound code, and third-party sound ROM conformance with no lane.

## Shipped — v2.0 "The whole library" (v0.5.0 to v0.12.0)

Shipped 2026-11-12 with 49 of 49 requirements, audit status tech_debt and no gaps (`milestones/v2.0-MILESTONE-AUDIT.md`). Eight phases, each merged by one pull request and released automatically: (5) the tune-up of the v1.0 debt; (6) the Korean boards, Zemina, Nemesis, 4 PAK and Hi-Com detected hash-first; (7) 93C46 EEPROM and battery saves that survive a restart; (8) Sports Pad, Paddle and 3-D glasses at pin level; (9) the Light Phaser; (10) BIOS boot from the user's own dump; (11) PAL and Japanese FM over the ledger; (12) the whole Master System and Game Gear list in the ledger, with `compat.exit` tying `is_experimental` to the judged rows. The flag stays until the owner's own-library run meets the floors (decision 2026-11-12). Requirements: `milestones/v2.0-REQUIREMENTS.md`.

**Unresolved debt carried into the next milestone** (none blocking): the owner's batched run (SEED-013, with the SEED-010 judges and the SEED-009 thresholds), timeout drift in asan, the nightly bench and bench-wasm32 and the other CI-speed items (SEED-015, SEED-007), BIOS follow-ups (SEED-011), the hash-less audio predicate (SEED-012) and PAL and Japanese FM follow-ups (SEED-014).

## Immediate — none

No milestone is in progress and no scope is owner-approved. **Preferred next step:** the owner's one batched own-library run, which can remove `is_experimental` (SEED-013), then a small tune-up from SEED-015 when the weekly digest names the drifted jobs. **Alternatives weighed:** v3.0 states and movies (the next horizon below), the browser build (SEED-008) and SG-1000/SC-3000 mode (SEED-006). Nothing moves into Immediate until the owner approves scope at `/gsd-new-milestone`.

## Mid — v3.0 "States and movies" (SEED-002), v4.0 "Measured accuracy" (SEED-003)

v3.0: fixed-size validated save states, rewind and run-ahead claims in the libretro `.info` behind their
tests, an own movie format with BizHawk and Dega import, Playstead continuation. **Trigger:** v2.0 shipped, or
Playstead's `sms`/`gg` integration needs continuation first.
v4.0: diagnostic ROMs run on the owner's consoles settle the fitted VDP and PSG constants; nightly oracle
lanes against released binaries; the per-dot VDP model only where a measured case needs it. **Trigger:** a
stable failing set in the ledger that documentation does not explain.

## Long — v5.0 "Tools" (SEED-004), v6.0 "Everywhere" (SEED-005), the browser (SEED-008), SG-1000 (SEED-006)

Hooks, debugger views, ROM-hacker aids, Python and Gymnasium bindings; handheld and console ports and the
libretro buildbot; the WebAssembly page; the SG-1000/SC-3000 profile. **Triggers:** a user ask or a met
instructions-per-frame budget; each seed names its own.

## Decision log

| Date | Change | Why |
|---|---|---|
| 2026-10-18 | Initial horizons set during preparation | Library shares from the CC0 MAME lists (653 SMS, 823 GG, 214 SG-1000 entries at `1d7a454ed79e`); sibling velocity evidence (every phase a release; fixtures chosen at kickoff); owner decisions: SMS + GG in v1.0, SG-1000 as a seed, repo created at preparation |
| 2026-10-18 | The browser build is SEED-008 with a wasm32 hash lane already in phase 1 | Owner: running efficiently in the browser through WebAssembly is very important |
| 2026-11-10 | v1.0 shipped and archived; horizons unchanged, nothing yet moved into Immediate | The order of v2.0 and later was not re-ranked: no new evidence at close, and the next scope needs owner approval at `/gsd-new-milestone` |
| 2026-11-12 | v2.0 shipped and archived; v3.0 stays next in order, nothing moved into Immediate | The owner's own-library run, not a new milestone, is what removes `is_experimental`; no new evidence re-ranks states and movies, the browser or SG-1000, and the next scope needs owner approval at `/gsd-new-milestone` |
