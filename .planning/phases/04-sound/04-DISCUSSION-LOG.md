# Phase 4: Sound - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-09
**Phase:** 04-Sound
**Areas discussed:** OPLL seed and FM core; synthesis kernel and rate contract; audio observability (hash, runner,
fixtures); libretro delivery and the hosted RetroArch check; PSG model and test-ROM conformance; FM control, mix,
profiles and state

**Method:** The owner's standing instruction was to fan out one research agent per decision area through every
relevant role lens, include an adversarial pass, favour primary sources, and follow the synthesised
recommendations without asking per-area questions. Six `gsd-advisor-researcher` agents ran in parallel (model
tier standard, Claude Sonnet 5.5); an adversarial cross-check of the reconciled CONTEXT.md followed. No
AskUserQuestion turns were used. "User's choice" below therefore reads "recommendation adopted".

---

## OPLL seed and FM core

| Option | Description | Selected |
|--------|-------------|----------|
| emu2413 v1.6.4 (MIT) | Plain C, integer widths, about 800 lines after pruning; one float table to convert | ✓ |
| ymfm OPLL (BSD-3) | OPLL rides the templated OPL engine, about 4,800 lines to prune to about 1,500 |  |
| ares `component/audio/ym2413` (ISC) | About 570 lines but nall bit-width types and float table generation; the fallback |  |

**User's choice:** emu2413 adopted (widens DEC.22). **Notes:** one instrument table, no A/B knob; lockstep test
against the unmodified original; Nuked-OPLL is GPL-2.0 and never opened; the SMS-AUDIO claim that emu2413 calls
sin/pow/log at init was wrong for the core path.

## Synthesis kernel and rate contract

| Option | Description | Selected |
|--------|-------------|----------|
| Sibling band-limited step, 16 × 64 Q20 | Measured about −95 dB at 44.1/48/96 kHz; research agent's recommendation | fallback |
| Sibling table as shipped (16 × 32 Q15) | −83.4 dB at 48 kHz (3.4 dB margin), −72.6 dB at 44.1 kHz; reuse, no new tool | ✓ |
| polyBLEP / box average / sample-and-hold | −27 / −15.6 / −4 dB: fail SND-05 |  |
| Oversample then decimate, minBLEP | Needs the 223.7 kHz tick rate and a 4.66:1 resample, or float generation |  |

**User's choice:** sibling table adopted for 48 kHz, 16 × 64 Q20 kept as the evidence-triggered fallback (the
adversarial pass applied the reuse-first ladder). **Notes:** impulse ring that sums per frame so unordered chip
catch-up is safe; one DC leaky estimator; 48 kHz only this phase; cadence corrected to 801,031 per 1,000 NTSC frames.

## Audio observability

| Option | Description | Selected |
|--------|-------------|----------|
| Caller-owned change log (`trace_records`) | No hook system exists; one branch per change; drained after `run_frame` | ✓ |
| The v5 hook system | Large unrelated feature |  |
| Compile-time tap | The runner would not test the shipped binary |  |

**User's choice:** change log adopted. **Notes:** attenuation-coded levels so retunes move only PCM digests;
merge in the runner; `--hash-audio` with no new flag; `--vgm` deferred.

## libretro delivery and the hosted check

| Option | Description | Selected |
|--------|-------------|----------|
| Drain the library; batch else single | Removes the adapter's accumulator (19 ppb PAL drift, outside the state visitor) | ✓ |
| Keep the adapter's accumulator | Simpler but drifts and is not in state |  |
| macOS mov recording analysed with `afinfo` | Measured with RetroArch 1.22.2; control session proves it is not vacuous | ✓ |
| Raw `--pcm-out` dump and `cmp` | Exact, but needs shift stability measured on the hosted runner |  |

**User's choice:** adopted. **Notes:** AV info keeps 48000.0 and the FRAME-03 fps pin; run-ahead out of scope;
e2e budget is exactly 300/300, so the new checks follow the YAML-step precedent (flagged in the summary).

## PSG model and test-ROM conformance

| Option | Description | Selected |
|--------|-------------|----------|
| Own fixtures gate; JoppyFurr ROMs as optional nightly locator | The ROMs print nothing; no upstream binary; GPL crt0 in our build | ✓ |
| Commit a built ROM | Does not fit ASSET_POLICY or `policy.notices` |  |
| Pass "the ROM's checks" | Unclaimable |  |

**User's choice:** adopted (requirement wording to be amended in planning). **Notes:** volume entry 6538 not
6568; LFSR period 57,337; `sms2_clip` omitted; GG `$06` only in native GG mode; a Phase 3 defect in `io.c` and
`state.c` (GG ports gated on machine, not on native mode) must be fixed.

## FM control, mix, profiles and state

| Option | Description | Selected |
|--------|-------------|----------|
| FM as a load-time profile field (`media.fm` AUTO/OFF/ON) | Presence changes guest-visible `$F2` reads | ✓ |
| Runtime toggle | Would desync states and hashes |  |
| Derive from region only | Forces Japan's readback onto export-flagged FM games |  |

**User's choice:** adopted. **Notes:** the "cannot clip by construction" claim was false at Q12 4096:1434;
headroom now comes from per-source gains and a no-clip test; one libretro option `cogmaster_fm`; one API/state
bump (API 9, revision 11, state 11); the `cogmaster_media` padding trap.

## Adversarial cross-check of the reconciled decisions

A separate reviewer attacked the first draft of CONTEXT.md against CLAUDE.md and the research. Verified and fixed:
the volume-law formula (the draft gave 4 dB steps), moving the hosted audio assertions into workflow YAML (a dodge
of the 300-line RetroArch budget; `docs/maintaining.md` requires hosted code under `tests/retroarch`), the "no signed
right shift" rule colliding with the lockstep proof of the OPLL port (a named floor-shift helper is now allowed
there), per-delta mixing that would drift (now gain the level and feed the difference), the missing SND-05 and
PSG cost decisions (now D-35), the period 0/1 ambiguity (constant high, no records, per SMS-AUDIO §2.2), reset
tick semantics, API/struct details, and the emu2413 pin label. Judgement calls made on the reviewer's evidence:
reuse the sibling kernel table at 48 kHz and drop the other rates and `tools/kernelgen` to a deferred fallback
(this deviates from the kernel research agent's recommendation of 16 × 64 Q20 and is flagged for the planner); the
third-party ROM lane and the FM ledger predicate moved to Deferred; SND-01 and SND-02 are amended, stated plainly.

## Claude's Discretion

Exact Q12 gain constants, trace capacity and struct shape, names inside the stated modules, plan order, whether the
lockstep oracle fetch is nightly or build-time.

## Deferred Ideas

See CONTEXT.md `<deferred>`: `--vgm`, `--pcm-out`, extra hosted lanes, hash pins at other rates, Mark III + FM unit,
per-chip volume options, run-ahead, SG-1000 constants, hardware captures.
