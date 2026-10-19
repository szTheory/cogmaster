---
id: SEED-001
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: milestone v1.0 has shipped (SMS and Game Gear games play with sound in RetroArch) and the next milestone is about which games and peripherals work
scope: large
---

# SEED-001: The whole library plays — every mapper, every peripheral, saves and PAL (v2.0)

## Why This Matters

After v1.0, cartridges on the Sega mapper, unmapped 32/48 KB images and Codemasters boards run with two pads,
Pause, battery RAM and the NTSC/PAL and Japan/Export profiles. What remains is what players and collectors
notice next: the Korean mappers (MSX 4PAK, Nemesis, Janggun, Hicom), the 93C46 EEPROM used by Game Gear sports
titles, the Light Phaser, the 3-D glasses, the Sports Pad and Paddle, the SG-1000/SC-3000 modes (SEED-006), the
BIOS boot path with the user's own dump, and the compatibility ledger over the whole CC0 MAME lists (653 SMS,
823 Game Gear entries at MAME `1d7a454ed79e`).

## When to Surface

**Trigger:** v1.0 shipped and the next milestone is about library breadth. Opens with SEED-007 (the tune-up),
which this time also builds the weekly triage digest, the CI-efficiency report and the `quality.json` ratchet.

## Scope Estimate

**Large** — a milestone. One phase per thing a player can see: the remaining mappers with their hash-DB
detection; saves (EEPROM serialisation, `.srm` interop with other emulators, runner `--save-dir` atomic writes,
a hosted save round trip); the light gun and the other controllers at pin level; full PAL timing and the
Japanese FM profile over the ledger; the ledger over the whole CC0 lists, after which the libretro `.info`
drops `is_experimental`.

## Breadcrumbs

- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` sections 2–4 and 7: mappers and their library
  share, controllers, saves, the milestone table.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 1 and 4: `.srm` layout, light-gun input, the
  runner's save options, Playstead's save contract.
- `.planning/preparation/CONFORMANCE.md` section 5: the ledger and its ratchet.
- `.planning/preparation/SMS-SYSTEM.md`: PAL timing, BIOS behaviour and hand-off.

## Notes

The Light Phaser depends on the H-counter latch timing and the VDP's per-line model; schedule it after the
v1.0 line timing has held for a phase.
