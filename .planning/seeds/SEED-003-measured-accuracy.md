---
id: SEED-003
status: dormant
planted: 2026-10-18
planted_during: preparation, before milestone v1.0
trigger_when: the compatibility ledger has a stable failing set that documentation alone does not explain, or the owner wants hardware-measured timing
scope: large
---

# SEED-003: Measured accuracy — hardware diagnostics, oracles and the fitted constants settled (v4.0)

## Why This Matters

Several VDP timings (H-counter jump points, line-interrupt reload edges, sprite-overflow flag timing, VRAM
write-spacing corruption, the SMS1 zoom bug, GG window edges) are documented from emulator inference, not from
scope captures. The owner has real consoles and carts. Diagnostic ROMs that print numbers (no verdicts) run on
hardware and settle each fitted constant; MAME and ares binaries locate divergences; Emulicious is the closed
reference other developers trust.

## When to Surface

**Trigger:** a stable failing set in the ledger, or the owner's hardware session. Not before v2.0.

## Scope Estimate

**Large** — a milestone: the diagnostic ROM set (mailbox protocol, on-screen hex, CRC-16) for VDP, PSG, FM and
ports; the hardware-run protocol; a nightly oracle lane against released binaries; the per-dot VDP model only
where a measured case needs it (ARCHITECTURE 9); per-line hooks for the homebrew debugger audience.

## Breadcrumbs

- `.planning/preparation/CONFORMANCE.md` sections 3 and 4.
- `.planning/preparation/SMS-VDP.md` sections 4 and 9; `SMS-AUDIO.md` section 1.
- `.planning/preparation/DECISIONS.md`: the open questions at its end.
