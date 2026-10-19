---
title: "Product roadmap (rolling)"
summary: "Immediate / Near / Mid / Long direction for cogmaster with the evidence and promotion trigger for each horizon; ROADMAP.md is the execution contract, seeds hold the source narratives."
read_when: "At every new-milestone and complete-milestone step, and when a finding changes priorities."
updated: 2026-10-18
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

## Immediate — v1.0 "SMS and Game Gear games play with sound in RetroArch" (committed)

Four phases, each merged by one pull request and released automatically: (1) a test frame in RetroArch with the
whole delivery path (library, runner, libretro core, six-platform CI, big-endian and wasm32 lanes, attested
releases, install line); (2) the Z80 matches the public SingleStepTests vectors on every cycle, with the bus,
the Sega, no-mapper and Codemasters boards, ports and the Pause NMI; (3) games play in RetroArch: VDP mode 4 on both machines (SMS field
and Game Gear window with its 12-bit palette), two pads, regions, battery saves, self-authored SMS and GG
fixtures with boot-to-play checks, the owner's carts through a local preset, the compatibility ledger;
(4) sound: the PSG with Game Gear stereo and the YM2413 for the Japanese profile, hashes identical on every
platform. Requirements: `.planning/REQUIREMENTS.md` v1. Evidence: `.planning/research/SUMMARY.md`.

## Near — v2.0 "The whole library" (SEED-001)

The remaining mappers (Korean, 93C46 EEPROM), the Light Phaser, 3-D glasses, Sports Pad and Paddle, the BIOS
boot path with the user's dump, `.srm` interop, full PAL and Japanese FM over the ledger, and the ledger over
the whole CC0 MAME lists. **Trigger:** v1.0 shipped. **Why next:** what collectors and players notice first
after the core plays; the owner's cart collection becomes the ratchet's input. Opens with SEED-007.
**Weighed at the same boundary:** the browser build (SEED-008) and SG-1000/SC-3000 mode (SEED-006), which can
ride inside v2.0 if the VDP's legacy modes are already needed for an SMS title.

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
