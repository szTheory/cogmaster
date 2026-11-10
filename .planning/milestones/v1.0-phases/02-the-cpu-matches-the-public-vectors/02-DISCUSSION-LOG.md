# Phase 2: The CPU matches the public vectors - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-10-19
**Phase:** 02-the-cpu-matches-the-public-vectors
**Areas discussed:** Z80 core structure, Per-cycle vector harness, VDP boundary in phase 2, Fixtures and mailbox, API and probe surface, First fault / loader / fuzz

Mode: advisor (minimal_decisive). The owner asked for a fan-out per decision point (all stakeholder lenses,
primary sources, adversarial pass) and for the synthesised recommendations to be followed without a question
round. One research agent per area; the orchestrator reconciled conflicts between agents.

---

## Z80 core structure

| Option | Description | Selected |
|--------|-------------|----------|
| Hand-written instruction-granular core | switch per page, x/y/z decode for regular blocks, inline flags, prefix = own step, access stamps at data T-state | ✓ |
| Generated cycle-stepped decoder (floooh style) | per-T-state bus for free; generator tool, macro-heavy, rejected by ARCHITECTURE §2 | |

**Choice:** hand-written core (D-04..D-11). **Notes:** the agent's traced-build seam was replaced by the vector agent's link seam (below).

## Per-cycle vector harness

| Option | Description | Selected |
|--------|-------------|----------|
| Link seam + access templates | test bus logs accesses, expands to T-states; 29,000/29,000 rebuilt exactly | ✓ |
| Traced build (`COGMASTER_TRACE=1`) recording per T-state | second compile of the Z80 with per-T-state samples | |
| Runtime callback | branch per access in the shipped build | |
| C converter vs `tools/sst2bin.py` | C chosen: no Python dependency | ✓ (C) |
| Access-form sample committed (≈ 14 MB) vs per-T-state (≈ 20 MB) vs generated at configure | access form committed | ✓ |
| Archive tarball via `fetch-pin.sh` vs sparse clone | tarball, digest pinned | ✓ |

**Choice:** D-12..D-17.

## VDP boundary in phase 2

| Option | Description | Selected |
|--------|-------------|----------|
| Decode-only stub | fixed values; fixtures hang on V-counter waits, TV detection impossible, a stub banned by D-09 | |
| VDP timing half, no renderer | counters, control port, regs, VRAM/CRAM stored, frame flag and /INT, Pause NMI | ✓ |

**Choice:** D-18..D-20. **Notes:** surfaced the Pause-line ambiguity (line 260 vs 261); settled from SP.047 as D-03.

## Fixtures and mailbox

| Option | Description | Selected |
|--------|-------------|----------|
| WLA-DX built at configure time on every leg | pinned tarball, two targets only | ✓ |
| Build once on Linux and share | only as a per-leg fallback | |
| Runner `--mailbox` (print only) + CMake `-P` judge | keeps exit table, phase 3 `--expect` absorbs the CSV | ✓ |
| In-process C test via internal headers | duplicates decoding, not runnable by hand | |

**Choice:** D-21..D-25. **Notes:** forces `cogmaster_peek` into the header (reconciled with the API agent).

## API and probe surface

| Option | Description | Selected |
|--------|-------------|----------|
| Narrow: probe, cart_info, peek, region/tv, two statuses, runner digests | every declaration has a consumer and a test | ✓ |
| Broad: also set_input, get_z80, get_counters, digests in core | ABI lock-in before hosts exist | |

**Choice:** D-26..D-30. **Notes:** the agent's combined `region` enum (AUTO/NTSC_U/NTSC_J/PAL) was overridden to keep DEC.42's separate region and tv fields; its 4 MB size bound was overridden by ARCHITECTURE §6's 1 MiB + 512; `peek` added for `--mailbox`.

## First fault, loader and fuzz

| Option | Description | Selected |
|--------|-------------|----------|
| Fault = undriven or unused-by-software; probes are counters | first fault stays informative | ✓ |
| ARCHITECTURE §8 read literally | fires on FM detection, keyboard probe, bank wrap in many games | |
| libFuzzer nightly on Linux + plain C replay on six legs per PR | | ✓ |

**Choice:** D-31..D-37.

## Claude's Discretion

Plan waves, exact module file split, fast-boot and open-bus details, arming the perf gate once control runs exist.

## Deferred Ideas

See CONTEXT.md `<deferred>`; the post-merge CI todo was reviewed and kept as its own `ci:` pull request.
