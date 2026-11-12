---
id: SEED-016
status: dormant
planted: 2026-11-12
planted_during: after v0.13.0, library run review
trigger_when: the next milestone is scoped; this is the recommended first item of it
scope: medium
---

# SEED-016: why a third of the checked cartridges fail the boots check

## Why This Matters

The first full library run published 396 default-profile passes and 210 default-profile `fail` rows (the run printed
fail=207, 603 judged). The page lists them honestly but nobody has looked at why. A fail here means the boots check
(a sampled display that is not blank, and no first fault within 600 frames) did not hold. Fixing a few causes should
flip many rows to `pass`; that is the biggest real improvement left in the core.

## What the ledger already shows (no ROMs needed)

- By mapper (ntsc default rows, fail / judged): `sega` 169 / 505, `none` 22 / 65, `zemina` 8 / 8, `korean_nb` 5 / 5,
  `korean` 5 / 7, `nemesis` 1 / 1, `codemasters` 0 / 4, `hicom` 0 / 9.
- 171 PAL-profile fails: every one is also an NTSC fail. PAL is not a cause; the same carts fail both ways.
- Well-known plain Sega-mapper titles fail (Dead Angle, Phantasy Star, Wimbledon, Vigilante), so one third of ordinary
  carts failing is more likely a few shared causes or a too-strict check than 169 separate emulation faults.
- Whole mapper families failing (`zemina`, `korean_nb`, most `korean`) point at a mapper behaviour gap.

## Where to start

1. Add a per-cart diagnostic to the library judge (count of frames to first non-blank, first fault kind and PC, mapper
   chosen) that prints counts and kinds only, never ROM bytes or names beyond catalog titles.
2. Group the 210 by the diagnostic and fix by group size: check strictness first, then mapper families, then
   individual faults.
3. Re-run the judge and merge the flipped rows by pull request.

## Open question for the discuss step

The agent has not read the owner's library (the secrets guard blocks it; the owner passes `COGMASTER_ROM_DIR` in a
shell). The diagnostic needs real dumps. Decide whether the owner runs the diagnostic and pastes the grouped counts, or
authorises the agent to run it in this shell against the extracted folder (counts only, nothing committed).
