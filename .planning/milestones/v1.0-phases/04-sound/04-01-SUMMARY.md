---
phase: 04-sound
plan: "01"
subsystem: planning-documents
tags: [requirements, roadmap, decisions, preparation, sound, reset-rule]

requires:
  - phase: 03-games-play-in-retroarch
    provides: the bus, reset and state contracts whose tick origin the amended decisions describe
provides:
  - SND-01, SND-02 and SND-03 and ROADMAP Phase 4 criteria 1 to 3 reworded to owned fixtures, closed-form units, the lockstep test and an achievable 801 or 802 cadence
  - third-party ROM conformance recorded as deferred, with its evidence, once in REQUIREMENTS.md
  - the single SOFT-reset rule (owner-confirmed 2026-11-09) in 04-CONTEXT D-01 and D-24, with the tick origin carried into D-11, D-18 and D-21
  - D-04, D-15, D-22 and D-35 amendments that plans 04-04, 04-06, 04-10, 04-12 and 04-14 rely on
  - corrected audio facts in the preparation notes (cadence, LFSR period, volume entry 7, emu2413, headroom rule, RetroArch runs locally)
affects: [04-02, 04-03, 04-04, 04-05, 04-06, 04-10, 04-12, 04-14]

actuals:
  tokens: 10800
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "Amendments keep the original decision text and append one 'Amended at planning' sentence with its evidence"

key-files:
  created: []
  modified:
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/preparation/DECISIONS.md
    - .planning/phases/04-sound/04-CONTEXT.md
    - .planning/preparation/SMS-AUDIO.md
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/CONFORMANCE.md
    - .planning/preparation/HOSTS-AND-INTEGRATION.md

key-decisions:
  - "Ticks and frames count from the last load or reset of either kind; across a SOFT reset the synth keeps its phase accumulator, ring, integrator, DC estimator, levels and undrained PCM and clears the trace log, while each chip returns to its power-on constants and step origin and its levels reach the synth at tick 0"
  - "Third-party SN76489 and YM2413 test ROM conformance is deferred with no lane, label or schedule; owned psg_sweep and fm_sweep fixtures plus units and the lockstep test carry SND-01 and SND-02"
  - "SND-03 cadence reads 801 or 802 samples per NTSC frame (965 or 966 PAL) with cumulative total floor(F x 3,504,512 / 4,375)"

patterns-established:
  - "Planning-evidence amendments are appended to the locked decision, never silently replace it"

requirements-completed: [SND-01, SND-02, SND-03]

coverage:
  - id: D1
    description: "SND-01, SND-02 and ROADMAP criteria 1 and 2 name the owned psg_sweep and fm_sweep fixtures, the closed-form units and the lockstep test; the deferral and its evidence are stated once in REQUIREMENTS.md"
    requirement: SND-01
    verification:
      - kind: other
        ref: "plan task 1 verify chain (grep over REQUIREMENTS.md, ROADMAP.md, DECISIONS.md and 04-CONTEXT.md) then sh scripts/hygiene.sh --tree"
        status: pass
    human_judgment: false
  - id: D2
    description: "SND-03 and ROADMAP criterion 3 state 801 or 802 samples per NTSC frame (965 or 966 PAL) with the floor formula, and the old exactly-per-frame clause is gone"
    requirement: SND-03
    verification:
      - kind: other
        ref: "plan task 1 verify chain, SND-03 and criterion 3 greps and the negated exactly-clause grep"
        status: pass
    human_judgment: false
  - id: D3
    description: "04-CONTEXT carries the single owner-confirmed SOFT rule in D-01 and D-24, the shared tick origin in D-11, D-18 and D-21 (with the fault.not_faults test name), and the D-04, D-15, D-22 and D-35 amendments, nine amendment markers in all"
    requirement: SND-02
    verification:
      - kind: other
        ref: "plan task 1 verify chain, amendment-marker count, origin phrase, negated stale-phrase grep"
        status: pass
    human_judgment: false
  - id: D4
    description: "The preparation notes carry the corrected facts and no line presents third-party test ROMs as built in CI, a PR lane, the oracle or the way to settle a hardware question"
    requirement: SND-01
    verification:
      - kind: other
        ref: "plan task 2 verify chain (fact greps, negated JoppyFurr/TestRom pipeline) then sh scripts/hygiene.sh --tree"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-09
status: complete
plan_head_before: 07f85740a95e8284268fe778dee469d27c79333b
plan_head_after: 4785c04dff18b9e82b9745573dd6a8219faba4db
commits: 2
---

# Phase 4 Plan 01: Sound requirement, roadmap, decision and preparation amendments Summary

