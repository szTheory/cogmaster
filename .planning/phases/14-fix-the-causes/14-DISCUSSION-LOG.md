# Phase 14: Fix the causes - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-12
**Phase:** 14-fix-the-causes
**Areas discussed:** fault policy and the boots check; the mapper_range group; unsupported video modes; mapper families, rom_write and port faults; phase shape and verification

The owner's fan-out boilerplate asked for one coherent set of recommendations that is followed, so no area was put to a vote. Five advisor agents researched one area each; the orchestrator reconciled them.

---

## Fault policy and the boots check

| Option | Description | Selected |
|--------|-------------|----------|
| Core stops latching benign kinds | Hardware-benign events are not recorded as the first fault; kind constants stay | ✓ |
| Core classifies (benign flag) | Keep recording, add a flag or counter; grows the public struct for one consumer | |
| Runner predicate ignores named kinds | No core change, but the record keeps only the first fault, so a benign one hides a later real one | |

**Choice:** core stops latching. **Notes:** boots check kept as is (`boots@600`, `distinct >= 2`); two no-fault failing rows settled by `distinct1800`.

## The mapper_range group (109)

| Option | Description | Selected |
|--------|-------------|----------|
| Drop the `$FFFC` unused-bit raise, keep the odd-image bank raise | Hardware ignores the unused bits; DEC.37 wrap stays | ✓ |
| Gate the Sega shift by image size now | Avoids junk bits 1-0 displacing banks; moves pins | |
| Keep both raises, fix the check | Hides nothing but fails carts for a harmless write | |

**Choice:** split by address and size class first, drop the `$FFFC` raise, gate the shift only if a trace shows the need.

## Unsupported video modes (36 static, 6 alive)

| Option | Description | Selected |
|--------|-------------|----------|
| Close static group as legacy TMS modes; fix alive group by evidence | Renderer is SEED-006 scope | ✓ |
| Implement Graphics I/II and TMS sprites now (`src/vdp_tms.c`, about 250 lines) | Could flip rows, but a feature phase on a summary-only spec | |
| Silence the fault for all non-mode-4 modes | Rows stay static and fail anyway | |

**Choice:** close with reason, record counts as trigger evidence for SEED-006; line-latched raise for the alive group if evidence shows a restart transient.

## Mapper families, rom_write and ports

| Option | Description | Selected |
|--------|-------------|----------|
| Families closed as legacy-mode cause unless a count shows mode 4; rom_write and port faults dropped in the core | Evidence-gated | ✓ |
| Keep rom_write for foreign-board exact registers as a canary | Extra table, advisor minority view | |
| Change mappers or scan thresholds for the families | No evidence of a mapper gap | |

**Choice:** drop; fix board choice per family only if the address class shows a whole family.

## Phase shape and verification

| Option | Description | Selected |
|--------|-------------|----------|
| Evidence plan first, one plan per group in descending rank, closures last; one PR `fix(core)` | One owner hand-off for the dumps | ✓ |
| One plan per fault kind | Mixes groups that interact | |
| Several PRs | More releases and CI cost for no gain | |

**Choice:** first option; behaviour revision 17 to 18 once; base-versus-head `hashes.txt` comparison for the no-moved-pin proof.

## Claude's Discretion

Evidence word names; how the mode nibble reaches the diagnostic; plan count beyond the order; the trace exemplar.

## Deferred Ideas

TMS renderer and SG-1000 mode (SEED-006); shift gating unless needed; a public benign-event counter; a Start-press campaign beyond one row.
