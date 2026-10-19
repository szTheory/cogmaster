---
id: SEED-002
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: milestone v2.0 has shipped and players, speedrunners or Playstead ask for save states, rewind, run-ahead or input movies
scope: large
---

# SEED-002: States, rewind, run-ahead and movies (v3.0)

## Why This Matters

Save states are the first thing a player asks a frontend for; speedrunners and TAS authors need frame-exact
movies, a lag counter and frame advance; RetroArch's rewind, run-ahead and netplay all need a fixed-size,
deterministic state; Playstead's continuation protocol needs a state that survives a fresh process.

## When to Surface

**Trigger:** v2.0 shipped. Sooner if Playstead's `sms`/`gg` integration lands first and needs continuation.

## Scope Estimate

**Large** — a milestone: the four-pass state visitor over every device (CPU incl. WZ/Q/R and pending EI, VDP
incl. read buffer and latch, PSG, FM, mapper, EEPROM, ports and TH latches); a fixed-size, CRC-checked state file
validated in full before it is applied; fork tests at every frame; libretro `serialize` with the
`deterministic` claim staged behind its tests; an own text movie format with lag frames, plus import of
BizHawk `.bk2` and Dega `.mmv`; the Playstead local-continuation adapter; rewind and run-ahead e2e in RetroArch
within the e2e line budget.

## Breadcrumbs

- `.planning/preparation/ARCHITECTURE.md` sections 7 and 8: state design, observability.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 1, 4 and 5: `.info` claims, continuation protocol,
  movie formats.
- `.planning/LESSONS.md` XFER-070: hidden-state checkpoints and omitted-field controls.

## Notes

Keep persistent saves, snapshots and movies as three formats with separate version rules (sibling lesson).
