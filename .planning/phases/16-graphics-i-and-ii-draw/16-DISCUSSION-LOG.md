# Phase 16: Graphics I and II draw - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-13
**Phase:** 16-Graphics I and II draw
**Areas discussed:** Graphics I/II tables and masks, legacy sprites and status, renderer architecture and state, fixtures and oracles, riders/docs/provenance/ship

The owner's standing fan-out instruction applied: five parallel research agents, one per area, then an adversarial pass
against the code, then one set of recommendations that is followed. No per-question prompts were asked.

---

## Graphics II masks (R3 and R4)

| Option | Description | Selected |
|--------|-------------|----------|
| No masking anywhere | TI manual: low bits "must be all 1s"; masking undocumented | |
| MAME 315_5124 model on all chips | AND `idx` with `(R4&3)<<8\|0xFF` and `(R3&0x7F)<<3\|7` | |
| SMS1 masks, SMS2 and GG do not | `msvdp` hardware note for SMS2/GG; existing `vdp_masks` flag | ✓ |
| MAME TMS9918A coupling of R3 into the pattern | unverified on Sega silicon | |
| ares model (R3 low bits ignored) | | |

**Choice:** SMS1 masks as MAME 315_5124, SMS2 and GG-in-SMS-mode unmasked (about 50 and 60 percent).
**Notes:** All sources agree on all-ones values. The prep note's "MAME contradicts" claim traced to a mis-attribution (the TMS file does not say SMS2 and GG do not mask; `315_5124.cpp` masks on every revision).

## Palette and backdrop

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed TMS table on all profiles | ares | |
| Fixed table on SMS1 and SMS2, CRAM 16+n on GG in SMS mode | msvdp, SMS Power, MAME | ✓ |
| Backdrop uses the TMS colour even with display off | would move boot-frame pins | |
| Backdrop gated on display | | ✓ |

**Choice:** fixed table on SMS, CRAM 16+n on GG; TMS backdrop only with display on and a legacy mode.
**Notes:** Power-on zero registers are Graphics I with display off.

## Sprite Y threshold, end of list, early clock

| Option | Description | Selected |
|--------|-------------|----------|
| `Y >= $E0` negative | ares, TI | ✓ |
| `Y > $E0` negative | MAME tms9928a | |
| `Y >= $F0` negative | MAME SMS legacy path, same as mode 4 | |

**Choice:** `>= $E0`; `$D0` ends the list unconditionally; early clock is attribute bit 7; R0 bits 3 and 5 ignored.

## Colour-0 collision and fifth-sprite index

| Option | Description | Selected |
|--------|-------------|----------|
| Pattern bits decide (colour-0 sprites collide) | TI 2.3.2, Young | ✓ |
| Opaque pixels only | MAME SMS | |
| Asymmetric stack rule | ares (shown buggy) | |

**Choice:** datasheet rule; fifth-sprite index latches first-wins until a status read; bits 4..0 read `$1F` when clear; no F gate.
**Notes:** No hardware evidence for any of these on the SMS VDP; revisit with the SG-1000.

## Module shape and state

| Option | Description | Selected |
|--------|-------------|----------|
| Grow `vdp_render.c` | | |
| New `vdp_legacy.c` with a per-line span and one class decision per call | | ✓ |
| Share `sprite[8]` widened to four fields plus flags | | ✓ |
| Separate 4-slot struct | duplicates counters and validators | |

**Choice:** new module, shared widened slot array, state format 17, behaviour revision 19.
**Notes:** Mid-line switches are dot-exact to the register write (checked in `vdp.c`); the fixtures lens had said per line and was corrected.

## Fixtures, oracles and pins

| Option | Description | Selected |
|--------|-------------|----------|
| Many ROMs, one per mask pair | | |
| Two assembly sources, six images, exhaustive cases in process | | ✓ |
| Byte-compare of hashes against base | fails by design with new fixtures | |
| One-time subset compare | | ✓ |

**Choice:** oracle commit before the renderer, tile-index formulation, literal cases, draft pull request right after the dispatch skeleton.

## Riders

| Option | Description | Selected |
|--------|-------------|----------|
| Dirty tree: stamp `+dirty` | rejected late, after a full run | |
| Dirty tree: refuse early, tracked ledger only | | ✓ |
| Assembler retry via `--retry-all-errors` in `fetch-pin.sh` | wrong script; retries 404 | |
| Assembler retry loop in `WlaDx.cmake` with hash checked once after | `EXPECTED_HASH` is fatal itself | ✓ |

**Choice:** as in CONTEXT D-27 to D-30.

## Claude's Discretion

File and function names in `vdp_legacy.c`; fixture linking function layout; extra mask pairs; doc wording; separate-struct fallback.

## Deferred Ideas

Text and multicolour modes; SG-1000 revisit of the model choices; diagnose prune and `ci.yml` (Phase 17); CI-speed seeds; MAME screenshot (optional only).
