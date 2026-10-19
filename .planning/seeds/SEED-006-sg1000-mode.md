---
id: SEED-006
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: v2.0 is being scoped, or the owner inserts an SG-1000 or SC-3000 image and asks for it to run
scope: medium
---

# SEED-006: SG-1000 and SC-3000 mode — the TMS9918 legacy modes, the keyboard and the older mappers

## Why This Matters

The Master System's VDP still implements the TMS9918 modes 0–3 that SG-1000 games use (and a few SMS titles
touch), the SG-1000 shares the Z80, PSG and most of the port map, and MAME's CC0 list has 214 SG-1000 entries.
The owner chose SMS + Game Gear for v1.0 (2026-10-18), so this mode is a seed, not a v1.0 requirement. Until it
lands, an SG-1000 image is recognised and refused with "unsupported: SG-1000 (planned)".

## When to Surface

**Trigger:** v2.0 scoping, or an owner ask. It is small enough to ride inside v2.0 if the VDP's legacy modes
are already needed for an SMS title in the ledger.

## Scope Estimate

**Medium** — one phase: modes 0–3 rendering with the SMS VDP's documented differences from a real TMS9918,
the SG-1000 memory map and 1 KB/2 KB RAM variants, the SC-3000 keyboard via ports $DE/$DF, Terebi Oekaki,
the Taiwanese multicart mappers, a machine profile `sg1000`/`sc3000`, and the `hash/sg1000.xml` ledger.

## Breadcrumbs

- `.planning/preparation/SMS-VDP.md` section 7.
- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` sections 2 and 3.
- `.planning/preparation/SMS-SYSTEM.md` sections 3 and 4.
