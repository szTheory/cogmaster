---
id: SEED-008
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: milestone v1.0 has shipped (weigh it against v2.0 at that new-milestone step), or the owner asks for a web build sooner
scope: medium
---

# SEED-008: cogmaster in the browser — an efficient WebAssembly build

## Why This Matters

The owner ranks running efficiently in the browser through WebAssembly as very important (2026-10-18). A web
page reaches players who will not install a frontend. Efficiency counts twice here: a browser tab on a laptop
or phone runs on battery, and the core's work per frame sets both its speed and its power draw. The cheap half,
a wasm32-wasi hash lane in CI, is in v1.0 phase 1 as a report-only lane.

## When to Surface

**Trigger:** v1.0 shipped, so there is something worth playing; weigh it against v2.0 at that new-milestone
step. Sooner if the owner asks.

## Scope Estimate

**Medium** — one or two phases: an Emscripten build of the core and a ~150-line JS harness (canvas 256×192 or
160×144, AudioWorklet at 48 kHz, keyboard and Gamepad API) on a static page; the player picks a cartridge file
locally and the page ships no ROM or BIOS (ASSET_POLICY); the test card and the project's own diagnostic ROMs
as built-in content; frame and audio hashes equal to native (the WASI lane); a size budget for the `.wasm`;
frame-time and power checks in Chrome, Firefox and Safari, including on a phone.

## Breadcrumbs

- `.planning/preparation/HOSTS-AND-INTEGRATION.md` section 6.
- `.planning/preparation/ARCHITECTURE.md`: portability row (no threads, files or mutable globals; 32-bit `size_t`).
- `.planning/preparation/SMS-ECOSYSTEM-AND-USERS.md` section 4: work per frame and device budgets.
- `.planning/preparation/ENGINEERING.md` sections 8, 9 and 14.
- `.planning/seeds/SEED-005-everywhere.md`: the other ports.

## Notes

No threads means no SharedArrayBuffer, so no COOP/COEP headers, and any static host works. Browser SMS
emulators exist (ECOSYSTEM 1); cogmaster's would be the accurate, permissive one other sites can embed.
