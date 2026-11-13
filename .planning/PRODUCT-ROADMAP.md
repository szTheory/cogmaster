---
title: "Product roadmap (rolling)"
summary: "Immediate / Near / Mid / Long direction for cogmaster with the evidence and promotion trigger for each horizon; ROADMAP.md is the execution contract, seeds hold the source narratives."
read_when: "At every new-milestone and complete-milestone step, and when a finding changes priorities."
updated: 2026-11-13
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

## Shipped — v2.1 "Fewer failing cartridges" (v0.14.0 to v0.14.2)

Shipped 2026-11-13 with 10 of 10 requirements, audit status tech_debt and no gaps (`milestones/v2.1-MILESTONE-AUDIT.md`). Three phases, each merged by one pull request and released automatically: (13) a counts-only boot diagnostic in the runner and the library judge, with a leak scan, that grouped the 205 failing default-profile rows by cause; (14) the causes fixed or closed with a stated reason: routine cartridge writes and accesses to ports nothing decodes stopped being faults, the legacy video-mode rows were closed as not emulated by design, the boots check was shown sound, behaviour revision 18 and no hash pin moved; (15) the owner's library re-run merged through the ratchet (285 rows from `fail` to `pass`, none lost, the page at pass 1029 and fail 96) and the drifted CI timeouts re-derived. Requirements: `milestones/v2.1-REQUIREMENTS.md`.

**Unresolved debt carried into the next milestone** (none blocking): stale fault-vocabulary lines in `docs/audio.md`, `docs/embedding.md` and `docs/runner.md`; the reg-0 board question (needs new evidence); the owner's batched hand-offs (SEED-010 judges, SEED-009 thresholds); the rest of SEED-015 and SEED-007 (ledger rows, review leftovers); BIOS follow-ups (SEED-011), the hash-less audio predicate (SEED-012) and PAL and Japanese FM follow-ups (SEED-014).

## Immediate — recommended, awaiting owner approval: v2.2 "Legacy video modes"

Nothing is owner-approved yet. The recommendation below comes from a nine-lens fan-out plus an adversarial pass on
2026-11-13 (product, VDP, SG-1000 machine, states and movies, browser/ports/tools, verification and CI, debt, sibling
lessons, legal and security); the reasoning is in `.planning/research/NEXT-MILESTONE.md`.

**Headline (player-visible):** cartridges that use the TMS9918 Graphics I and II modes, mostly Korean MSX-style ports
and a few Western Sega-mapper carts, draw their real picture in RetroArch instead of a blank or wrong one. Most of
the 46 failing default-profile rows that wait on these modes (graphics1 13, graphics2 33, no M4=1 row) should leave
the unsupported-mode leaf; fewer than 46 will flip, because earlier rounds sent 5 to 8 percent of rows on to a next
fault and a static correct picture still fails the `distinct >= 2` boots predicate.

- **Rider PR before phase 16** (a `docs:`/`fix:` PR, not a phase): the false `docs/audio.md:77` and stale
  `docs/embedding.md` lines, the library run path refusing a dirty tree for the `core=` stamp, `curl --retry` on the
  assembler fetch.
- **Phase 16 `feat(vdp)`**: Graphics I and II (modes 0000 and 0010) on SMS1, SMS2 and Game Gear in SMS video, in a new
  `src/vdp_legacy.c`, one dispatch per line, fixed TMS palette, 4-slot legacy sprite engine, mode-aware backdrop.
  Behaviour revision 19 and state format 17 (the saved sprite slots lack colour, early-clock and a 0..15 row, so the
  layout does change). Closed-form pixel oracles and owned WLA-DX fixtures written first, including one that enters
  Graphics II after frame 600; draft PR early to read the perf gate. The fault stays for text, multicolour, the mixes,
  the invalid text modes and native Game Gear.
- **Phase 17 `fix(compat)`**: after the release merges, run the library on `main` so the ledger stamp is an ancestor of
  `main`, merge through the ratchet, re-derive timeouts, prune the dead diagnose vocabulary (`pclass`, `wclass`,
  `where=port`, `rom_write`, `vdpmode`) once, and sweep the docs.
- **Out of v2.2:** text and multicolour (only if `diagnose` shows 5 or more rows landing there), the SG-1000 machine, the
  SC-3000, multicarts.

