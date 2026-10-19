---
id: SEED-005
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: a port target is named (a handheld, a console homebrew SDK, the libretro buildbot) or the instructions-per-frame ratchet shows a low-power budget is met
scope: medium
---

# SEED-005: Everywhere — handhelds, console homebrew ports, the libretro buildbot, Playstead (v6.0)

## Why This Matters

The owner wants the core on low-power devices (RG35XX-class ARM handhelds, Nintendo DS/3DS, PSP homebrew) with
good battery life, in the libretro buildbot so RetroArch users get it without a manual install, and in Playstead.
An 8-bit console core that stays integer-only, allocation-free after load and free of mutable globals ports with
a toolchain file and a compile lane.

## When to Surface

**Trigger:** a named target or a met budget (ECOSYSTEM 4 gives the per-device instructions-per-frame budgets).

## Scope Estimate

**Medium** — compile-only lanes per SDK (devkitARM, PSPSDK) first, then one playable port the owner can try;
the buildbot recipe; the Playstead `sms`/`gg` system IDs and adapter pin.

## Breadcrumbs

- `.planning/preparation/SMS-ECOSYSTEM-AND-USERS.md` section 4: budgets.
- `.planning/preparation/ENGINEERING.md` sections 9 and 14: portability lanes, WASM and low power.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` section 4: Playstead.