**Requirements, roadmap criteria, decisions and preparation notes aligned to owned audio fixtures, an achievable 801 or 802 per-frame cadence and one owner-confirmed SOFT-reset rule, with third-party ROM conformance recorded as deferred.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-11-09T19:28:00Z (approximate)
- **Completed:** 2026-11-09T19:33:00Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- SND-01 and SND-02 and ROADMAP criteria 1 and 2 no longer require a third-party ROM; they name the owned `psg_sweep` and `fm_sweep` fixtures, the closed-form units and the lockstep test, and one sentence under Sound states the evidence for the deferral.
- SND-03 and criterion 3 state the cadence a single frame can hold (801 or 802 samples NTSC, 965 or 966 PAL) with the cumulative floor formulas.
- 04-CONTEXT D-01 and D-24 carry the one SOFT rule with its code evidence (`cogmaster__bus_origin`, `docs/embedding.md` "## Reset", `test_frame_origin`) and the owner's confirmation; D-11, D-18 and D-21 take the same tick origin and D-21 names the real test `fault.not_faults`.
- D-04 (`<< 21` for the 32-phase table), D-15 (`src/opll_reg.c` for the 800-line rule), D-22 (`definitions[7]`, loop `i < 6`) and D-35 (the 16 x rings x (4 x PSG steps + 5 x OUT writes + FM samples) bound) each carry one appended amendment.
- DEC.22, DEC.35, DEC.42, DEC.46 and DEC.47 and the SMS-AUDIO, ARCHITECTURE, CONFORMANCE and HOSTS-AND-INTEGRATION notes no longer carry the stale facts (801,034, the 65,535 LFSR, entry 6568, "cannot clip", the ymfm and table-knob statements, the JoppyFurr built-in-CI and oracle claims, the sandbox statement).

## Task Commits

1. **Task 1: Amend SND-01..03, roadmap criteria, decision rows, SOFT rule and origin amendments** - `813aab1` (docs)
2. **Task 2: Correct stale preparation facts and third-party ROM claims** - `4785c04` (docs)

**Plan metadata:** recorded in the following docs commit.

## Files Created/Modified

- `.planning/REQUIREMENTS.md` - amended SND-01 to SND-03 and the single deferral sentence
- `.planning/ROADMAP.md` - Phase 4 Success Criteria 1 to 3 only
- `.planning/preparation/DECISIONS.md` - DEC.22, DEC.35, DEC.42, DEC.46, DEC.47
- `.planning/phases/04-sound/04-CONTEXT.md` - nine appended or replaced amendments
- `.planning/preparation/SMS-AUDIO.md` - cadence, LFSR period, entry 7, tick, attenuation-coded levels, FM-CORE, FM-MIX, test ROM rows, section 9 rows and the soft-reset row
- `.planning/preparation/ARCHITECTURE.md` - psg, opll and synth rows, audio model and hash rows, section 5 reset text, P4 row
- `.planning/preparation/CONFORMANCE.md` - boot_to_play and expected.txt statements, the JoppyFurr and GG candidate rows, hardware-question rows
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` - RetroArch runs locally; `audio_driver=null` does not isolate audio

## Decisions Made

- The SOFT rule lives in D-01 and is referenced by D-24, so every later plan reads one statement; the PCM accumulator and ring survive SOFT while chip step origins restart with the tick count.
- The kernel quality line in SMS-AUDIO section 5 now says about -83 dB (float simulation) instead of the unreproduced -87 dB, matching D-03; the integer spectral test remains the arbiter.
- Beyond the plan's list, small stale items in the same sentences were fixed (the `cogmaster_audio_format()` call, `--vgm` marked deferred, the SMS-AUDIO risk paragraph naming ymfm); each is a consequence of a locked decision, not new scope.

## Deviations from Plan

None - plan executed exactly as written. The extra stale-item fixes above sit inside the paragraphs the plan already rewrote.

## Issues Encountered

None. Both verify commands exited 0 on the first full run after the edits (one intermediate wording slip in the D-21 amendment, which reused a banned phrase, was fixed before the first commit).

## Verification

Run from the repository root; exit statuses recorded, no logs committed.

- Task 1 verify chain (grep chain then `sh scripts/hygiene.sh --tree`): exit 0.
- Task 2 verify chain (fact greps, negated JoppyFurr/TestRom pipeline, then `sh scripts/hygiene.sh --tree`): exit 0.
- `sh scripts/hygiene.sh --tree` after the second commit: exit 0.

## Known Stubs

None. This plan changes documents only.

## Threat Flags

None. No code, endpoint or file-access surface was added.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for 04-02 (API 9 door). Plans 04-02 to 04-14 read the SOFT rule, the cadence wording and the amended D-04, D-15, D-22 and D-35 from 04-CONTEXT.md. Phase requirements SND-01 to SND-03 remain open in the traceability table: this plan amends their wording only, and the behaviour that satisfies them lands in later plans.

---
*Phase: 04-sound*
*Completed: 2026-11-09*