**Alternatives weighed and why they lost:** states first (largest universal gap, but its trigger has not fired on
evidence and every behaviour revision already refuses older states, so doing modes first costs no migration);
modes plus the SG-1000 machine in one milestone (two unrelated risks, the SG-1000 "pass" is weaker still); the
browser build (rule 2 is settled by DEC.68, but it is sequenced after states); a standalone tune-up phase (the debt is 2 to 3 hours and
lands in files the feature edits).

## Mid — v3.0 "States and rewind", v3.1 "SG-1000", v4.0 "Measured accuracy" (SEED-002, SEED-006, SEED-003)

v3.0 (pre-committed as the milestone after v2.2): a public `cogmaster_state_*` API (API 15), libretro
serialize and unserialize with `savestate_features`, rewind and run-ahead behind an in-process harness, and the
`SYSTEM_RAM` memory export (a read-only mirror serves achievements; cheat writes need a new accessor, so it is
a phase item, not a rider). First fix the open review items IN-07, IN-02, IN-06 and IN-04 and add the missing
`fuzz.state` target, and run a spike on dropping the `field` buffers from the blob (a state measures about 317 KB,
not the 28 KiB in ARCHITECTURE). No movie format and no freeze of the state layout. **Trigger:** v2.2 shipped.
v3.1: the SG-1000 and SG-1000 II machine as a thin profile (209 of 214 pinned entries should boot under the boots
check; the ledger grows from 1,986 to 2,200 rows). **Trigger:** modes shipped, or the owner asks for an SG-1000
image to run; promotable ahead of v3.0 at low rework cost.
v4.0: diagnostic ROMs run on the owner's consoles settle the fitted VDP and PSG constants; nightly oracle
lanes against released binaries; the per-dot VDP model only where a measured case needs it. **Trigger:** a
stable failing set in the ledger that documentation does not explain.

## Long — browser, movies, v5.0 "Tools" (SEED-004), v6.0 "Everywhere" (SEED-005)

The browser (SEED-008): the library as a zero-import wasm module with a hash test in the repo and the page in a
sibling repo that pins the release asset (DEC.68, owner decision 2026-11-13: rule 2 and DEC.01 stay unchanged). Movies (SEED-002's second
half): an own format only when a named consumer exists; BizHawk import needs an inflate seed and stays out of the
reference sources (use the TASVideos format pages). Hooks, debugger views, `ctypes` docs rather than PyPI or npm
packages; handheld ports and a libretro buildbot listing only after save states exist. **Triggers:** a user ask or
a met instructions-per-frame budget; each seed names its own.

## Decision log

| Date | Change | Why |
|---|---|---|
| 2026-10-18 | Initial horizons set during preparation | Library shares from the CC0 MAME lists (653 SMS, 823 GG, 214 SG-1000 entries at `1d7a454ed79e`); sibling velocity evidence (every phase a release; fixtures chosen at kickoff); owner decisions: SMS + GG in v1.0, SG-1000 as a seed, repo created at preparation |
| 2026-10-18 | The browser build is SEED-008 with a wasm32 hash lane already in phase 1 | Owner: running efficiently in the browser through WebAssembly is very important |
| 2026-11-10 | v1.0 shipped and archived; horizons unchanged, nothing yet moved into Immediate | The order of v2.0 and later was not re-ranked: no new evidence at close, and the next scope needs owner approval at `/gsd-new-milestone` |
| 2026-11-12 | v2.0 shipped and archived; v3.0 stays next in order, nothing moved into Immediate | The owner's own-library run, not a new milestone, is what removes `is_experimental`; no new evidence re-ranks states and movies, the browser or SG-1000, and the next scope needs owner approval at `/gsd-new-milestone` |
| 2026-11-13 | v2.1 shipped and archived; SEED-006 becomes the preferred next step, nothing moved into Immediate | The Phase 14 outcomes show 46 of the 49 remaining failing rows need the legacy modes, which is the trigger SEED-006 names ("already needed for an SMS title in the ledger"); the other horizons were not re-ranked and the next scope needs owner approval at `/gsd-new-milestone` |
| 2026-11-13 | v2.2 "Legacy video modes" recommended for Immediate; v3.0 re-scoped to "States and rewind" and pre-committed as the milestone after it; the SG-1000 machine moved to v3.1; movies and the browser moved to Long | A nine-lens fan-out and an adversarial pass ranked Graphics I and II first on hard evidence and low risk. Player value alone favours states, so the order is the owner's to swap. Movies have no named consumer, and the owner settled the browser question as DEC.68 (the library as wasm here, the page in a sibling repo). Awaiting owner approval at `/gsd-new-milestone` |
